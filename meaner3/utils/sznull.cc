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
 * sznull - program for deleting data range from szarpbase files
 * format
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
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

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include <sys/statvfs.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>

#include "liblog.h"
#include "libpar.h"
#include "execute.h"
#include "szarp_config.h"
#include "szbase/szbbase.h"

#define _ICON_ICON_
#define _HELP_H_

#include "szarp.h"
#include "conversion.h"

#undef _ICON_ICON_
#undef _HELP_H_

/** Path to main config file. */
#define SZARP_CFG "/etc/"PACKAGE_NAME"/"PACKAGE_NAME".cfg"

/** Name of section in main config file for reading data dir.. */
#define SZARP_CFG_SECTION "meaner3"

/** arguments processing, see info argp */
#include <argp.h>

const char *argp_program_version = "nodata ""$Revision: 6199 $";
const char *argp_program_bug_address = "coders@praterm.com.pl";
static char doc[] = "Fills all parameters in szbase with SZB_NODATA for provided data range.\v\
Config file:\n\
Configuration options are read from file /etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg,\n\
from section 'meaner3' or from global section.\n\
These options are mandatory:\n\
	IPK		full path to configuration file\n\
	datadir		root directory for base.\n\
These options are optional:\n\
	szbase_format	'printf like' format string for creating data base file \n\
			names from parameter name (%%s), year and month (%%d).\n\
	log		path to log file, default is " PREFIX "/log/meaner3\n\
	log_level	log level, from 0 to 10, default is from command line\n\
			or 2.\n\
";

static struct argp_option options[] = {
	{"debug", 'd', "n", 0, "Set initial debug level to n, n is from 0 \
(error only), to 10 (extreme debug), default is 2. This is overwritten by \
settings in config file."},
	{"D<param>", 0, "val", 0,
	 "Set initial value of libpar variable <param> to val."},
	{"start", 's', "", 0, "Start of time range. In format yyyy-mm-dd hh:mm."}, 
	{"end", 'e', "", 0, "End of time range. In format yyyy-mm-dd hh:mm."},
	{0}
};

struct arguments {
	time_t start;
	time_t end;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = (struct arguments *) state->input;
	
	switch (key) {
		case 'd':
		case 0:
			break;
		case 's':
		case 'e': {
			int year, month, day, hour, minute;
			if (sscanf(arg, "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute) != 5) {
				argp_usage(state);
				return EINVAL;
			}

			struct tm tm;
			tm.tm_sec = 0;
			tm.tm_min = minute;
			tm.tm_hour = hour;
			tm.tm_mday = day;
			tm.tm_mon = month - 1;
			tm.tm_year = year - 1900;
			tm.tm_isdst = -1;

			time_t t = mktime(&tm);
			if (t < 0) {
				argp_usage(state);
				return EINVAL;
			}

			if (key == 's')
				arguments->start = t;
			else
				arguments->end = t;
			break;
		}
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}


static struct argp argp = { options, parse_opt, 0, doc };

/** Parameter object */
class TSaveParam {
	public:
		TSaveParam();
		~TSaveParam();
		void SetParam(TParam *param);
		int AddData(SZB_FILE_TYPE value, time_t time);
		int Close();
	protected:
		void Open();
		std::wstring cname;	/**< Encoded name of parameter */
		bool is_open;
		int year, month;
		int maxprobes;
		int probes_c;
		SZB_FILE_TYPE *probes;
};

/** Main szbase base directory */
std::wstring data_dir;

TSaveParam::TSaveParam()
{
	probes_c = 0;
	is_open = false;
}

TSaveParam::~TSaveParam()
{
}

void TSaveParam::SetParam(TParam *p) {
	Close();
	year = -1; month = -1;
	cname.clear();
	cname = wchar2szb(p->GetName());
}

/** value != NO_DATA
 * @return 0 on success, 1 on critical error */
int TSaveParam::AddData(SZB_FILE_TYPE value, time_t time)
{
	assert (!cname.empty());

	int y, m, p;
	
	/* check if file has changed and has to be written */
	szb_time2my(time, &y, &m);

	if ((year != y) || (month != m))  
		if (Close())
			return 1;
	szb_time2my(time, &year, &month);

	if (!is_open)
		Open();

	/* get index */
	p  = szb_probeind(time);

	assert (p < maxprobes);

	/* fill in th gaps */
	for (int i = probes_c; i < p; i++)
		probes[i] = SZB_FILE_NODATA;
	/* set value */
	probes[p] = value;
	/* set probes counter */
	if (probes_c <= p)
		probes_c = p+1; 

	return 0;
}

int TSaveParam::Close()
{
	if (!is_open) {
		return 0;	
	}

	std::wstring path, name;
	int fd;
	int written, towrite;
	
	is_open = false;

	if (probes_c <= 0) {
		free(probes);
		return 0;
	}
	
	name = szb_createfilename_ne(cname, year, month);
	path = data_dir + L"/" +  name;

	if (szb_cc_parent(path)) {
		printf("\nError creating parent directory for file '%ls', errno %d\n",
				path.c_str(), errno);
		return 1;
	}
	fd = open(SC::S2A(path).c_str(), O_RDWR | O_TRUNC, SZBASE_CMASK);
	if (fd < 0) {
		if (errno == ENOENT) {
			return 0;
		}
		printf("\nError creating file '%ls', errno %d\n", path.c_str(),
				errno);
		return 1;
	}
	written = 0;
	
	towrite = probes_c * sizeof(SZB_FILE_TYPE);
	while (1) {
		int t = write(fd, ((char*)probes) + written, towrite);
		if (t > 0) {
			towrite -= t;
			written += t;
			if (towrite == 0)
				break;
		}
		if (t < 0) {
			if (errno == EINTR)
				continue;
			printf("\nError writing to file '%ls', errno %d\n",
					path.c_str(), errno);
			close(fd);
			return 1;
		}
		if (towrite == 0)
			break;
	}
	close(fd);
	
	probes_c = 0;
	free(probes);
	
	return 0;
}

void TSaveParam::Open() {
	std::wstring path;
	int r = 0;
	int read_bytes = 0;

	if (is_open)
		Close();

	probes_c = 0;

	std::wstring name = szb_createfilename_ne(cname, year, month);
	path = data_dir + L"/" + name;

	maxprobes = szb_probecnt(year, month);
	probes = (short int *) malloc(sizeof(short int) * maxprobes);
	assert (probes != NULL);

	int fd = open(SC::S2A(path).c_str(), O_RDWR);
	if (fd < 0) 
		goto cleanup;


	while ((r = read(fd, ((char*)probes) + read_bytes, maxprobes * sizeof(SZB_FILE_TYPE) - read_bytes)) >= 0) 
		if (r == 0 && errno != EINTR)
	       		break;
		else
			read_bytes += r;

	probes_c = read_bytes / sizeof(SZB_FILE_TYPE);

	if (r < 0) {
		probes_c = 0;
		printf("Error while reading file: %ls\n", path.c_str());
	} 

	close(fd);

cleanup:
	is_open = true;

}


int Nullfily(TParam *p, time_t start, time_t end) {
	TSaveParam sp;
	sp.SetParam(p);

	for (time_t t = szb_round_time(start, PT_MIN10, 0);
			t <= end; 
			t = szb_move_time(t, 1, PT_MIN10, 0)) {

		if (sp.AddData(SZB_FILE_NODATA, t) != 0)
			return 1;
	}

	sp.Close();

	return 0;

}

	
int SetNoData(time_t start, time_t end)
{
	char* c;
	char *ipk_path;
	int l;
	TSzarpConfig* ipk;
	
	/* Search only for 'system wide' config file, exit on error. */
	libpar_init_with_filename(SZARP_CFG, 1);
	
	ipk_path = libpar_getpar(SZARP_CFG_SECTION, "IPK", 0);
	if (ipk_path == NULL) {
		sz_log(0, "sznull: set 'IPK' param in "SZARP_CFG" file");
		return 1;
	}
	
	char *_data_dir = libpar_getpar(SZARP_CFG_SECTION, "datadir", 0);
	if (_data_dir == NULL) {
		sz_log(0, "sznull: set 'datadir' param in "SZARP_CFG" file");
		return 1;
	}
	data_dir = SC::A2S(_data_dir);

	c = libpar_getpar(SZARP_CFG_SECTION, "log_level", 0);
	if (c == NULL)
		l = 2;
	else {
		l = atoi(c);
		free(c);
	}
	l = loginit(l, NULL);

	ipk = new TSzarpConfig();
	if (ipk->loadXML(SC::A2S(ipk_path))) {
		sz_log(0, "sznull: error loading IPK configuration '%s'",
				ipk_path);
		return 1;
	};

	for (TParam *p = ipk->GetFirstParam(); p; p = p->GetNext(true)) {
		if (!p->IsInBase())
			continue;
		printf("Setting param %ls\n", p->GetName().c_str());
		if (Nullfily(p, start, end) != 0)
			return 1;
	}

	return 0;
}


int main(int argc, char* argv[])
{
	struct arguments arguments;
	int loglevel;	/**< Log level. */

	setbuf(stdout, 0);
	
	/* Set initial logging. */
	loglevel = loginit_cmdline(2, NULL, &argc, argv);

	/* Load configuration data. */
	libpar_read_cmdline(&argc, argv);
	
	/* parse params */
	arguments.start = (time_t) -1;
	arguments.end = (time_t) -1;
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	if (arguments.start == -1 
			|| arguments.end == -1 
			|| arguments.start > arguments.end) {
		printf("\nIsuffcient/invalid time specification\n");
		argp_help(&argp, stdout, ARGP_HELP_USAGE, "sznull");
		return 1;
	}

	printf("Start time: %s", asctime(localtime(&arguments.start)));
	printf("End time: %s", asctime(localtime(&arguments.end)));

	for (int i = 10; i > 0; --i) {
		printf("\rCommencing in: %2d seconds", i);
		sleep(1);
	}
	printf("\nGo!\n");
		
	SetNoData(arguments.start, arguments.end);

	libpar_done();

	return 0;
}
	
