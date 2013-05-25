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

/*Find and eliminates no data values in szbase, loosely based on peaktor and szbnull.*/

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
#include <argp.h>

#include "liblog.h"
#include "libpar.h"
#include "execute.h"
#include "szarp_config.h"
#include "tsaveparam.h"
#include "szbase/szbbase.h"

#include "szarp.h"
#include "conversion.h"

/** Path to main config file. */
#define SZARP_CFG "/etc/"PACKAGE_NAME"/"PACKAGE_NAME".cfg"

/** Name of section in main config file for reading data dir.. */
#define SZARP_CFG_SECTION "meaner3"


char argp_program_name[] = "szbndremover";
const char *argp_program_version = "sbndremover" "$Revision: 6199 $";
const char *argp_program_bug_address = "reksio@praterm.com.pl";
static char args_doc[] = "[ PARAMETER NAME ] ...";
static char doc[] = "Eliminates SZB_NODATA values for given params and time range.\v\
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
	std::vector<std::wstring> params;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = (struct arguments *) state->input;
	
	switch (key) {
		case 'd':
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
		case ARGP_KEY_ARG:
			arguments->params.push_back(SC::A2S(arg));
			break;
		case ARGP_KEY_END:
			if (arguments->params.size() == 0)
				argp_usage(state);
			break;
		default:
			break;
	}
	return 0;
}

std::string format_time(time_t t) {
	char tbuf[40];
	strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M", localtime(&t));
	return tbuf;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

double find_probe_value(szb_buffer_t* szb, TParam *p, time_t t) {
	for (size_t i = 1; i < 4 * 6; i++) {
		time_t tp = szb_move_time(t, -i, PT_MIN10, 0);
		time_t tn = szb_move_time(t, i, PT_MIN10, 0);
		double vp = szb_get_probe(szb, p, tp, PT_MIN10, 0);
		double vn = szb_get_probe(szb, p, tn, PT_MIN10, 0);

		if (!IS_SZB_NODATA(vp)) {
			if (!IS_SZB_NODATA(vn)) {
				std::cerr << "Using average from time: " << format_time(tp);
				std::cerr << " and time: " << format_time(tn) << " : " << (vn + vp) / 2;
				std::cerr << std::endl;
				return (vn + vp) / 2;
			} else {
				std::cerr << "Using value from time: " << format_time(tp);
				std::cerr << " : " << vp; 
				std::cerr << std::endl;
				return vp;
			}
		} else if (!IS_SZB_NODATA(vn)) {
				std::cerr << "Using value from time: " << format_time(tn);
				std::cerr << " : " << vn; 
				std::cerr << std::endl;
				return vn;
		}
	}

	t = szb_round_time(t, PT_DAY, 0);
	double v = szb_get_avg(szb, p, t, szb_move_time(t, 1, PT_DAY, 0));
	if (!IS_SZB_NODATA(v)) {
		std::cerr << "Using day average from time: " << format_time(t);
		std::cerr << " : " << v;
		std::cerr << std::endl;
		return v;
	}

	t = szb_round_time(t, PT_WEEK, 0);
	v = szb_get_avg(szb, p, t, szb_move_time(t, 1, PT_WEEK, 0));
	if (!IS_SZB_NODATA(v)) {
		std::cerr << "Using week average from time: " << format_time(t);
		std::cerr << " : " << v;
		std::cerr << std::endl;
	}
	return v;
}

std::vector<std::pair<time_t, double> > remove_nodata(szb_buffer_t* buf, TParam* p, time_t start, time_t end) {
	std::vector<std::pair<time_t, double> > vals;
	std::cerr << "Starting no data removal for param:" << SC::S2A(p->GetName()) << std::endl;
	for (time_t t = start; t <= end; t = szb_move_time(t, 1, PT_MIN10, 0)) {
		double v = szb_get_probe(buf, p, t, PT_MIN10, 0); 
		if (!IS_SZB_NODATA(v))
			continue;
		std::cerr << "Value for time " << format_time(t) << " is no data, processing." << std::endl;
		v = find_probe_value(buf, p, t);
		if (!IS_SZB_NODATA(v))
			vals.push_back(std::make_pair(t, v));
		else
			std::cerr << "No suitable data found, ignoring" << std::endl;
	}
	std::cerr << "No data removal for param:" << SC::S2A(p->GetName()) << " finished" << std::endl;
	return vals;
}

void save_real_param(double pw, TParam *p, std::vector<std::pair<time_t, double> >& vals, szb_buffer_t *szb) {
	TSaveParam sp(p);
	for (std::vector<std::pair<time_t, double> >::iterator i = vals.begin();
			i != vals.end();
			i++) 
		sp.Write(szb->rootdir.c_str(),
				i->first,
				(SZB_FILE_TYPE)round(i->second * pw),
				NULL, /* no status info */
				1, /* overwrite */
				0 /* force_nodata */);
}

void save_combined_param(double pw, TParam *p, std::vector<std::pair<time_t, double> >& vals, szb_buffer_t *szb) {
	TSaveParam sp_msw(p->GetFormulaCache()[0]);
	TSaveParam sp_lsw(p->GetFormulaCache()[1]);
	for (std::vector<std::pair<time_t, double> >::iterator i = vals.begin();
			i != vals.end();
			i++) {
		int v = round(i->second * pw);
		unsigned int* uv = (unsigned int*) &v;
		sp_msw.Write(szb->rootdir.c_str(),
				i->first,
				*uv >> 16,	
				NULL, /* no status info */
				1, /* overwrite */
				0 /* force_nodata */);
		sp_lsw.Write(szb->rootdir.c_str(),
				i->first,
				*uv & 0xffff,	
				NULL, /* no status info */
				1, /* overwrite */
				0 /* force_nodata */);
	}

}

int main(int argc, char* argv[])
{
	struct arguments arguments;
	int loglevel;	/**< Log level. */
	char *ipk_prefix;
	char *szarp_data_root;

	setbuf(stdout, 0);
	
	/* Set initial logging. */
	loglevel = loginit_cmdline(2, NULL, &argc, argv);

	/* Load configuration data. */
	libpar_read_cmdline(&argc, argv);
	libpar_init();

	/* parse params */
	arguments.start = (time_t) -1;
	arguments.end = (time_t) -1;
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	if (arguments.start == -1 
			|| arguments.end == -1 
			|| arguments.start > arguments.end) {
		printf("\nIsuffcient/invalid time specification\n");
		argp_help(&argp, stdout, ARGP_HELP_USAGE, argp_program_name);
		return 1;
	}

	szarp_data_root = libpar_getpar("", "szarp_data_root", 1);
	assert(szarp_data_root != NULL);

	ipk_prefix = libpar_getpar("", "config_prefix", 1);
	assert (ipk_prefix != NULL);

	libpar_done();

	std::vector<TParam*> params;

	IPKContainer::Init(SC::A2S(szarp_data_root), SC::A2S(PREFIX), L"");

	Szbase::Init(SC::A2S(szarp_data_root), NULL);

	TSzarpConfig *ipk = IPKContainer::GetObject()->GetConfig(SC::A2S(ipk_prefix));
	if (ipk == NULL) {
		sz_log(0, "Could not load IPK configuration for prefix '%s'", ipk_prefix);
		return 1;
	}

	szb_buffer_t *szb = Szbase::GetObject()->GetBuffer(SC::A2S(ipk_prefix));
	if (szb == NULL) {
		sz_log(0, "Error initializing SzarpBase buffer");
		return 1;
	}

	for (size_t i = 0; i < arguments.params.size(); i++) {
		TParam *p = ipk->getParamByName(arguments.params[i]);
		if (p == NULL) {
			std::wcerr << "Parameter '" << arguments.params[i] << "' not found in IPK\n\n";;
			return 1;
		}
		if (!p->IsInBase() && !p->GetType() == TParam::P_COMBINED) {
			std::wcerr << "Parameter '" << arguments.params[i] << "' is not in base and in not combined param\n\n";;
			return 1;
		}
		params.push_back(p);
	}

	std::cerr << "start time: " << format_time(arguments.start) << std::endl;
	std::cerr << "end time: " << format_time(arguments.end) << std::endl;

	for (size_t i = 0; i < params.size(); i++) {
		TParam *p = params[i];
		std::vector<std::pair<time_t, double> > vals = remove_nodata(szb, p, arguments.start, arguments.end);
		double pw = pow(10.0, p->GetPrec());
		if (p->GetType() == TParam::P_COMBINED)
			save_combined_param(pw, p, vals, szb);
		else
			save_real_param(pw, p, vals, szb);
	}

	libpar_done();

	return 0;
}
