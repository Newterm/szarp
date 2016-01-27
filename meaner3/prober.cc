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
 * prober - daemon for writing 10-seconds data probes to disk, highly experimental ;-)
 * Most of code is shared with meaner3, data format is identical to szbase, except for
 * fact that there's one probe for each 10 seconds, not 10 minutes. Each file contains
 * data for one month, about 0,5 MB per parameter per month.
 * Data is written to disk in chunks of 6 elements every minute or at program termination.
 * One configuration parameter is required (except for default parcook_path and IPK):
 * prober:
 * cachedir=/path/to/cache/dir
 * Directory pointed by cachedir must exist and must be writeable.
 * TODO: setting limit of disk usage and removing old data to fit within limit.
 
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

#include "classes.h"
#include "prober.h"

#include "liblog.h"
#include "libpar.h"
#include "execute.h"
#include "szarp_config.h"
#include "szbase/szbbase.h"

#include "tmeaner.h"
#include "tparcook.h"
#include "texecute.h"

/** hary: nanosleep */
#include <time.h>

/** arguments processing, see info argp */
#include <argp.h>

const char *argp_program_version = "prober 3.""$Revision$";
const char *argp_program_bug_address = "coders@praterm.com.pl";
static char doc[] = "SZARP 10-seconds probes cache saving daemon.\v\
Config file:\n\
Configuration options are read from file /etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg,\n\
from section '" SZARP_CFG_SECTION "' or from global section.\n\
These options are mandatory:\n\
	IPK		full path to configuration file\n\
	cachedir	root directory for probes cache.\n\
	parcook_path	path to file used to create identifiers for IPC\n\
			resources, this file must exist.\n\
	months_count	number of full months to keep cache, older files are removed\n\
			from cache; data for 1 month for 1 parameters used about\n\
			0.5 MB of disk space, so set this parameter accordingly.\n\
These options are optional:\n\
	log		path to log file, default is " PREFIX "/log/" SZARP_CFG_SECTION ".log\n\
	log_level	log level, from 0 to 10, default is from command line\n\
			or 2.\n\
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

TProber* g_prober = NULL;

RETSIGTYPE g_CriticalHandler(int signum)
{
	sz_log(0, "prober: signal %d caught, exiting, report to author",
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
		sz_log(2, "prober: interrupt signal %d caught for further processing", signum);
	} else {
		/* signal '0' is program-generated */
		if (signum != 0) 
			sz_log(2, "prober: interrupt signal %d caught, cleaning up", 
				signum);
		g_prober->WriteParams(true);
		delete g_prober;
		sz_log(2, "prober: cleanup finished, exiting");
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

int main(int argc, char* argv[])
{
	struct arguments arguments;
	int i;
	TProber* prober;
	time_t last_cycle; /**< time of last cycle */
	char* probes_buffer_size;

	libpar_read_cmdline(&argc, argv);

	/* parse params */
	arguments.no_daemon = 0;
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	/* read params from szarp.cfg file */
	libpar_init_with_filename("/etc/szarp/szarp.cfg", 1);

	probes_buffer_size = libpar_getpar("", "probes_buffer_size", 0);

	/* Check for other copies of program. */
	if ((i = check_for_other (argc, argv))) {
		sz_log(0, "prober: another copy of program is running, pid %d, exiting", i);
		return 1;
	}

	if (probes_buffer_size) {
		prober = new TProber(atoi(probes_buffer_size));
		free(probes_buffer_size);
	} else {
		prober = new TProber();
	}

	assert (prober != NULL);
	
	/* Set signal handling */
	g_prober = prober;
	if (prober->InitSignals(g_CriticalHandler, g_TerminateHandler) != 0) {
		sz_log(0, "prober: error setting signal actions, exiting");
		return 1;
	}
	
	/* Load configuration data. */
	if (prober->LoadConfig(SZARP_CFG_SECTION) != 0) {
		sz_log(0, "prober: error while loading configuration, exiting");
		return 1;
	}
	
	libpar_done();

	/* Check base. */
	if (prober->CheckBase() != 0) {
		sz_log(0, "prober: cannot initialize base, exiting");
		return 1;
	}

	/* Load IPK */
	if (prober->LoadIPK() == -1) {
		sz_log(0, "prober: cannot set up IPK configuration, exiting");
		return 1;
	}
	
	/* Try connecting with parcook. */
	if (prober->InitReader() != 0) {
		sz_log(0, "prober: connection with parcook failed, exiting");
		return 1;
	}

	/* Go into background. */
	if (arguments.no_daemon == 0)
		daemon(
				0, /* set working dir to '/' */
				0);/* close stdout and stderr descriptors */
	sz_log(2, "prober started");
	
	time_t periods = 1;
	if (prober->IsBuffered()) periods = prober->GetBuffSize();

	last_cycle = 0;
	int cycle_count = 0;
	while (1) {
		/* Get current time */
		time_t t;
		time(&t);

		sz_log(2, "prober: t:%ld last:%ld cycle_count:%d periods:%ld",t,last_cycle,cycle_count,periods);
		time_t plan = prober->WaitForCycle(BASE_PERIOD, t);

		sz_log(2, "prober: done waiting plan:%ld",plan);

		if ((t - last_cycle > BASE_PERIOD) && (last_cycle > 0)) {
			sz_log(1, "prober: cycle lasted for %ld seconds, planned %ld", t - last_cycle, plan);
		} else if ((t / BASE_PERIOD) <= (last_cycle / BASE_PERIOD)) {
			/* Sometimes, because of clock skew, we are still in the same
			 * or even earlier cycle. */
			continue;
		}
		last_cycle = t;
		
		/* Connect with parcook. */
		prober->ReadParams();

		if (prober->IsBuffered()) {
			prober->ReadDataSnapshot();
		}

		cycle_count = (cycle_count + 1) % periods;
		if (cycle_count != 0) continue;

		/* Write data to base. */
		if (prober->IsBuffered()) {
			int curr_pos = prober->GetCurrPos();
			int write_pos = prober->GetWritePos();
			sz_log(3, "curr_pos:%d write_pos:%d", curr_pos, write_pos);
			if (write_pos != -1 && curr_pos != write_pos) {
				if (curr_pos == (write_pos - 1) % periods) {
					sz_log(1, "prober: fixing -1 pos drift");
					while (prober->GetCurrPos() != write_pos) {
						prober->ReadParams();
						prober->ReadDataSnapshot();
						struct timespec to_sleep;
						to_sleep.tv_sec = 0; to_sleep.tv_nsec = 1000;
						nanosleep(&to_sleep, nullptr);
					}	
							
				} else if (curr_pos == (write_pos + 1) % periods) {
					sz_log(1, "prober: fixing +1 pos drift");
					prober->WriteParamsMissed(1);
					prober->SetWritePos(curr_pos);
				} else {
					/* This is indeed fixable with more buffering */
					//prober->WriteParamsMissed(nlost);	
					sz_log(0, "prober: unfixable pos diff - data lost");
					prober->SetWritePos(curr_pos);
				}
			}
			prober->WriteParamsBuffered();
		} else
			prober->WriteParams();
	}
	
	return 0;
}
	
