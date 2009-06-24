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
 * SZARP - xgrep2

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * $Id$
 *
 * Program do wyszukiwania informacji w plikach Praterm News. Wersja z 
 * koszeniem trawy i podlewaniem kwiatków...
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <dirent.h>

#include <string>
#include <vector>

#include "tokens.h"
#include "convuri.h"

#ifndef min
#define min(a, b) ((a < b) ? (a) : (b))
#endif

#define PRATERM_NEWS_PATH	"/var/www/praterm-news-stripped"

#define COLOR_INFO "magenta"
#define COLOR_KSIEGOWOSC "#a188ae"
#define COLOR_CIEPLO "green"
#define COLOR_CIEKAWOSTKI "#427798"
#define COLOR_PRIV "gray"
#define COLOR_PRATERM "#aa8800"
#define COLOR_OTHER "purple"

using namespace std;

/** Possible types of expresions to match (used to modify HTML output). */
typedef enum { REGEX_MAIL, REGEX_HTTP, REGEX_WWW, REGEX_PN, REGEX_BILL_NUM, 
	REGEX_BILL_YEAR, REGEX_BILL_DATE, REGEX_IMAGE, REGEX_LAST } regex_type_t;
/** Array of compiled expresions. */
regex_t regexs[REGEX_LAST];

struct filter_struct;
/** Types of lines scanning functions. */
typedef int (search_fun_t)(char *str, struct filter_struct* filter_data);

/** Structure describing content filter */
typedef struct filter_struct {
	int enough;		/* is filter enough to accept line */
	int negate;		/* 1 to negate result of filter */
	int showhidden;		/* 1 to include hidden data in search */
	int onlyhidden;		/* 1 to search only in hidden data */
	char *phrase;		/* phrase to search for (may be NULL) */
	char *tag;		/* tag to search in (may be NULL) */
	char *priv;		/* type of priv tag to search in (TO attribute, may be NULL) */
	int alltags;		/* 1 to accept all tags, including priv */
	int allpriv;		/* 1 to accept all priv tags */
	int passed;		/* field for marking that some line passed filter */
	int is_regex;		/* is filter a regex filter */
	regex_t regex;		/* compiled regexp, unitialized if filter uses
				simple text */
	search_fun_t* search_fun;
				/* function used to scan lines */
} filter_t;

/** Possible output types */
typedef enum { OUTPUT_HTML, OUTPUT_CONSOLE, OUTPUT_TEXT, OUTPUT_LAST } output_t;

/** Global output state info. */
typedef struct {
	int hidden_par;		/**< are we in paragraph that should not be 
				  printed */
	int date_printed;	/**< 1 if PN date was already printed */
	int index;		/**< index of file used to create previous/next 
				  links */
	char date[9];		/**< copy of date to print */
	int pre;		/**< are we in <pre> fragment */
	int table;		/**< are we in table */
	int tag_printed;	/**< 1 if tag name was already printed (for 
				  'paragraphs mode') */
	int info_printed;	/**< 1 if file and line info was already printed */
} output_state_t;

/** Type for token matching functions */
typedef int (parse_fun_t)(const char *);

/** Type for output printing functions. */
typedef void (output_fun_t)(const char *, int);

/** Type for main grep functions. */
typedef int (grep_fun_t) (const char*, const char *);

/** Types of output helper functions. */
typedef enum { OUTPUT_H_MAIN, OUTPUT_H_END_TAG, OUTPUT_H_END_PARA, OUTPUT_H_LAST } output_helper_t;

/** Types of possible input tokens  */
typedef enum { PRIV_START = 0, TAG_END, TAG_START, HIDDEN_START, HIDDEN_LINE, EMPTY_LINE, LINE,
	TOKEN_LAST } token_t;

/** Parser states */
typedef enum { S_OUT, S_TAG, S_PAR, S_HIDDEN, S_HLINE, S_ERROR } state_t;

/* This is state transtition table description */
//STATE  tag-start  tag-end  priv-start  hidden-start  hidden-line empty-line line
//out    tag        ERROR     tag        ERROR         ERROR       out        out
//tag    ERROR      out      ERROR       hidden        hline       IGNORE     par
//par    ERROR      out      ERROR       ERROR         hline       tag        par
//hidden ERROR      out      ERROR       ERROR         ERROR       tag        hidden
//hline  ERROR      out      ERROR       ERROR         hidden      tag        par

/* GLOBAL VARIABLES */
/** State transtition array. */
state_t state_trans[S_ERROR][TOKEN_LAST] = {
	{ S_TAG, S_ERROR, S_TAG, S_ERROR, S_ERROR, S_OUT, S_OUT }, 
	{ S_ERROR, S_OUT, S_ERROR, S_HIDDEN, S_HLINE, S_TAG, S_PAR }, 
	{ S_ERROR, S_OUT, S_ERROR, S_HIDDEN, S_HLINE, S_TAG, S_PAR }, 
	{ S_ERROR, S_OUT, S_ERROR, S_ERROR, S_HLINE, S_TAG, S_PAR }, 
	{ S_ERROR, S_OUT, S_ERROR, S_HIDDEN, S_HLINE, S_TAG, S_PAR }, 
};

/**
 * FUNCTIONS FORWARDS DECLARATIONS
 */
int add_filter(char* filter);
int add_file(char* file);
int init_regex();
void clear_filters();

int search_str(char *str, filter_t* filter_data);
int search_strcase(char *str, filter_t* filter_data);
int search_strregex(char *str, filter_t* filter_data);

int is_priv_start(const char *str);
int is_tag_start(const char *str);
int is_tag_end(const char *str);
int is_hidden(const char *str);
int parse_line(const char *str);
int is_hidden_line(const char *str);
int is_empty_line(const char *str);

void print_n(const char *str, int n);
void print_none(const char *, int);

void regex_output_pn(const char *str, int len);
void regex_output_bill_num(const char *str, int len);
void regex_output_bill(const char *str, int len);
void regex_output_image(const char *str, int len);
void regex_output_link(const char *str, int len);
void regex_output_mail(const char *str, int len);
void regex_output_link_www(const char *str, int len);
void regex_output_filter(const char *str, int len);
void regex_output_console(const char *str, int len);

void out_html_tag(const char *str, int len);
void out_html_tag_end(const char *str, int len);
void out_html_hidden(const char *str, int len);
void out_html_empty(const char *str, int len);
void out_text_empty(const char *str, int len);
void out_html_char(char c);
void out_html_text(const char *str, int len);
int check_table_start(const char *str);
int check_table_in(const char *str);
void out_html_line(const char *str, int len);
void out_html_hline(const char *str, int len);

void html_output_end_para(const char *, int);
void html_output_end_tag(const char *, int);
void html_output(const char *str, int len);

void out_console_hidden(const char *str, int len);
void out_console_line(const char *str, int len);
void out_console_hline(const char *str, int len);
void out_console_tag(const char *str, int len);
void out_console_tag(const char *str, int len);

void console_output_end_para(const char*, int);
void console_output(const char* str, int len);

void out_text_tag(const char *str, int len);
void out_text_hidden(const char *str, int len);
void out_text_line(const char *str, int len);
void out_text_hline(const char *str, int len);

void text_output_end_para(const char*, int);
void text_output(const char* str, int len);

int grep_file(const char *str, const char* filename);
int grep_file_html(const char *str, const char* filename);

/**
 * GLOBAL VARIABLES
 */

/** output process state */
output_state_t output_state = { 0, 0, 0, "", 0, 0, 0, 0 };

/** vector for storing names of files to process */
vector<string> filenames;

/** vector for string filters */
vector<filter_t> filters;

#define MAX_TAG_LENGTH 30
char tag_name[MAX_TAG_LENGTH+1];	/**< name of current tag */
#define MAX_PRIV_LENGTH 8
char priv_name[MAX_PRIV_LENGTH+1];	/**< type of current priv, empty for tags */
const char* cur_start;			/**< pointer to start of current parse unit (tag
					 or paragraph) */
int g_line = 0;				/**< current line number */
int g_start_line = 0;			/**< line number of current unit start */
const char* g_filename = NULL;		/**< path to currently searched file */
int g_return_code = 1;			/**< global return code */
int g_cgi = 0;				/**< CGI mode marker */
const size_t MAX_CGI_DATA = 100000;	/**< Maximum amount of data (in bytes) 
					  printed in CGI mode */
size_t g_printed_data = 0;		/**< Current amount of data (in bytes)
					  printed in CGI mode */

/** Parsed CGI query info */
const char *g_cgi_start = "", *g_cgi_stop = "", *g_cgi_search = "", *g_cgi_tags = "";
int g_cgi_pars = 0;
int g_cgi_icase = 0;
int g_cgi_ormode = 0;

#define G_REGMATCH_SIZE	5
/**< Global arrays for storing results of regex matching. */
regmatch_t g_regmatch[G_REGMATCH_SIZE];
regmatch_t g_tmp[G_REGMATCH_SIZE];

/* Array of token parsing funtions to try (order corresponds to token_t type) */ 
parse_fun_t* parse_functions[] = {
	is_priv_start,
	is_tag_end,
	is_tag_start,
	is_hidden,
	is_hidden_line,
	is_empty_line,
	parse_line
};

/** Array of HTML output functions for different types of matched regular
 * expresions. This functions takes two arguments - pointer to start of line
 * (position of matched string is stored in global g_regmatch variable);
 * second argument is ignored.
 */
output_fun_t* regex_output[REGEX_LAST+1] = {
	regex_output_mail,
	regex_output_link,
	regex_output_link_www,
	regex_output_pn,
	regex_output_bill_num,
	regex_output_bill,
	regex_output_bill,
	regex_output_image,
	regex_output_filter,
};

/** Array of output funtions to try (order corresponds to token_t type) */ 
output_fun_t* output_functions[OUTPUT_LAST][TOKEN_LAST] = {
	{ /* HTML */
		out_html_tag,
		out_html_tag_end,
		out_html_tag,
		out_html_hidden,
		out_html_hline,
		out_html_empty,
		out_html_line,
	}, 
	{ /* CONSOLE */
		out_console_tag,
		out_console_tag,
		out_console_tag,
		out_console_hidden,
		out_console_hline,
		out_text_empty,
		out_console_line,
	},
	{ /* TEXT */
		out_text_tag,
		out_text_tag,
		out_text_tag,
		out_text_hidden,
		out_text_hline,
		out_text_empty,
		out_text_line,
	},
};

/** Array of main grep functions for each kind of output */
grep_fun_t* grep_functions[OUTPUT_LAST] = {
	grep_file_html,		/* HTML*/
	grep_file,		/* CONSOLE */
	grep_file,		/* TEXT */
};

/** Array of output helpers function for each kind of output type and
 * helper function type. */
output_fun_t* output_helpers[OUTPUT_LAST][OUTPUT_H_LAST] = {
	{	/* HTML */
		html_output,
		html_output_end_tag,
		html_output_end_para,
	},	
	{	/* CONSOLE */
		console_output,
		print_none,
		console_output_end_para,
	},
	{	/* TEXT */
		text_output,
		print_none,
		text_output_end_para,
	},
};

/**
 * ARGUMENTS HANDLING
 */

typedef enum { PARSE_TAGS, PARSE_LINES, PARSE_PARAGRAPHS } parse_type_t;

#include <argp.h>
/**< argp arguments struct */
struct arguments {
	parse_type_t parse_type;	/**< should we parse whole tags or every paragraph or every line */
	output_t output;		/**< type of output */
	int show_hidden;		/**< show hidden lines and paragraphs in output */
	int noline;			/**< skip line and file info in console or text output or date in HTML mode */
} arguments = { PARSE_TAGS, OUTPUT_CONSOLE, 0, 0 };

/** argp documentation info */
const char *argp_program_version = "xgrep2 ""$Revision: 6789 $";
const char *argp_program_bug_address = "coders@praterm.com.pl";
static char doc[] = "Parse and search Praterm-News files.\v\
Filters have format /PHRASE/TAG/FLAGS where:\n\
/\tcan be replaced by any single character that does not appear in other parts\n\
\tof filter\n\
PHRASE\tis phrase to search for.\n\
TAG\tis name of tag to search within, you can use:\n\
\tempty tag to search within all tags,\n\
\t!ID to search PRIV tags with TO=\"ID\",\n\
\t! to search all PRIV tags,\n\
\t* to search all tags including PRIV.\n\
FLAGS\tare optional and are composed of letters:\n\
\tt to use simple text searching, not regular expresions,\n\
\ti to ignore case in phrase searching,\n\
\th to search also in hidden lines and paragraphs,\n\
\tH to search only in hidden lines and paragraphs,\n\
\tn to negate result of filter,\n\
\tE to create 'enough' filter - ends a sequence of 'AND' filters and starts\n\
\ta new sequence, OR'ed with previous; filters are applied in order of \n\
\tappearance.\n\
\n\
Examples:\n\
Search for 'sprawnosc' but not 'elektryczna' in tag 'ZAMOSC', print\n\
paragraphs, not tags:\n\
# ./xgrep2 -P -f '/sprawnosc/ZAMOSC/i' -f '/elektryczna//in' *.txt\n\
Display lines that contains ('Linux' and 'Debian') or 'RedHat':\n\
# ./xgrep2 -L -f '/Debian//i' -f '/Linux//iE' -f '/redhat//i' *.txt\n\
\n\
If QUERY_STRING environment variable is defined, program ignores all arguments\n\
and acts like a CGI script, using 'search', 'tags', 'start', 'stop', 'pars',\n\
'icase' and 'ormode' GET variables.\n\
";
static char args_doc[] = "[ FILES ...]";
static struct argp_option options[] = {
	{ "paragraphs", 'P', 0, 0, "Search and display single paragraphs, not whole tags", 1 },
	{ "lines", 'L', 0, 0, "Search and display single lines, not tags or paragraphs", 1 },
	{ "filter", 'f', "FILTER", 0, "Add specified filter, see filters description below.", 1 },
	{ "hidden", 'H', 0, 0, "Show hidden lines and paragraphs in output", 1 },
	{ "web", 'w', 0, 0, "Web mode - output in HTML format, filters are applied to all\
 tags/paragraphs and not single lines, hidden data is not recognized", 2 },
	{ "text", 't', 0, 0, "Clear text output (default is with console control sequences)", 2 },
	{ "no-info", 'n', 0, 0, "Do not print file and line number info in console or clear text mode", 3 },
	{ 0 }
};

/** argp parser function
 */
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
        struct arguments *arguments = (struct arguments *) state->input;
	switch (key) {
		case 'P' : 
			arguments->parse_type = PARSE_PARAGRAPHS;
			break;
		case 'L' :
			arguments->parse_type = PARSE_LINES;
			break;
		case 'f' :
			if (add_filter(arg)) {
				return -1;
			}
			break;
		case 't' :
			arguments->output = OUTPUT_TEXT;
			break;
		case 'w' :
			arguments->output = OUTPUT_HTML;
			break;
		case 'H' :
			arguments->show_hidden = 1;
			break;
		case 'n' :
			arguments->noline = 1;
			break;
		case ARGP_KEY_ARG :
			if (add_file(arg)) {
				argp_error(state, "Problem processing file name '%s'",
						arg);
				return -1;
			}
			break;
		case ARGP_KEY_END :
			if (arguments->output == OUTPUT_HTML
					&& arguments->parse_type == PARSE_LINES) {
				argp_error(state, "Cannot mix --lines and --web options.");
				
			}
			if (filenames.size() <= 0) {
				argp_usage(state);
				return -1;
			}
		default :
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/** argp parameters */
static struct argp argp = { options, parse_opt, args_doc, doc };

/** Append file name to vector of files to process */
int add_file(char* file)
{
	filenames.push_back(string(file));
	return 0;
}

/**
 * FUNCTIONS CHECKING INPUT TOKENS */

/**
 * @return if string matches tag, returns length of matching token, 0 if does not match
 */
int is_priv_start(const char *str)
{
	char *c;
	if (strncmp("<PRIV TO=\"", str, 10))
		return 0;
	c = index(str + 10, '"');
	if (!c)
		return 0;
	if (c - str <= 1) 
		return 0;
	if (c - str > (10 + MAX_PRIV_LENGTH))
		return 0;
	strncpy(priv_name, str + 10, c - str);
	tag_name[0] = 0;
	for (const char *tmp = str + 10; tmp < c; tmp++) {
		if (!isalpha(*tmp))
			return 0;
	}
	while (isalnum(*c) || (*c == ' ') || (*c == '"') || (*c == '='))
		c++;
	if (*c != '>') {
		fprintf(stderr, "UKNOWN CHAR '%c' IN PRIV TAG START\n", *c);
		return 0;
	}

	if (*(c+1) == '\n')
		return (c - str) + 1;
	if (*(c+1) != '\r')
		return 0;
	if (*(c+2) == '\n')
		return (c - str) + 2;
	return 0;
}

/**
 * @return if string matches tag, returns length of matching token, 0 if does not match
 */
int is_tag_start(const char *str)
{
	const char *c = str;
	if (*c != '<') return 0;
	c++;
	while (1) {
		if (isupper(*c) || (*c == ' ') || (*c == '-')) {
			c++;
		} else if (*c == '>') {
			strncpy(tag_name, str + 1, min(MAX_TAG_LENGTH, c - str - 1));
			tag_name[min(MAX_TAG_LENGTH, c - str - 1)] = 0;
			priv_name[0] = 0;
			if (c - str > 2) {
				c++;
				if (*c == '\n')
					return c - str + 1;
				if (*c != '\r')
					return 0;
				c++;
				if (*c == '\n')
					return c - str + 1;
				return 0;
			} else {
				return 0;
			}
		} else {
			return 0;
		}
	}
}

/**
 * @return if string matches tag, returns length of matching token, 0 if does not match
 */
int is_tag_end(const char *str)
{
	const char *c = str;
	if (*c != '<') return 0;
	c++;
	if (*c != '/') return 0;
	c++;
	while (1) {
		if (isupper(*c) || (*c == ' ') || (*c == '-')) {
			c++;
		} else if (*c == '>') {
			if (c - str > 2) {
				c++;
				if (*c == '\n')
					return c - str + 1;
				if (*c != '\r')
					return 0;
				c++;
				if (*c == '\n')
					return c - str + 1;
				return 0;
			} else {
				return 0;
			}
		} else {
			return 0;
		}
	}
}

/**
 * @return if string matches tag, returns length of matching token, 0 if does not match
 */
int is_hidden(const char *str)
{
	if (strncmp("<UWAGA!!!>", str, 10))
		return 0;
	if (str[10] == '\n')
		return 11;
	if (str[10] != '\r')
		return 0;
	if (str[11] == '\n')
		return 12;
	return 0;
}

/**
 * @return if string matches tag, returns length of matching token, 0 if does not match
 */
int parse_line(const char *str)
{
	int ret = 0;
	while ((*str != '\r') && (*str != '\n') && (*str != 0)) {
		ret++;
		str++;
	}
	if (*str == '\r') {
		str++;
		ret++;
		if (*str != '\n')
			return -1;
	}
	return ret+1;
}

/**
 * @return if string matches tag, returns length of matching token, 0 if does not match
 */
int is_hidden_line(const char *str)
{
	if (strncmp("<!!!>", str, 5)) {
		return 0;
	}
	return parse_line(str);
}

/**
 * @return if string matches tag, returns length of matching token, 0 if does not match
 */
int is_empty_line(const char *str)
{
	if (str[0] == '\n')
		return 1;
	if ((str[0] == '\r') && (str[1] == '\n'))
		return 2;
	return 0;
}

/**
 * FILTERS HANDLING
 */

/** Parse filter and add to global vector of filters.
 * @param filter text description of filters
 * @return 0 on success, -1 on error (error description is printed on stderr)
 */
int add_filter(char* filter)
{
	char d;
	char *c, *c2, *last;
	int l;
	filter_t f = { 0, 0, 0, 0, NULL, NULL, NULL, 0, 0, 0 };
	int text_search = 0;
	int igncase = 0;

	assert (filter != NULL);
	l = strlen(filter);
	last = filter + l;
	if (l < 3) {
		fprintf(stderr, "Incorrect filter (to short): %s\n", filter);
		return -1;
	}
	d = filter[0];
	c = index(filter + 1, d);
	if (c == NULL) {
		fprintf(stderr, "Incorrect filter (no delimiter after phrase): %s\n", filter);
		return -1;
	}
	if (c - filter > 1) {
		*c = 0;
		f.phrase = strdup(filter + 1);
	}
	*c = d;
	c2 = index(c+1, d);
	if (c2 == NULL) {
		fprintf(stderr, "Incorrect filter (no delimiter after tag name): %s\n", filter);
		return -1;
	}
	if (c2 - c > 1) {
		*c2 = 0;
		if (c[1] == '!') {
			if (c[2] == 0) {
				f.allpriv = 1;
			} else {
				if (strlen(c+2) > MAX_PRIV_LENGTH) {
					*c2 = d;
					fprintf(stderr, "Incorrect filter (priv tag id to long): %s\n", filter);
					return -1;
				}
				for (char *tmp = c + 2; *tmp; tmp++)
					if (!isalpha(*tmp)) {
						*c2 = d;
						fprintf(stderr, "Incorrect filter (incorrect priv tag id): %s\n", filter);
						return -1;
					}
				f.priv = strdup(c + 2);
			}
		} else if ((c[1] == '*') && (c[2] == 0)) {
			f.alltags = 1;
		} else {
			f.tag = strdup(c + 1);
			for (c = c + 1; c < c2; c++) {
				if (!isupper(*c) && (*c != ' ') && (*c != '-')) {
					*c2 = d;
					fprintf(stderr, "Incorrect filter (incorrect characters in tag name): %s\n", filter);
					return -1;
				}
			}
		}
		*c2 = d;
	}
	for (c = c2 + 1; *c != 0; c++) {
		switch (*c) {
			case 'i' :
				igncase = 1;
				break;
			case 'h' :
				f.showhidden = 1;
				break;
			case 'H' :
				f.onlyhidden = 1;
				f.showhidden = 1;
				break;
			case 'n' :
				f.negate = 1;
				break;
			case 'E' :
				f.enough = 1;
				break;
			case 't' :
				text_search = 1;
				break;
			default :
				fprintf(stderr, "Incorrect filter (unknown flag '%c'): %s\n", 
						*c, filter);
				return -1;
				break;
		}
	}

	if (text_search) {
		f.is_regex = 0;
		if (igncase)
			f.search_fun = search_strcase;
		else
			f.search_fun = search_str;
	} else if (f.phrase) {
		int ret = regcomp(&f.regex, f.phrase, REG_EXTENDED |
				REG_NEWLINE | (igncase ? REG_ICASE : 0));
		if (ret != 0) {
#define ERROR_LENGTH 200
			char error[ERROR_LENGTH+1];
			regerror(ret, &f.regex, error, ERROR_LENGTH);
			fprintf(stderr, "Error compiling regular expresion for	filter %s: %s\n",
					filter, error);
			exit(1);
		};
		f.is_regex = 1;
		f.search_fun = search_strregex;
	}
	filters.push_back(f);
	return 0;
}

/* Clear 'passed' field of filters. */
void clear_filters()
{
	size_t i;
	for (i = 0; i < filters.size(); i++) {
		filters[i].passed = filters[i].negate;
	}
}

/** @return 1 if filter matchs given line, 0 otherwise */
int try_filter(filter_t *f, char *str, int hidden) 
{
	int ret;
	/* check for hidden */
	if (hidden) {
		if (f->showhidden == 0) {
			return 0;
		}
	} else {
		if (f->onlyhidden == 1) {
			return 0;
		}
	}
	/* check for tag */
	if (tag_name[0] != 0) {
		if (((f->tag != NULL) && strcmp(tag_name, f->tag))
				|| f->allpriv)
			return 0;
	} else if (priv_name[0] != 0) {
		if (f->priv != NULL) {
			if (strcmp(priv_name, f->priv))
				return 0;
		} else {
			if (!f->allpriv && !f->alltags)
				return 0;
		}
					
	}
	/* check for phrase */
	if (f->phrase != NULL) {
		ret = f->search_fun(str, f);
	} else {
		ret = 1;
	}
	if (ret) {
		f->passed = ! f->negate;
	}
	return (f->passed);
}

/**
 * Loops over all filters and applies them to given text (line, usually).
 * @param start pointer to start of search line
 * @param end pointer to end of search line
 * @param hidden 1 for hidden data, otherwise 0
 */
void filter_line(char *start, char *end, int hidden)
{
	char c = *end;
	
	/* break string at the end of searched line */
	*end = 0;
	for (size_t i = 0; i < filters.size(); i++) {
		/* perhaps we already passed this filter */
		if (!(filters[i].passed ^ filters[i].negate)) {
			try_filter(&filters[i], start, hidden);
		}  
	}
	/* restore previous char at the end of line */
	*end = c;
}

/**
 * Checks if filters accepted input.
 * @return 1 if current combination of filters states accept input, 0 otherwise
 */
int check_filters()
{
	int ok = 1;
	if (filters.size() == 0) 
		return priv_name[0] == 0;
	for (size_t i = 0; i < filters.size(); i++) {
		if (!filters[i].passed)
			ok = 0;
		if (filters[i].enough) {
			if (ok)
				return 1;
			if ((i+1) < filters.size())
				ok = 1;
		}
	}
	if (ok)
		g_return_code = 0;
	return ok;
}

int search_str(char *str, filter_t* filter_data)
{
	return strstr(str, filter_data->phrase) != NULL;
}

int search_strcase(char *str, filter_t* filter_data)
{
	return strcasestr(str, filter_data->phrase) != NULL;
}

int search_strregex(char *str, filter_t* filter_data)
{
	return regexec(&filter_data->regex, str, 0, NULL, 0) == 0;
}


/**
 * REGEX FUNCTIONS
 */

/** Compilation of expresions. Compiled expresions are stored in regexs array.
 * @return index of expresion type that failed to compile (this always mean bug
 * in expresion - so it is not really a runtime error).
 */
int init_regex()
{
	if (regcomp(&regexs[REGEX_MAIL], "[-a-zA-Z0-9_+.]+@([a-zA-Z0-9][-a-zA-Z0-9.+]+\\.[a-zA-Z]{2,})",
			REG_EXTENDED | REG_NEWLINE))
		return REGEX_MAIL;
	if (regcomp(&regexs[REGEX_HTTP], "(https|http|ftp)://[-a-zA-Z0-9_/.:,+~%#?@;&=]*[a-zA-Z0-9_/+~%#?@;&=]",
			REG_EXTENDED | REG_NEWLINE))
		return REGEX_HTTP;
	if (regcomp(&regexs[REGEX_WWW], "www\\.[^.]+\\.[^ ]*",
			REG_EXTENDED | REG_NEWLINE))
		return REGEX_WWW;
	if (regcomp(&regexs[REGEX_PN], "Praterm news ([1-2][0-9]{3})\\.([0-9]{2})\\.([0-9]{2})",
			REG_EXTENDED | REG_NEWLINE))
		return REGEX_PN;
	if (regcomp(&regexs[REGEX_BILL_NUM], "Dz\\.U\\.[ ]*Nr[\\.]?[ ]*([0-9]+)[,]?[ ]+poz[\\.]?[ ]*([0-9]+)",
			REG_EXTENDED | REG_ICASE | REG_NEWLINE))
		return REGEX_BILL_NUM;
	if (regcomp(&regexs[REGEX_BILL_YEAR], "Dz\\.U\\.[ ]*([0-9]{4})[ ]+nr[ ]*([0-9]+)[ ]+poz[\\.]?[ ]*([0-9]+)",
			REG_EXTENDED | REG_ICASE | REG_NEWLINE))
		return REGEX_BILL_YEAR;
	if (regcomp(&regexs[REGEX_BILL_DATE], "Dz\\.U\\.[ ]*[0-9]{2}[. ]+[0-9]{2}[. ]+([0-9]{4})[ ]+nr[ ]*([0-9]+)[ ]+poz[\\.]?[ ]*([0-9]+)",
			REG_EXTENDED | REG_ICASE | REG_NEWLINE))
		return REGEX_BILL_DATE;
	if (regcomp(&regexs[REGEX_IMAGE], "<IMG:([^>]+)>", REG_EXTENDED | REG_NEWLINE))
		return REGEX_IMAGE;
	return 0;
}

/** Searches for regex in text str. Info about matched expresion is stored in
 * global g_regmatch variable.
 * @param links 1 for matching links, 0 only for filters
 * @return type of matching expresion or REGEX_LAST if no expresion matches 
 * or -(filter_index) if some of regex filters matched
 */
int search_regex(const char *str, int links = 1)
{
	int first = REGEX_LAST;

	int i = 0;
	if (!links)
		i = -1;
	/* we try all regexps to find one that matches earlier in the line */
	for ( ; i >= - (int)filters.size(); 
			(i < 0) ? i-- : ((i < REGEX_LAST - 1) ? i++ : i = -1 )
			) {
		regex_t* rx;
		if (i >= 0) {
			rx = &regexs[i];
		} else if (!filters[-i -1].is_regex) {
			continue;
		} else {
			rx = &filters[-i -1].regex;
		}
		if (regexec(rx, str, G_REGMATCH_SIZE, g_tmp, 0) == 0) {
			if ((first == REGEX_LAST) || (g_tmp[0].rm_so < g_regmatch[0].rm_so)) {
				first = i;
				memcpy(g_regmatch, g_tmp, G_REGMATCH_SIZE * sizeof(regmatch_t));
			}
		}
	}
	return first;
}

/**
 * OUTPUT FUNCTIONS
 */

/** Most general output functions - print n chars starting from str */
void print_n(const char *str, int n)
{
	int i;
	for (i = 0; i < n; i++, str++) {
		putchar(*str);
	}
}

/** Prints nothing. */
void print_none(const char *, int)
{
}


/** REGEX HTML OUTPUT FUNCTIONS
 * Outputs HTML modified corresponding to matched expresions.
 */

void regex_output_pn(const char *str, int len)
{
	const char *year = str + g_regmatch[1].rm_so;
	const char *month = str + g_regmatch[2].rm_so;
	const char *day = str + g_regmatch[3].rm_so;
	fputs("<a href=\"http://www.szarp.com.pl/wpn/xgrep2.cgi?\
start=", stdout);
	print_n(year, 4);
	print_n(month, 2);
	print_n(day, 2);
	fputs("&stop=", stdout);
	print_n(year, 4);
	print_n(month, 2);
	print_n(day, 2);
	fputs("\">Praterm news ", stdout);
	print_n(year, 4);
	putchar('.');
	print_n(month, 2);
	putchar('.');
	print_n(day, 2);
	fputs("</a>", stdout);
}

void regex_output_bill_num(const char *str, int len)
{
	fputs("<a href=\"http://isip.sejm.gov.pl/   >servlet/Search?adresNr=", stdout);
	print_n(str + g_regmatch[1].rm_so, g_regmatch[1].rm_eo - g_regmatch[1].rm_so);
	fputs("&adresPoz=", stdout);
	print_n(str + g_regmatch[2].rm_so, g_regmatch[2].rm_eo - g_regmatch[2].rm_so);
	fputs("&adresWyd=WDU\">Dz.U. nr ", stdout);
	print_n(str + g_regmatch[1].rm_so, g_regmatch[1].rm_eo - g_regmatch[1].rm_so);
	fputs(" poz. ", stdout);
	print_n(str + g_regmatch[2].rm_so, g_regmatch[2].rm_eo - g_regmatch[2].rm_so);
	fputs("</a>", stdout);
}

void regex_output_bill(const char *str, int len)
{
	fputs("<a href=\"http://isip.sejm.gov.pl/   >servlet/Search?adresRR=", stdout);
	print_n(str + g_regmatch[1].rm_so, g_regmatch[1].rm_eo - g_regmatch[1].rm_so);
	fputs("&adresNr=", stdout);
	print_n(str + g_regmatch[2].rm_so, g_regmatch[2].rm_eo - g_regmatch[2].rm_so);
	fputs("&adresPoz=", stdout);
	print_n(str + g_regmatch[3].rm_so, g_regmatch[3].rm_eo - g_regmatch[3].rm_so);
	fputs("&adresWyd=WDU\">Dz.U. ", stdout);
	print_n(str + g_regmatch[1].rm_so, g_regmatch[1].rm_eo - g_regmatch[1].rm_so);
	fputs(" nr ", stdout);
	print_n(str + g_regmatch[2].rm_so, g_regmatch[2].rm_eo - g_regmatch[2].rm_so);
	fputs(" poz. ", stdout);
	print_n(str + g_regmatch[3].rm_so, g_regmatch[3].rm_eo - g_regmatch[3].rm_so);
	fputs("</a>", stdout);
}

void regex_output_image(const char *str, int len)
{
	printf("<img src=\"/wpn-img/%c%c%c%c%c%c%c%c/",  
				output_state.date[0],
				output_state.date[1],
				output_state.date[2],
				output_state.date[3],
				output_state.date[4],
				output_state.date[5],
				output_state.date[6],
				output_state.date[7]
				);
	print_n(str + g_regmatch[1].rm_so, g_regmatch[1].rm_eo - g_regmatch[1].rm_so);
	fputs("\"/>", stdout);
}

void regex_output_link(const char *str, int len)
{
	const char* start = str + g_regmatch[0].rm_so;
	len = g_regmatch[0].rm_eo - g_regmatch[0].rm_so;
	fputs("<a href=\"", stdout);
	print_n(start, len);
	fputs("\">", stdout);
	out_html_text(start, len);
	fputs("</a>", stdout);
}

void regex_output_mail(const char *str, int len)
{
	const char* start = str + g_regmatch[0].rm_so;
	len = g_regmatch[0].rm_eo - g_regmatch[0].rm_so;
	fputs("<a href=\"mailto:", stdout);
	print_n(start, len);
	fputs("\">", stdout);
	out_html_text(start, len);
	fputs("</a>", stdout);
}
void regex_output_link_www(const char *str, int len)
{
	const char* start = str + g_regmatch[0].rm_so;
	len = g_regmatch[0].rm_eo - g_regmatch[0].rm_so;
	fputs("<a href=\"http://", stdout);
	print_n(start, len);
	fputs("\">", stdout);
	out_html_text(start, len);
	fputs("</a>", stdout);
}
void regex_output_filter(const char *str, int len)
{
	fputs("<span style=\"color: red;\">", stdout);
	for (int i = g_regmatch[0].rm_so; i < g_regmatch[0].rm_eo; i++) {
		out_html_char(str[i]);
	}
	fputs("</span>", stdout);
}

/** HTML OUTPUT FUNCTIONS */

/** @return 1 if str is name of Praterm holding member */
int is_praterm(const char* str)
{
	const char *names[] = {
		"BIOENERGIA",
		"BYTOW",
		"CHRZANOW",
		"DOBRE MIASTO",
		"GNIEW",
		"KRASNIK",
		"LIBIAZ",
		"LIDZBARK WARMINSKI",
		"MIEDZYRZEC PODLASKI",
		"MODLIN",
		"PASLEK",
		"POREBA",
		"PRZASNYSZ",
		"RADOMSKO",
		"SOSNOWIEC",
		"SWIECIE",
		"SWIDNIK",
		"SZTUM",
		"TARNOWSKIE GORY",
		"TRZEBINIA",
		"ZAMOSC",
	};
	for (unsigned i = 0; i < sizeof(names)/sizeof(char *); i++) {
		if (!strncmp(names[i], str, strlen(names[i]))) {
			return 1;
		}
	}
	return 0;
}

void out_html_tag(const char *str, int len)
{
	char *c;
	int l;
	if (!strncmp(str+1, "INFO", 4)) {
		puts("<div><span style=\"color: " COLOR_INFO ";\"><a name=\"INFO\"><b>&lt;INFO&gt;</b></a></span><br><img src=\"/wpn-img/common/info.png\"/>");
	} else if (!strncmp(str+1, "KSIEGOWOSC", 10)) {
		puts("<div style=\"color: " COLOR_KSIEGOWOSC ";\"><a name=\"KSIEGOWOSC\"><b>&lt;KSIEGOWOSC&gt;</b></a><br><img src=\"/wpn-img/common/ksiegowosc.png\"/>");
	} else if (!strncmp(str+1, "CIEKAWOSTKI", 11)) {
		puts("<div style=\"color: " COLOR_CIEKAWOSTKI ";\"><a name=\"CIEKAWOSTKI\"><b>&lt;CIEKAWOSTKI&gt;</b></a><br><img src=\"/wpn-img/common/ciekawostki.png\"/>");
	} else if (!strncmp(str+1, "CIEPLO-HOWTO", 12)) {
		puts("<div style=\"color: " COLOR_CIEPLO ";\"><a name=\"CIEPLO-HOWTO\"><b>&lt;CIEPLO-HOWTO&gt;</b></a><br><img src=\"/wpn-img/common/cieplo.png\"/>");
	} else {
		c = index(str, '>');
		l = (c - str) - 1;
		assert (c != NULL);
		fputs("<div style=\"color: ", stdout);
		if (!strncmp(str+1, "PRIV", 4)) {
			fputs(COLOR_PRIV, stdout);
		} else if (is_praterm(str + 1)) {
			fputs(COLOR_PRATERM, stdout);
		} else {
			fputs(COLOR_OTHER, stdout);
		}
		fputs(";\"><a name=\"", stdout);	
		print_n(str+1, l);
		fputs("\"><b>&lt;", stdout);
		print_n(str+1, l);
		puts("&gt;</b></a><br>");
	}
}

void out_html_tag_end(const char *str, int len)
{
	char *c;
	c = index(str, '>');
	assert (c != NULL);
	if (output_state.pre) {		
		output_state.pre = 0;
		puts("</pre>");
	}
	if (!strncmp(str+2, "INFO", 4)) {
		puts("<span style=\"color: magenta;\"><b>&lt;/INFO&gt;</b></span></div><br> ");
	} else {
		fputs("<b>&lt;/", stdout);
		print_n(str + 2, c - str - 2);
		puts("&gt;</b></div><br>\n");
	}
	output_state.hidden_par = 0;
}

void out_html_hidden(const char *str, int len)
{
	if (arguments.show_hidden) {
		puts("<span style=\"color: gray;\"><b>&lt;UWAGA!&gt;</b></span><br>");
	} else {
		output_state.hidden_par = 1;
	}

}

void out_html_empty(const char *str, int len)
{
	if (output_state.pre) {		
		puts("</pre>");
	}
	if (output_state.hidden_par) {
		output_state.hidden_par = 0;
	} else {
		if (!output_state.pre && !output_state.table)
			puts("<br>");
	}
	output_state.pre = 0;
}

void out_html_char(char c)
{
	if (c == '<')
		fputs("&lt;", stdout);
	else if (c == '>')
		fputs("&gt;", stdout);
	else if (c == '&')
		fputs("&amp;", stdout);
	else
		putchar(c);
}

/** Output text escaping HTML entities. */
void out_html_text(const char *str, int len)
{
	const char *c;
	int found;
	for (c = str; c < str + len; c++) {
		found = 0;
		/* This loop marks searched phrases in red. */
		for (size_t f = 0; f < filters.size(); f++) {
			if (!filters[f].is_regex
					&& filters[f].phrase 
					&& (
						(g_cgi_icase &&
						!strncasecmp(c, filters[f].phrase, 
						strlen(filters[f].phrase)))
						||
						(!g_cgi_icase &&
						!strncmp(c, filters[f].phrase, 
						strlen(filters[f].phrase)))
					)
			) {
				fputs("<span style=\"color: red;\">", stdout);
				for (char *p = filters[f].phrase; *p; p++, c++)
					out_html_char(*c);
				c--;
				fputs("</span>", stdout);
				found = 1;
				break;
			}
		}
		if (!found)
			out_html_char(*c);
	}
}

/* check for start of table */
int check_table_start(const char *str)
{
	while ((*str == ' ') || (*str == '\t')) str++;
	for (int i = 0; i < 4; i++, str++) {
		if ((*str != '+') && (*str != '-') && (*str != '='))
			return 0;
	}
	return 1;
}

/* check for continuation of table */
int check_table_in(const char *str)
{
	if ((index(str, '\n') - str )> 100)
		return 0;
	return 1;
}

void out_html_line(const char *str, int len)
{
	const char *r;
	const char *prev = str;
	int t;

	if (output_state.hidden_par)
		return;
	if (output_state.table) {
		if (!check_table_in(str)) {
			output_state.table = 0;
			puts("</pre>");
		}
	} else if (!output_state.pre) {
		if (check_table_start(str)) {
			output_state.table = 1;
			puts("<pre>");
		} else if (*str == '\t') {
			output_state.pre = 1;
			puts("<pre>");
		} 
	} else { /* output_state.pre */
		if (*str != '\t') {
			output_state.pre = 0;
			puts("</pre>");
		}
	}

	char c = *(str + len);
	*((char *)str + len) = 0;
	while ((t = search_regex(prev)) != REGEX_LAST) {
		/* t < 0 => filter regex */
		r = prev + g_regmatch[0].rm_so;
		if (r > prev) {
			out_html_text(prev, r - prev);
		}
		regex_output[t >= 0 ? t : REGEX_LAST](prev, 0);
		prev += g_regmatch[0].rm_eo ;
	}
	if (prev < (str + len))
		out_html_text(prev, str + len - prev);
	if (!output_state.pre && !output_state.table)
		puts("<br>");
	*((char *)str + len) = c;
}

void out_html_hline(const char *str, int len)
{
	if (arguments.show_hidden) {
		puts("<span style=\"color: gray;\">");
		out_html_line(str + 5, len - 5);
		puts("</span><br>");
	}
}

void html_output_end_para(const char *, int)
{
	if (output_state.table || output_state.pre)
		puts("</pre>");
	puts("<br>");
}

void html_output_end_tag(const char *, int)
{
	char *s;
	if (priv_name[0] != 0)
		asprintf(&s, "</PRIV>");
	else
		asprintf(&s, "</%s>", tag_name);
	out_html_tag_end(s, strlen(s));
	free(s);
}

void html_output(const char *str, int len)
{
	const char *cur = str;
	int ret, total = 0;

	if (!output_state.date_printed && !arguments.noline) {
		output_state.date_printed = 1;
		printf("<div class=\"date\"><a name=\"data%d\">Data:\
%c%c%c%c-%c%c-%c%c</a>",
				output_state.index, 
				output_state.date[0],
				output_state.date[1],
				output_state.date[2],
				output_state.date[3],
				output_state.date[4],
				output_state.date[5],
				output_state.date[6],
				output_state.date[7]
				);
		if (output_state.index > 0) {
			printf(" <a href=\"#data%d\">Poprzednia</a>", output_state.index - 1);
		}
		printf(" <a href=\"#data%d\">Nastêpna</a></div>\n", output_state.index + 1);
		output_state.index++;
	}
	if ((arguments.parse_type != PARSE_TAGS) && !output_state.tag_printed) {
		char *s;
		if (priv_name[0] != 0)
			asprintf(&s, "<PRIV TO=\"%s\">", priv_name);
		else
			asprintf(&s, "<%s>", tag_name);
		out_html_tag(s, strlen(s));
		output_state.tag_printed = 1;
		free(s);
	}
	while (total < len) {
		for (int i = 0; i < TOKEN_LAST; i++) {
			if ((ret = parse_functions[i](cur)) > 0) {
				output_functions[arguments.output][i](cur, ret);
				cur += ret;
				total += ret;
				break;
			}
		}
	}
}

/* CONSOLE OUTPUT FUNCTIONS
 */

void out_console_hidden(const char *str, int len)
{
	if (arguments.show_hidden) {
		if (!arguments.noline && !output_state.info_printed) {
			printf("\033[01;42m%s:\033[00m\033[01;32m%d:\033[00m", g_filename, g_start_line);
			output_state.info_printed = 1;
		}
		puts("\033[01;34m<UWAGA!!!>\033[00m");
	} else {
		output_state.hidden_par = 1;
	}

}

void out_console_text(const char *str, int len)
{
	const char *c;
	int found;
	for (c = str; c < str + len; c++) {
		found = 0;
		/* This loop marks searched phrases in red. */
		for (size_t f = 0; f < filters.size(); f++) {
			if (filters[f].phrase && !strncasecmp(c, filters[f].phrase, 
						strlen(filters[f].phrase))) {
				fputs("\033[01;31m", stdout);
				for (char *p = filters[f].phrase; *p; p++, c++)
					putchar(*c);
				fputs("\033[00m", stdout);
				c--;
				found = 1;
				break;
			}
		}
		if (!found)
			putchar(*c);
	}
}

void out_console_line(const char *str, int len)
{
	if (output_state.hidden_par)
		return;

	if (!arguments.noline && !output_state.info_printed) {
		printf("\033[01;42m%s:\033[00m\033[01;32m%d:\033[00m", g_filename, g_start_line);
		output_state.info_printed = 1;
	}

	char c = *(str + len);
	*((char *)str + len) = 0;
	const char *prev = str;
	while (search_regex(prev, 0) != REGEX_LAST) {
		const char* r = prev + g_regmatch[0].rm_so;
		if (r > prev) {
			out_console_text(prev, r - prev);
		}
		regex_output_console(prev, 0);
		prev += g_regmatch[0].rm_eo ;
	}
	if (prev < (str + len))
		out_console_text(prev, str + len - prev);
	*((char *)str + len) = c;
}

void out_console_hline(const char *str, int len)
{
	if (arguments.show_hidden) {
		if (!arguments.noline && !output_state.info_printed) {
			printf("\033[01;42m%s:\033[00m\033[01;32m%d:\033[00m", g_filename, g_start_line);
			output_state.info_printed = 1;
		}
		fputs("\033[01;34m<!!!>\033[00m", stdout);
		out_console_line(str +5, len - 5);
	}
}

void out_console_tag(const char *str, int len)
{
	if (!arguments.noline && !output_state.info_printed) {
		printf("\033[01;42m%s:\033[00m\033[01;32m%d:\033[00m", g_filename, g_start_line);
		output_state.info_printed = 1;
	}
	fputs("\033[01;35m", stdout);
	print_n(str, len);
	fputs("\033[00m", stdout);
	output_state.hidden_par = 0;
}

void console_output_end_para(const char*, int)
{
	output_state.hidden_par = 0;
	if (arguments.parse_type == PARSE_TAGS)
		putchar('\n');
}

void console_output(const char* str, int len)
{
	int total = 0, ret;
	const char *cur = str;
	
	output_state.info_printed = 0;
	while (total < len) {
		for (int i = 0; i < TOKEN_LAST; i++) {
			if ((ret = parse_functions[i](cur)) > 0) {
				output_functions[arguments.output][i](cur, ret);
				cur += ret;
				total += ret;
				break;
			}
		}
	}
}

void regex_output_console(const char *str, int len)
{
	fputs("\033[01;31m", stdout);
	print_n(str + g_regmatch[0].rm_so, g_regmatch[0].rm_eo - g_regmatch[0].rm_so);
	fputs("\033[00m", stdout);
}

/* TEXT OUTPUT FUNCTIONS
 */

void out_text_tag(const char *str, int len)
{
	if (!arguments.noline && !output_state.info_printed) {
		printf("%s:%d:", g_filename, g_start_line);
		output_state.info_printed = 1;
	}
	print_n(str, len);
	output_state.hidden_par = 0;
}

void out_text_hidden(const char *str, int len)
{
	if (!arguments.show_hidden)
		output_state.hidden_par = 1;
	else {
		if (!arguments.noline && !output_state.info_printed) {
			printf("%s:%d:", g_filename, g_start_line);
			output_state.info_printed = 1;
		}
		puts("<UWAGA!!!>");
	}
}

void out_text_line(const char *str, int len)
{
	if (output_state.hidden_par)
		return;
	if (!arguments.noline && !output_state.info_printed) {
		printf("%s:%d:", g_filename, g_start_line);
		output_state.info_printed = 1;
	}
	print_n(str, len);
}

void out_text_hline(const char *str, int len)
{
	if (arguments.show_hidden) {
		if (!arguments.noline && !output_state.info_printed) {
			printf("%s:%d:", g_filename, g_start_line);
			output_state.info_printed = 1;
		}
		out_text_line(str, len);
	}
}

void out_text_empty(const char *str, int len)
{
	if (output_state.hidden_par) 
		output_state.hidden_par = 0;
	else
		putchar('\n');
}


void text_output(const char* str, int len)
{
	int total = 0, ret;
	const char *cur = str;
	
	output_state.info_printed = 0;
	while (total < len) {
		for (int i = 0; i < TOKEN_LAST; i++) {
			if ((ret = parse_functions[i](cur)) > 0) {
				output_functions[arguments.output][i](cur, ret);
				cur += ret;
				total += ret;
				break;
			}
		}
	}
}

void text_output_end_para(const char *, int)
{
	output_state.hidden_par = 0;
	putchar('\n');
}

/**
 * MAIN GREP FUNCTIONS
 */

/** Prints info about to long CGI output and link for next result pages. */
void out_html_continue()
{
	printf("\
<div class=\"continue\"><a name=\"data%d\" href=\"?search=%s&start=%s\
&stop=%s&tags=%s\
&andor=%s&icase=%s&pars=%s\">\
Wyniki zapytania s± zbyt du¿e aby wy¶wietliæ je za jednym razem. Kliknij \
aby obejrzeæ dalsz± czê¶æ wyników.</a></div>\n",
		output_state.index,
		g_cgi_search,
		output_state.date,
		g_cgi_stop,
		g_cgi_tags,
		g_cgi_ormode ? "or" : "and",
		g_cgi_icase ? "true" : "",
		g_cgi_pars ? "true" : ""
		);
}

/** Simpler and faster version of grep_file, without hidden data handling, 
 * designed for maximum performance in 'web' mode.
 * @return 1 if output in cgi mode is to big to print it, 0 otherwise
 */
int grep_file_html(const char *str, const char* filename)
{
	char* cur = (char *)str;
	int ret, i;
	state_t state = S_OUT;
	state_t prev;
	token_t token = TOKEN_LAST;
	output_state.date_printed = 0;

	strncpy(output_state.date, basename(filename), 8);
	while (*cur != 0) {
		for (i = 0; i < TOKEN_LAST; i++) {
			prev = state;
			ret = (parse_functions[(token_t)i])(cur);
			if (ret > 0) {
				token = (token_t)i;
				state = state_trans[state][token];
				break;
			}
		}
		if (arguments.parse_type == PARSE_TAGS) {
			if ((token == TAG_START) || (token == PRIV_START)) {
				/* for 'whole tags' mode mark start of tag */
				cur_start = cur;
			} else if (token == TAG_END) {
				clear_filters();
				filter_line((char *)cur_start, cur, 0);
				if (check_filters()) {
					if (g_cgi && (g_printed_data > MAX_CGI_DATA) && !output_state.date_printed) {
						out_html_continue();
						return 1;
					}
					g_printed_data += cur - cur_start +
						ret;
					/* print matching tag */
					output_helpers[arguments.output][OUTPUT_H_MAIN](cur_start, cur - cur_start + ret);
				}
			}
		} else {
			if ((prev == S_TAG) && (
					(state == S_PAR) ||
					(state == S_HIDDEN) ||
					(state == S_HLINE))) {
				/* for 'paragraph' mode, mark start of para */
				cur_start = cur;
			}
			if (token == TAG_START)
				output_state.tag_printed = 0;
			if (((prev == S_PAR) || (prev == S_HLINE)) &&
					((state == S_TAG) || (state == S_OUT))) {
				clear_filters();
				filter_line((char*)cur_start, cur, 0);
				if (check_filters()) {
					if (g_cgi && (g_printed_data > MAX_CGI_DATA) && !output_state.date_printed) {
						out_html_continue();
						return 1;
					}
					g_printed_data += cur - cur_start;
					output_helpers[arguments.output][OUTPUT_H_MAIN](cur_start, cur - cur_start);
					output_helpers[arguments.output][OUTPUT_H_END_PARA](NULL, 0);
				}
			}
			if ((token == TAG_END) && (output_state.tag_printed))
				output_helpers[arguments.output][OUTPUT_H_END_TAG](NULL, 0);
		}
		cur += ret;
	}
	return 0;
}

/** Main parsing function.
 * @param str content of file
 * @param filename name of file
 */
int grep_file(const char *str, const char* filename)
{
	char* cur = (char *)str;
	int ret, i;
	state_t state = S_OUT;
	state_t prev;
	token_t token = TOKEN_LAST;
	int hidden_par = 0;	/* are we in hidden paragraph? */
	output_state.date_printed = 0;

	g_filename = filename;
	strncpy(output_state.date, basename(filename), 8);
	g_line = 0;
	while (*cur != 0) {
		g_line++;
		for (i = 0; i < TOKEN_LAST; i++) {
			prev = state;
			ret = (parse_functions[(token_t)i])(cur);
			if (ret > 0) {
//#define DEBUG_XGREP2
#ifdef DEBUG_XGREP2
				printf("DEBUG: TOKEN %d STATE %d: ", i, state);
				print_n(cur, ret);
				printf("\n");

#endif
				token = (token_t)i;
				state = state_trans[state][token];

				if (state == S_ERROR) {
					fprintf(stderr, "PARSE ERROR LINE %d FILE '%s' (PREV: %d TOKEN: %d CUR: %d)\n",
							g_line, g_filename,
							prev, token, state);
					exit(1);
				}
				break;
			}
		}
		if (ret < 0) {
			fprintf(stderr, "PARSE ERROR LINE %d (uknown token)\n", g_line);
			exit(1);
		}
		/* check for hidden para */
		if (token == HIDDEN_START) {
			hidden_par = 1;
		} else if (hidden_par && ((token == TAG_END) || (token == EMPTY_LINE))) {
			hidden_par = 0;
		}
		if (arguments.parse_type == PARSE_TAGS) {
			if ((token == TAG_START) || (token == PRIV_START)) {
				/* for 'whole tags' mode mark start of tag */
				cur_start = cur;
				g_start_line = g_line;
				clear_filters();
			} else if ((token == TAG_END) && check_filters()) {
				/* print matching tag */
				output_helpers[arguments.output][OUTPUT_H_MAIN](
						cur_start, cur - cur_start + ret);
			}
		} else if (arguments.parse_type == PARSE_LINES) {
			if (((prev == S_PAR) || (prev == S_HIDDEN) || (prev ==
				S_HLINE)) && check_filters()) {
				output_helpers[arguments.output][OUTPUT_H_MAIN](
						cur_start, cur - cur_start);
			}
			/* reset on overy token (one token == one line) */
			cur_start = cur;
			g_start_line = g_line;
			clear_filters();
		} else {
			if ((prev == S_TAG) && (
					(state == S_PAR) ||
					(state == S_HIDDEN) ||
					(state == S_HLINE))) {
				/* for 'paragraph' mode, mark start of para */
				cur_start = cur;
				g_start_line = g_line;
				clear_filters();
			}
			if (token == TAG_START)
				output_state.tag_printed = 0;
			if (check_filters() && ((prev == S_PAR) || (prev == S_HLINE)) &&
					((state == S_TAG) || (state == S_OUT))) {
				output_helpers[arguments.output][OUTPUT_H_MAIN](
						cur_start, cur - cur_start);
				output_helpers[arguments.output][OUTPUT_H_END_PARA](NULL, 0);
			}
			if ((token == TAG_END) && (output_state.tag_printed))
				output_helpers[arguments.output][OUTPUT_H_END_TAG](NULL, 0);

		}

		if ((state != S_OUT) && ((token == LINE) || (token == HIDDEN_LINE))) {
			filter_line(cur + ((token == HIDDEN_LINE) ? 5 : 0), cur + ret, 
						hidden_par || (token == HIDDEN_LINE));
		}
		cur += ret;
	}
	return 0;
}

/***
 * CGI FUNCTIONS
 */

/**
  * Dividies query string into name and content parts.
  */
void parse_query(const char *query, string* name, string* content)
{
	*name = "";
	*content = "";
	const char* eqs = index(query, '=');
	if (eqs == 0)
		return;
	name->append(query, 0, eqs - query);
	*content = eqs + 1;
}

/**
  * @return 1 for PN files with date between g_cgi_start and g_cgi_stop,
  * 0 otherwise. For use by scandir.
  */
int scandir_date(const struct dirent* dir)
{
	for (int i = 0; i < 8; i++) {
		if (!isdigit(dir->d_name[i]))
			return 0;
		if (dir->d_name[i] < g_cgi_start[i])
			return 0;
		if (dir->d_name[i] > g_cgi_start[i])
			break;
	}
	for (int i = 0; i < 8; i++) {
		if (dir->d_name[i] > g_cgi_stop[i])
			return 0;
		if (dir->d_name[i] < g_cgi_stop[i])
			break;
	}
	if (strcmp(".txt", dir->d_name + 8))
		return 0;
	return 1;
}

/** 
  * Selectes PN files based on start and stop date and append them
  * to global files vector.
  */
void parse_cgi_files()
{
	struct dirent **dirs;
	int c;
	c = scandir(PRATERM_NEWS_PATH, &dirs, scandir_date,
			alphasort);
	for (int i = 0; i < c; i++) {
		filenames.push_back(
				string(PRATERM_NEWS_PATH"/")
				+ string(dirs[i]->d_name));
		free(dirs[i]);
	}
	free(dirs);
}

/**
  * Parses search string and creates fitlers.
  * @param search search phrase
  * @param tags tag name
  * @param icase 1 for non-case-sensitive search, 0 otherwise
  * @param ormode 1 for OR mode, 0 for AND mode
  */
void parse_cgi_search(const char *search, const char *tags, 
		int icase, int ormode)
{
	int tokc = 0;
	char **toks;

	filter_t f = { 0, 0, 0, 0, NULL, NULL, NULL, 0, 0, 0 };
	tokenize(search, &toks, &tokc);
	if (!tags[0] || !strcmp(tags, "Wszystkie tagi")) {
		f.alltags = 1;
	} else {
		f.tag = strdup(tags);
	}
	if (!tokc && !f.alltags) {
		filters.push_back(f);
		return;
	}
	f.enough = ormode;
	if (icase)
		f.search_fun = search_strcase;
	else
		f.search_fun = search_str;
	for (int i = 0; i < tokc; i++) {
		f.phrase = strdup(toks[i]);
		filters.push_back(f);
	}
	tokenize(NULL, &toks, &tokc);
}

/**
 * Parses CGI query string and initializes arguments data.
 * param query query string
 * return 0 if query is correct, 1 otherwise
 */
int parse_cgi(const char *query)
{
	const char *search = "", *tags = "";
	
	char **toks;
	int tokc = 0;
	tokenize_d(query, &toks, &tokc, "&");
	for (int i = 0; i < tokc; i++) {
		string name, content;
		parse_query(toks[i], &name, &content);
		char* str = convURI(content.c_str());

		if (!name.compare("start")) {
			if (g_cgi_start[0]) {
				return 1;
			}
			g_cgi_start = str;
		} else if (!name.compare("stop")) {
			if (g_cgi_stop[0]) {
				return 1;
			}
			g_cgi_stop = str;
		} else if (!name.compare("search")) {
			if (search[0]) {
				return 1;
			}
			search = str;
			g_cgi_search = strdup(content.c_str());
		} else if (!name.compare("tags")) {
			if (tags[0]) {
				return 1;
			}
			tags = str;
			g_cgi_tags = strdup(content.c_str());
		} else if (!name.compare("pars")) {
			if (!strcmp("true", str))
				g_cgi_pars = 1;
			free(str);
		} else if (!name.compare("icase")) {
			if (!strcmp("true", str )) {
				g_cgi_icase = 1;
			}
			free(str);
		} else if (!name.compare("andor")) {
			if (!strcmp("or", str)) {
				g_cgi_ormode = 1;
			}
			free(str);
		} else {
			free(str);
		}
	}
	tokenize(NULL, &toks, &tokc);

	for (int i = 0; i < 8; i++) {
		if (!isdigit(g_cgi_start[i]))
			return 1;
		if (!isdigit(g_cgi_stop[i]))
			return 1;
	}
	if (g_cgi_start[8])
		return 1;
	if (g_cgi_stop[8])
		return 1;
			
	arguments.output = OUTPUT_HTML;
	if (g_cgi_pars)
		arguments.parse_type = PARSE_PARAGRAPHS;

	fputs("<div class=\"qinfo\">Twoje zapytanie: tagi - ", stdout);
	if (tags[0] == 0) {
		fputs("Wszystkie tagi", stdout);
	} else {
		out_html_text(tags, strlen(tags));
	}
	if (search[0] == 0) {
		fputs(", dowolne s³owa", stdout);
	} else {
		fputs(", s³owa - ", stdout);
		out_html_text(search, strlen(search));
		printf(" (%s, %s wielko¶æ liter)", 
				g_cgi_ormode ? "dowolne" : "wszystkie",
				g_cgi_icase ? "ignoruj" : "uwzglêdnij");
	}
	if (g_cgi_pars) {
		fputs(", wy¶wietlaj pojedyncze akapity.", stdout);
	} else {
		fputs(".", stdout);
	}
	printf("<br/>Data pocz±tkowa - %c%c%c%c-%c%c-%c%c, koñcowa -	%c%c%c%c-%c%c-%c%c.",
		g_cgi_start[0],g_cgi_start[1],g_cgi_start[2], g_cgi_start[3], 
		g_cgi_start[4], g_cgi_start[5],
		g_cgi_start[6], g_cgi_start[7],
		g_cgi_stop[0],g_cgi_stop[1],g_cgi_stop[2], g_cgi_stop[3], 
		g_cgi_stop[4], g_cgi_stop[5],
		g_cgi_stop[6], g_cgi_stop[7]);

	parse_cgi_files();
	printf(" %d wydañ Praterm News do przeszukania.</div>", 
			filenames.size());
	parse_cgi_search(search, tags, g_cgi_icase, g_cgi_ormode);

	return 0;
}

/**
 * Prints CGI output header.
 */
void start_cgi(const char *query)
{
	fputs("\
Content-Type: text/html\r\n\
\r\n\
<?xml version=\"1.0\" encoding=\"iso-8859-2\"?>\n\
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n\
<html>\n\
<head>\n\
<title>WPN v2.0</title>\n\
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-2\"/>\n\
<link rel=\"stylesheet\" href=\"/css/new.css\" type=\"text/css\" media=\"all\"/>\n\
<link rel=\"stylesheet\" href=\"/css/wpn.css\" type=\"text/css\" media=\"all\"/>\n\
</head>\n\
<body>\n\
<div>\n\
<a href=\"http://www.praterm.pl\"><img src=\"/images/praterm-www.png\" alt=\"Praterm S.A.\"></a><br/>\n\
<h1>WPN - system wyszukiwania informacji w bazie PRATERM News</h1>\n\
<h2>Wyniki wyszukiwania</h2>\n\
<p><a href=\"index.php?",
	stdout);
	fputs(query, stdout);
	fputs("\
\">Powrót do definiowania zapytania</a></p><br/>\n",
	stdout);
}

/**
 * Prints CGI footer.
 */
void end_cgi()
{
	fputs("\
</div></body></html>\n", stdout);
}

int
main(int argc, char *argv[])
{
	size_t i;
	off_t size = 0;
	char* str = NULL;
	int rd;
	int ret;

	if ((ret = init_regex()) != 0) {
		fprintf(stderr, "ERROR COMPILING EXPRESION %d\n", ret);
		exit(1);
	}

	if (getenv("QUERY_STRING")) {
		g_cgi = 1;
		start_cgi(getenv("QUERY_STRING"));
		if (parse_cgi(getenv("QUERY_STRING"))) {
			fputs("<span class=\"error\">B³êdne zapytanie!</span>",
				stdout);
		}
	} else {
		int err = argp_parse(&argp, argc, argv, 0, 0, &arguments);
		if (err != 0)
			return err;
	}

	tag_name[MAX_TAG_LENGTH] = 0;
	priv_name[2] = 0;
	for (i = 0; i < filenames.size(); i++) {
		int fd = open(filenames[i].c_str(), O_RDONLY);
		if (fd == -1) {
			fprintf(stderr, "Error opening file %s\n", filenames[i].c_str());
			continue;
		}
		struct stat st;
		if (fstat(fd, &st) != 0) {
			fprintf(stderr, "Error accessing file %s\n", filenames[i].c_str());
			close(fd);
			continue;
		}
		if (st.st_size > size) {
			size = st.st_size;
			str = (char *) realloc(str, size + 1);
		}
		assert (str != NULL);
		rd = 0;
		while (rd < size) {
			ret = read(fd, str + rd, size - rd);
			if (ret < 0) {
				fprintf(stderr, "Error reading file %s\n", filenames[i].c_str());
				close(fd);
				continue;
			}
			if (ret == 0)
				break;
			rd += ret;
		}
		close(fd);
		str[rd] = 0;

		if (grep_functions[arguments.output](str, filenames[i].c_str()))
			break;;
	}
	if (g_cgi) {
		end_cgi();
		return 0;
	}
	return g_return_code;
}

