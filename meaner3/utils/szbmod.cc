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
#include <sstream>
#include <fstream>
#include <stdexcept>
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


char argp_program_name[] = "szbmod";
const char *argp_program_version = "szbmod " "$Revision: 6199 $";
const char *argp_program_bug_address = "reksio@praterm.com.pl";
static char args_doc[] = "PARAMETER NAME [PARAMETER NAME]...\n";


static char doc[] = "Allows for fast fast data modification of szbase parameter.\n"
"Tool for quick modification of szbase historical values.\n"
"Szbmod allows for modification of historical data through\n"
"LUA formulas. Szbmod takes a lua formula, period of time and a param\n"
"to be modified. Then for each database probe from given time period\n" 
"it runs the formula much in the same way as it is done with drawdefinable\n"
"params. The values returned from formula are then stored in database.\n"
"For example if program is run with following params:\n"
"./szbmod -fformula -Dprefix=xxxx -s'2007-01-01 00:00' -e'2009-12-30 00:00' \"Sieæ:Sterownik:Temperatura zewnêtrzna\"\n"
"and contents of file formula is like that:\n"
"v = 2 * p(\"xxx:Sieæ:Sterownik:Temperatura zewnêtrzna\", t, pt)\n"
"then for given period values of \"Sieæ:Sterownik:Temperatura zewnêtrzna\" will be doubled.\n"
"Config file:\n\
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
	{"formula", 'f', "", 0, "File with formula to apply."},
	{0}
};

struct arguments {
	time_t start;
	time_t end;
	std::string formula;
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
		case 'f': {	
			std::fstream f(arg);
			std::ostringstream oss;
			oss << f.rdbuf();
			arguments->formula = oss.str();
			if (arguments->formula.empty()) {
				fprintf(stderr, "Failed to load formula from file: %s", arg);
				argp_usage(state);
			}
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
	strftime(tbuf, sizeof(tbuf), "%Y-%d-%m %H:%M", localtime(&t));
	return tbuf;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

std::vector<std::pair<time_t, std::vector<double> > > do_run(time_t start, time_t end, lua_State *lua, int pnum) {
	std::vector<std::pair<time_t, std::vector<double> > > vals;
	Lua::fixed.push(true);
	for (time_t t = start; t <= end; t = szb_move_time(t, 1, PT_MIN10, 0)) {
		lua_pushvalue(lua, -1);	
		lua_pushnumber(lua, t);	
		lua_pushnumber(lua, PT_MIN10);
		lua_pushnumber(lua, 0);
		int ret = lua_pcall(lua, 3, pnum, 0);
		if (ret != 0) {
			std::ostringstream oss;
			oss 
				<< "Error while getting parameter value for time: " << format_time(t)
				<< " error: " << lua_tostring(lua, -1)
				<< ", terminating" << std::endl;
			throw std::runtime_error(oss.str().c_str());
		} else {
			std::vector<double> vs;
			std::cerr << "Values for time " << format_time(t) << " are ";
			for (int i = 0; i < pnum; i++) {
				double v = lua_tonumber(lua, -(pnum - i));
				std::cerr << v << " ";
				vs.push_back(v);
			}
			std::cerr << std::endl;
			vals.push_back(std::make_pair(t, vs));
		}
		lua_pop(lua, pnum);

	}
	return vals;
}

void save_real_param(double pw, TParam *p, std::vector<std::pair<time_t, std::vector<double> > >& vals, szb_buffer_t *szb, size_t pn) {
	TSaveParam sp(p);
	for (std::vector<std::pair<time_t, std::vector<double> > >::iterator i = vals.begin();
			i != vals.end();
			i++)  {
		short int v;
		if (!std::isnan(i->second[pn]))
			v = (SZB_FILE_TYPE)round(i->second[pn] * pw);
		else
			v = SZB_FILE_NODATA;
		sp.Write(szb->rootdir.c_str(),
				i->first,
				v,
				NULL, /* no status info */
				1, /* overwrite */
				1 /* force_nodata */);
	}
}

void save_combined_param(double pw, TParam *p, std::vector<std::pair<time_t, std::vector<double> > >& vals, szb_buffer_t *szb, size_t pn) {
	TSaveParam sp_msw(p->GetFormulaCache()[0]);
	TSaveParam sp_lsw(p->GetFormulaCache()[1]);
	for (std::vector<std::pair<time_t, std::vector<double> > >::iterator i = vals.begin();
			i != vals.end();
			i++) {
		int v;
		unsigned int* uv = (unsigned int*) &v;
		if (!std::isnan(i->second[pn]))
			v = round(i->second[pn] * pw);
		else
			v = (SZB_FILE_NODATA << 16) | SZB_FILE_NODATA;
		sp_msw.Write(szb->rootdir.c_str(),
				i->first,
				*uv >> 16,	
				NULL, /* no status info */
				1, /* overwrite */
				1 /* force_nodata */);
		sp_lsw.Write(szb->rootdir.c_str(),
				i->first,
				*uv & 0xffff,	
				NULL, /* no status info */
				1, /* overwrite */
				1 /* force_nodata */);
	}

}

int main(int argc, char* argv[])
{
	struct arguments arguments;
	int loglevel;	/**< Log level. */
	char *ipk_prefix;
	char *szarp_data_root;
	Szbase* szbase;

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

	IPKContainer::Init(SC::A2S(szarp_data_root), SC::A2S(PREFIX), L"", new NullMutex());

	Szbase::Init(SC::A2S(szarp_data_root), NULL);
	szbase = Szbase::GetObject();

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

	std::vector<TParam*> params;
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

	lua_State* lua = Lua::GetInterpreter();

	if (arguments.formula.empty()) {
		std::wcerr << "No formula given, error" << std::endl;
		return 1;
	}

	if (szb_compile_lua_formula(lua, (const char*) SC::A2U(arguments.formula).c_str(), "", false) == false) {
		std::wcerr << "Failed to compile formula, error: " << lua_tostring(lua, -1) << std::endl;
		return 1;
	}

	std::cerr << "start time: " << format_time(arguments.start) << std::endl;
	std::cerr << "end time: " << format_time(arguments.end) << std::endl;

	std::vector<std::pair<time_t, std::vector<double> > > vals = do_run(arguments.start, arguments.end, lua, arguments.params.size());

	for (size_t i = 0; i < params.size(); i++) {
		TParam *p = params[i];
		double pw = pow(10.0, p->GetPrec());
		if (p->GetType() == TParam::P_COMBINED)
			save_combined_param(pw, p, vals, szb, i);
		else
			save_real_param(pw, p, vals, szb, i);
	}

	libpar_done();

	return 0;
}
