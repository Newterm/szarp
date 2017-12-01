/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/*
 * meaner3 - daemon writing data to base in SzarpBase format
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id: meaner3.cc 1 2009-06-24 15:09:25Z isl $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <sys/resource.h>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp> 


#include "classes.h"
#include "meaner3.h"

#include "liblog.h"
#include "libpar.h"
#include "execute.h"
#include "szarp_config.h"
#include "szbase/szbbase.h"

#include "tmeaner.h"
#include "tparcook.h"
#include "texecute.h"


/** arguments processing, see info argp */
#include <argp.h>

const char *argp_program_version = "meaner 3.""$Revision$";
const char *argp_program_bug_address = "coders@praterm.com.pl";
static char doc[] = "SZARP database daemon.\v\
Config file:\n\
Configuration options are read from file /etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg,\n\
from section '" SZARP_CFG_SECTION "' or from global section.\n\
These options are mandatory:\n\
	IPK		full path to configuration file\n\
	datadir		root directory for base.\n\
	parcook_path	path to file used to create identifiers for IPC\n\
			resources, this file must exist.\n\
These options are optional:\n\
	log		path to log file, default is " PREFIX "/log/" SZARP_CFG_SECTION ".log\n\
	log_level	log level, from 0 to 10, default is from command line\n\
			or 2.\n\
These options are optional and are read from 'execute' section:\n\
	execute_stdout	path to file for standard output of executed jobs,\n\
			default is " PREFIX "/log/execute.stdout\n\
	execute_stderr	path to file for standard output of executed jobs,\n\
			default is " PREFIX "/log/execute.stderr\n\
	execute_sections list of sections with command to execute, sections\n\
			are reviewed in order\n\
Each of sections to execute may contain following options:\n\
	time		crontab-like description of date and time for section\n\
			execution; fields are (in order): day of week (from 1\n\
			to 7), month (1 to 12), day of month (1 to 31), hour\n\
			(0 to 23), minutes (0 to 59 but values are rounded\n\
			down by 10); wildcars (*), multiple values separated\n\
			by commas and ranges (with '-' sign) are possible;\n\
			fields are separated by space; option is mandatory\n\
	command		shell command to execute, mandatory option\n\
	limit		time limit in seconds for executing command; after\n\
			limit expiration command is interrupted (1 second\n\
			before limit SIGTERM signal is send, and then SIGKILL);\n\
			limit is measured from section start time (not from\n\
			command execution time), negative values are counted\n\
			as number of seconds before next full 10 minutes cycle;\n\
			default value is -1 (the same as 599)\n\
	retry		number of following 10 minutes cycle in which command\n\
			execution should be retried if command returned error\n\
			or was interrupted because of limit expiration,\n\
			default is 0\n\
	parallel	if option value is 'yes' following sections may be run\n\
			immediately, without waiting for command return,\n\
			default value is 'no'\n\
";

static struct argp_option options[] = {
	{"debug", 'd', "n", 0, "Set initial debug level to n, n is from 0 \
(error only), to 10 (extreme debug), default is 2. This is overwritten by \
settings in config file."},
	{"D<param>", 0, "val", 0,
	 "Set initial value of libpar variable <param> to val."},
	{"no-daemon", 'n', 0, 0,
	 "Do not fork and go into background, useful for debug."},
	{0}
};

struct arguments
{
	int no_daemon;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = (struct arguments *)state->input;

	switch (key) {
		case 'd':
		case 0:
			break;
		case 'n':
			arguments->no_daemon = 1;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };


/* GLOBAL VARIABLES */

volatile sig_atomic_t g_signals_blocked = 0;

volatile sig_atomic_t g_should_exit = 0;

TMeaner* g_meaner = NULL;
TExecute* g_exec = NULL;

RETSIGTYPE g_CriticalHandler(int signum)
{
	sz_log(1, "meaner3: signal %d caught, exiting, report to author",
			signum);
	/* resume default action - abort */
	signal(signum, SIG_DFL);
	/* raise signal again */
	raise(signum);
}

RETSIGTYPE g_TerminateHandler(int signum)
{
	if (g_signals_blocked) {
		g_should_exit = 1;
		sz_log(2, "meaner3: interrupt signal %d caught for further processing", signum);
	} else {
		/* signal '0' is program-generated */
		if (signum) 
			sz_log(2, "meaner3: interrupt signal %d caught, cleaning up", 
				signum);
		delete g_meaner;
		sz_log(2, "meaner3: cleanup finished, exiting");
		logdone();
#ifdef VALGRIND_IN_USE
		exit(0);
#else
		signal(signum, SIG_DFL);
		raise(signum);
#endif
	}
}

/***********************************************************************/

void log_ulimit()
{
	struct rlimit limit;
	getrlimit(RLIMIT_NOFILE, &limit);
	sz_log(2, "meaner3: RLIMIT_NOFILE soft %ld, hard %ld", limit.rlim_cur, limit.rlim_max);
}

int main(int argc, char* argv[])
{
	struct arguments arguments;
	int i;
	TMeaner* meaner;
	TStatus *status;
	time_t last_cycle; /**< time of last cycle */

#if BOOST_FILESYSTEM_VERSION == 3
	boost::filesystem::wpath::imbue(std::locale("C")); 	
#else
	boost::filesystem::wpath_traits::imbue(std::locale("C")); 	
#endif

	libpar_read_cmdline(&argc, argv);
	
	/* parse params */
	arguments.no_daemon = 0;
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	/* Check for other copies of program. */
	if ((i = check_for_other (argc, argv))) {
		sz_log(0, "meaner3: another copy of program is running, pid %d, exiting", i);
		return 1;
	}

	status = new TStatus();
	assert (status != NULL);
	meaner = new TMeaner(status);
	assert (meaner != NULL);
	
	/* Set signal handling */
	g_meaner = meaner;
	if (meaner->InitSignals(g_CriticalHandler, g_TerminateHandler) != 0) {
		sz_log(0, "meaner3: error setting signal actions, exiting");
		return 1;
	}
	
	/* Load configuration data. */
	if (meaner->LoadConfig(SZARP_CFG_SECTION) != 0) {
		sz_log(0, "meaner3: error while loading configuration, exiting");
		return 1;
	}
	
	libpar_done();

	/* Check base. */
	if (meaner->CheckBase() != 0) {
		sz_log(0, "meaner3: cannot initialize base, exiting");
		return 1;
	}

	/* Load IPK */
	if (meaner->LoadIPK() != 0) {
		sz_log(0, "meaner3: cannot set up IPK configuration, exiting");
		return 1;
	}
	
	/* Try connecting with parcook. */
	if (meaner->InitReader() != 0) {
		sz_log(0, "meaner3: connection with parcook failed, exiting");
		return 1;
	}

	/* Go into background. */
	if (arguments.no_daemon == 0)
		daemon(
				0, /* set working dir to '/' */
				0);/* close stdout and stderr descriptors */
	sz_log(2, "meaner3 started");
	log_ulimit();
	
	/* set alarm for next cycle */
	meaner->InitExec();
	g_exec = meaner->GetExec();

	last_cycle = 0;
	while (1) {
		/* Set status parameters info */
		status->NextCycle();
		
		/* Process alarms and wait for next save-time. */
		meaner->GetExec()->WaitForCycle();
		
		/* Get current time */
		time_t t;
		time(&t);

		/* Sometimes, because of clock skew, we are still in the same
		 * or even earlier cycle. */
		if ((t / BASE_PERIOD) <= (last_cycle / BASE_PERIOD))
			continue;
		last_cycle = t;

		/* Connect with parcook. */
		meaner->ReadParams();

		/* Write data to base. */
		meaner->WriteParams();

		/* Mark sections that should be run */
		meaner->GetExec()->MarkToRun(t);

		/* Run sections */
		meaner->GetExec()->RunSections();
	}
	
	return 0;
}
	
