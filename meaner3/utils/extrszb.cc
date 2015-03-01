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
 * cb3szb - program for converting new SZARP data base format to CSV
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define X_OPEN_SOURCE

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <time.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

#include <libxml/xmlwriter.h>

#include "liblog.h"
#include "libpar.h"
#include "szbase/szbbase.h"
#include "szarp_config.h"
#include "szbextr/extr.h"
#include "conversion.h"

/** arguments processing, see info argp */
#include <argp.h>

const char *argp_program_version = "extrszb ""$Revision: 6789 $";
const char *argp_program_bug_address = "coders@praterm.com.pl";
static char doc[] = "Extracts data for PARAMETERS from SzarpBase to file.\v\
Config file:\n\
Configuration options are read from file /etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg,\n\
These options from global section are mandatory:\n\
	IPK		full path to configuration file\n\
	datadir		root directory for base.\n\
Ctr-C can be used to break extraction process.\n\
";

static char args_doc[] = "[PARAMETER ...]";

static struct argp_option options[] = {
	{"debug", 1, "N", 0, "Set initial debug level to N, N is from 0 \
(error only), to 10 (extreme debug), default is 2", -2},
	{"D<param>", 0, "VALUE", 0,
		"Set initial value of libpar variable <param> to VALUE, for example -Dprefix=test sets configuration prefix to 'test'", -2},
	{"file", 'f', "PARAMS_LIST", 0, 
		"Read list of parameters from file PARAMS_LIST, 1 parameter per line", -1},
	{"start", 's', "DATE", 0,
		"Start from given DATE",
		1},
	{"end", 'e', "DATE", 0,
		"End before given DATE", 
		2},
	{"date-format", 'D', "STRING", 0,
		"Set format for for following start or end dates to STRING (see 'man strptime'), default is '%%Y-%%m-%%d %%H:%%M'", 
		3},
	{"month", 'm', "YYYYMM", 0,
		"Extract data from given month, cannot be used with -s or -e", 
		3},
	{"probe", 'p', "TYPE", 0,
		"Extract probes of given type. TYPE can be '10min', 'hour', '8hour' 'day', 'week', 'month' or length of probe in seconds",
		4},
	{"no-data", 'N', "TEXT", 0, "Set TEXT value for no-data values (default is 'NO_DATA'), ignored with -r", 4},
	{"empty", 'E', NULL, 0, "Print also empty rows (ignored by default)", 4},
	{"output", 'o', "FILE", 0,
		"Output data to FILE", 5},
	{"csv", 'c', NULL, 0,
		"Set output format to CSV (Comma Separated Values), this is default format", 6},
	{"xml", 'x', NULL, 0,
		"Set output format to XML", 6},
	{"openoffice", 'O', NULL, 0,
		"Set output format to OpenOffice spreadsheet, -o is also requiered", 6},
	{"delimiter", 'd', "TEXT", 0,
		"Use TEXT as fields delimieter, instead of comma; implies '-c'.", 6},
	{"dec-separator", 'S', "C", 0,
		"Use C as decimal separator instead of default", 6},
	{"progress", 'P', NULL, 0,
		"Print progress information on stderr", -3},
	{"sum", 'X', NULL, 0,
		"Print only sums of values from given time range, implies --csv", 7},
	{"probes-server", 'A', "ADDRESS", 0,
		"Address of probes server to fetch data from", 7},
	{"probes-server-port", 'L', "PORT", 0, "Port probes server listens on", 7},
	{0}
};

typedef struct par_list par_list;
struct par_list {
	char *name;
	par_list *next;
};

struct arguments {
	int debug_level;
	char * date_format;
	char * params_list;
	time_t start_time;
	time_t end_time;
	int year;
	int month;
	SZARP_PROBE_TYPE probe;
	int probe_length;
	int openoffice;
	int csv;
	int xml;
	char * output;
	char * delimiter;
	int progress;
	std::vector<SzbExtractor::Param> params;
	int params_count;
	par_list * list;
	char *no_data;
	char dec_sep;
	int empty;
	int sum;
	char * prober_address;
	char * prober_port;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = (struct arguments *) state->input;
	struct tm tm = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#define BUFFER_SIZE 1000
	static char buffer[BUFFER_SIZE];
	FILE *f;
	int line;

	switch (key) {
		case 'D':
			if (arguments->date_format)
				free(arguments->date_format);
			arguments->date_format = strdup(arg);
			break;
		case '1':
			arguments->debug_level = atoi(arg);
			break;
		case 'f':
			f = fopen(arg, "r");
			if (f == NULL) {
				sz_log(1, "Cannot open file '%s', errno %d",
						arg, errno);
				return errno;
			}
			line = 0;
			while (!feof(f) && !ferror(f)) {
				line++;
				if (fgets(buffer, BUFFER_SIZE, f) == NULL) {
					if (feof(f))
						break;
					sz_log(1, "Error reading from file '%s', line '%d'", 
							arg, line);
					fclose(f);
					return -1;
				};
				int i = strlen(buffer) - 1;
				if (buffer[i] == '\n')
					buffer[i] = 0;
				if (strlen(buffer) <= 0)
					continue;
				
				par_list *nl;
				nl = (par_list*) malloc (sizeof(par_list));
				assert (nl != NULL);
				
				nl->name = strdup(buffer);
				nl->next = arguments->list;
				arguments->list = nl;
				arguments->params_count++;
			}
			fclose(f);
	
			break;
		case 's':
			if (arguments->year >= 0) {
				sz_log(0, "Option -m cannot be used together with -s or -e.");
				argp_usage(state);
			}
			if (strptime(arg, arguments->date_format, &tm) == NULL) {
				sz_log(0, "Failed to parse start date '%s'", arg);
				return -1;
			}
			tm.tm_isdst = -1;
			arguments->start_time = mktime(&tm);
			break;
		case 'e':
			if (arguments->year >= 0) {
				sz_log(0, "Option -m cannot be used together with -s or -e.");
				argp_usage(state);
			}
			if (strptime(arg, arguments->date_format, &tm) == NULL) {
				sz_log(0, "Failed to parse end date '%s'", arg);
				return -1;
			}
			tm.tm_isdst = -1;
			arguments->end_time = mktime(&tm);
			break;
		case 'm':
			if ((arguments->start_time >= 0) || 
					(arguments->end_time >= 0)) {
				sz_log(0, "Option -m cannot be used together with -s or -e.");
				argp_usage(state);
			}
			if (strlen(arg) != 6) {
				sz_log(0, "Bad format for month: '%s', should be YYYYMM", arg);
				return -1;
			}
			for (int i = 0; i < 6; i++)
				if (!isdigit(arg[i])) {
					sz_log(0, "Bad format for month, digit expected, got '%c' at position %d",
							arg[i], i + 1);
					return ARGP_ERR_UNKNOWN;
				}
			arguments->year = (arg[0] - '0') * 1000 +
				(arg[1] - '0') * 100 +
				(arg[2] - '0') * 10 +
				(arg[3] - '0');
			arguments->month = (arg[4] - '0') * 10 +
				(arg[5] - '0');
			break;
		case 'p':
			if (!strcmp(arg, "10sec")) 
				arguments->probe = PT_SEC10;
			else if (!strcmp(arg, "10min")) 
				arguments->probe = PT_MIN10;
			else if (!strcmp(arg, "hour"))
				arguments->probe = PT_HOUR;
			else if (!strcmp(arg, "8hour"))
				arguments->probe = PT_HOUR8;
			else if (!strcmp(arg, "day"))
				arguments->probe = PT_DAY;
			else if (!strcmp(arg, "week"))
				arguments->probe = PT_WEEK;
			else if (!strcmp(arg, "month"))
				arguments->probe = PT_MONTH;
			else {
				arguments->probe = PT_CUSTOM;
				arguments->probe_length = atoi(arg);
				if (arguments->probe_length <= 0) {
					sz_log(0, "Bad value for probe type (%s)", arg);
					return -1;
				}
			}
			break;
		case 'N':
			free(arguments->no_data);
			arguments->no_data = strdup(arg);
			break;
		case 'E':
			arguments->empty = 1;
			break;
		case 'S' :
			if (strlen(arg) != 1) {
				sz_log(0, "Bad decimal separator '%s' - one character expected",
						arg);
				return -1;
			}
			arguments->dec_sep = arg[0];
			break;
		case 'o':
			if (arguments->output)
				free(arguments->output);
			arguments->output = strdup(arg);
			break;
		case 'c':
			if (arguments->openoffice || arguments->xml) {
				sz_log(0, "Option -c cannot be used with -x or -O");
				return -1;
			};
			arguments->csv = 1;
			break;
		case 'X':
			if (arguments->openoffice || arguments->xml) {
				sz_log(0, "Option -X cannot be used with -x or -O");
				return -1;
			};
			arguments->sum = 1;
			break;
		case 'x':
			if (arguments->openoffice || arguments->csv || arguments->sum) {
				sz_log(0, "Option -x cannot be used with -c, -d, -X or -O");
				return -1;
			};
			arguments->xml = 1;
			break;
		case 'O':
			if (arguments->xml || arguments->csv || arguments->sum) {
				sz_log(0, "Option -O cannot be used with -c, -d, -X or -x");
				return -1;
			};
			arguments->openoffice = 1;
			break;
		case 'd':
			if (arguments->openoffice || arguments->xml) {
				sz_log(0, "Option -d cannot be used with -x or -O");
				return -1;
			};
			arguments->csv = 1;
			free(arguments->delimiter);
			asprintf(&arguments->delimiter, "%s", arg);
			break;
		case 'A':
			arguments->prober_address = strdup(arg);
			sz_log(10, "Setting prober address: %s", arg);
			break;
		case 'L':
			arguments->prober_port = strdup(arg);
			break;
		case 'P':
			arguments->progress = 1;
			break;
		case ARGP_KEY_ARG:
			sz_log(3, "Adding parameter '%s'", arg);
			
			par_list *nl;
			nl = (par_list*) malloc (sizeof(par_list));
			assert (nl != NULL);
			
			nl->name = strdup(arg);
			nl->next = arguments->list;
			arguments->list = nl;

			break;
		case ARGP_KEY_END:
			arguments->params_count += state->arg_num;
			if (arguments->params_count < 1) {
				sz_log(0, "Error: no parameters to extract");
				argp_usage(state);
				return -1;
			}

			for (int i = 1; i <= arguments->params_count; i++) {
				assert (arguments->list != NULL);
				par_list *tmp = arguments->list->next;
				arguments->params.push_back(
					SzbExtractor::Param(
						SC::L2S(arguments->list->name).c_str(),
						L"",
						NULL,
						SzbExtractor::TYPE_AVERAGE ));
				free(arguments->list);
				arguments->list = tmp;
			}
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}


static struct argp argp = { options, parse_opt, args_doc, doc };

void *print_progress(int progress, void *data)
{
	if (progress <= 100)
		fprintf(stderr, "   %02d%% extracted\r", progress);
	else switch (progress) {
		case 101 :
			fprintf(stderr, "   applying XSL transformation\r");
			break;
		case 102 :
			fprintf(stderr, "   compressing output file    \r");
			break;
		case 103 :
			fprintf(stderr, "   dumping data to output     \r");
			break;
		case 104:
			fprintf(stderr, "   Completed                  \r");
			break;
		default :
			fprintf(stderr, "   no idea what is going on...\r");
			break;
	}
	return NULL;
}

/* signal handling */

int cancel_value = 0;	/**< global cancel-control value */

/** interrupt handler, sets cancel_value to 1 */
RETSIGTYPE int_handler(int signum)
{
	cancel_value = 1;
}

/** sets signals handling */
void set_int_handler(void)
{
	int ret;
	struct sigaction sa;
	sigset_t block_mask;
	
	sigemptyset(&block_mask);
	sigaddset(&block_mask, SIGTERM);
	sigaddset(&block_mask, SIGINT);
	sa.sa_mask = block_mask;
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = int_handler;
	ret = sigaction(SIGTERM, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGINT, &sa, NULL);
	assert (ret == 0);
}

int main(int argc, char* argv[])
{
	struct arguments arguments;
	char *ipk_prefix;
	char *szarp_data_root;
	error_t err;

	arguments.params_count = 0;
	arguments.list = NULL;
	arguments.debug_level = 2;
	arguments.date_format = strdup("%Y-%m-%d %H:%M");
	arguments.params_list = NULL;
	arguments.start_time = -1;
	arguments.end_time = -1;
	arguments.year = -1;
	arguments.month = -1;
	arguments.probe = PT_MIN10;
	arguments.probe_length = 0;
	arguments.output = NULL;
	arguments.openoffice = 0;
	arguments.csv= 0;
	arguments.xml = 0;
	arguments.delimiter = strdup(", ");
	arguments.progress = 0;
	arguments.no_data = strdup("NO_DATA");
	arguments.dec_sep = 0;
	arguments.empty = 0;
	arguments.sum = 0;
	arguments.prober_address = NULL;
	arguments.prober_port = NULL;

	setbuf(stdout, 0);
	
	/* Set logging. */
	loginit_cmdline(2, NULL, &argc, argv);
	/* Turn off logging of additional info. */
	log_info(0);
	/* parse libpar command line arguments */
	libpar_read_cmdline(&argc, argv);

	err = argp_parse(&argp, argc, argv, 0, 0, &arguments);
	if (err != 0)
		return err;
	if (arguments.openoffice && !arguments.output) {
		sz_log(0, "Option -O requires also -o.");
		return 1;
	}
	if (arguments.year < 0) {
		/* check start and end date */
		if ((arguments.start_time < 0) || (arguments.end_time < 0)) {
			sz_log(0, "Option -m or both -s and -e are required");
			return 1;
		}
	}
	
	/* Load configuration data. */
	libpar_init();
	szarp_data_root = libpar_getpar("", "szarp_data_root", 1);
	assert(szarp_data_root != NULL);
	ipk_prefix = libpar_getpar("", "config_prefix", 1);
	assert (ipk_prefix != NULL);
	libpar_done();

	LIBXML_TEST_VERSION
	xmlSubstituteEntitiesDefault(1);

	IPKContainer::Init(SC::L2S(szarp_data_root), SC::L2S(PREFIX), L"");
	Szbase::Init(SC::L2S(szarp_data_root), NULL);
	
	TSzarpConfig *ipk = IPKContainer::GetObject()->GetConfig(SC::L2S(ipk_prefix));
	if (ipk == NULL) {
		sz_log(0, "Could not load IPK configuration for prefix '%s'", ipk_prefix);
		return 1;
	}

	if (arguments.prober_address) {
		sz_log(0, "Prober address: %s", arguments.prober_address);
		Szbase::GetObject()->SetProberAddress(SC::L2S(ipk_prefix),
			SC::L2S(arguments.prober_address),
			arguments.prober_port ? SC::L2S(arguments.prober_port) : L"8090");
	}

	szb_buffer_t *szb = Szbase::GetObject()->GetBuffer(SC::L2S(ipk_prefix));
	if (szb == NULL) {
		sz_log(0, "Error initializing SzarpBase buffer");
		return 1;
	}


	SzbExtractor * extr = new SzbExtractor(Szbase::GetObject());

	for (int i = 0; i < arguments.params_count; i++) {
		arguments.params[i].szb = szb;
		arguments.params[i].prefix = ipk->GetPrefix();
	}
	
	assert (extr != NULL);
	if (arguments.year < 0)
		extr->SetPeriod(arguments.probe, arguments.start_time,
				arguments.end_time, arguments.probe_length);
	else
		extr->SetMonth(arguments.probe, arguments.year, 
				arguments.month, arguments.probe_length);
	if ((err = extr->SetParams(arguments.params))
			> 0) {
		sz_log(0, "Parameter with name '%s' can not be read", 
				SC::U2A(SC::S2U(arguments.params[err - 1].name)).c_str());
		return 1;
	}
	extr->SetNoDataString(SC::L2S(arguments.no_data));
	extr->SetEmpty(arguments.empty);
	
	FILE *output = NULL;
	if (!arguments.openoffice) {
		if (arguments.output != NULL) {
			output = fopen(arguments.output, "w");
			if (output == NULL) {
				sz_log(0, "Error creating output file '%s', errno %d",
						arguments.output, errno);
				return 1;
			}
		} else
			output = stdout;
	}
	
	if (arguments.progress) {
		extr->SetProgressWatcher(print_progress, NULL);
	}

	if (arguments.dec_sep) {
		extr->SetDecDelim(arguments.dec_sep);
	}

	/* cancel at Ctrl-C */
	extr->SetCancelValue(&cancel_value);
	set_int_handler();

	SzbExtractor::ErrorCode ret;
	
	if (arguments.openoffice)
		ret = extr->ExtractToOpenOffice(SC::L2S(arguments.output));
	else if (arguments.xml)
		ret = extr->ExtractToXML(output);
	else if (arguments.sum)
		ret = extr->ExtractSum(output, SC::L2S(arguments.delimiter));
	else
		ret = extr->ExtractToCSV(output, SC::L2S(arguments.delimiter));
	if (arguments.progress)
		fprintf(stderr, "\n");
	
	switch (ret) {
		case SzbExtractor::ERR_OK :
			break;
		case SzbExtractor::ERR_NOIMPL :
			sz_log(0, "Error: function not implemented");
			break;
		case SzbExtractor::ERR_CANCEL :
			sz_log(0, "Error: extraction canceled by user");
			break;
		case SzbExtractor::ERR_XMLWRITE :
			sz_log(0, "Error: XMLWriter error, program or libxml2 bug");
			break;
		case SzbExtractor::ERR_SYSERR :
			perror("Error: system error");
			break;
		case SzbExtractor::ERR_ZIPCREATE :
			sz_log(0, "Error: cannot create zip file");
			break;
		case SzbExtractor::ERR_ZIPADD :
			sz_log(0, "Error: error adding content to zip file");
			break;
		case SzbExtractor::ERR_LOADXSL :
			sz_log(0, "Error: error loading XSLT stylesheet");
			break;
		case SzbExtractor::ERR_TRANSFORM :
			sz_log(0, "Error: error applying XSL stylesheet");
			break;
		default:
			sz_log(0, "Error: unknown error (bug?)");
			break;
	}
	
	return (ret != SzbExtractor::ERR_OK);
}
	
