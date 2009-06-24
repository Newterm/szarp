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
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <time.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <stdio.h>

#include "szbase/szbdate.h"


/** arguments processing, see info argp */
#include <argp.h>

const char *argp_program_version = "szbindex ""$Revision: 4968 $";
const char *argp_program_bug_address = "coders@praterm.com.pl";
static char doc[] = "Prints index in SzarpBase file for given date.";
static char args_doc[] = "DATE";


static struct argp_option options[] = {
	{"date-format", 'D', "STRING", 0,
		"Set format for date to STRING (see 'man strptime'), default is '%Y-%m-%d %H:%M'", 
		1},
	{0}
};

struct arguments {
	char * date_format;
	char * date_string;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = (struct arguments *) state->input;

	switch (key) {
		case 'D':
			if (arguments->date_format)
				free(arguments->date_format);
			arguments->date_format = strdup(arg);
			break;
		case ARGP_KEY_ARG:
			if (arguments->date_string)
				free(arguments->date_string);
			arguments->date_string = strdup(arg);
			break;
		case ARGP_KEY_END:
			if (state->arg_num != 1) {
				fprintf(stderr, "Error: no date given.\n");
				argp_usage(state);
				return -1;
			}
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}
	
static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char* argv[])
{
	struct arguments arguments;
	time_t t;
	struct tm tm;
	int index;
	
	arguments.date_format = strdup("%Y-%m-%d %H:%M");
	arguments.date_string = NULL;
	
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	if (strptime(arguments.date_string, arguments.date_format, &tm) == NULL) {
		fprintf(stderr, "Error converting string '%s' to date.\n", arguments.date_string);
		return -1;
	};

	t = mktime(&tm);
	if (t < 0) {
		fprintf(stderr, "Error converting date to time_t value.\n");
		return -1;
	}

	index = szb_probeind(t);
	if (index < 0) {
		fprintf(stderr, "Error converting time to file index.\n");
		return -1;
	}

	printf("%d\n", index);


	return 0;
}
	
