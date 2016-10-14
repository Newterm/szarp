/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * SZARP 2.0
 * biblioteka MyInc
 * libpar.c
 */

/*
 * libpar.c
 *
 * Modul obslugi plikow konfiguracyjnych
 * dla projektu Szarp
 *
 * 1998.03.12 Codematic
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <wchar.h>

#include "libpar.h"
#include "libpar_int.h"
#include "liblog.h"

#ifndef MINGW32
#include "msgerror.h"
#include "execute.h"
#endif

#ifdef MINGW32
#include "mingw32_missing.h"
#endif

Par *globals = NULL;
Sections *sect = NULL;

/**lista zmiennych rozpoznanych przez libpar_read_cmdline*/
static Par* command_line_pars = NULL;

int parse(FILE * stream);
void parser_reset(void);
void parser_hard_reset(void);
void parser_destroy(void);

/* Implementacja f. p.*/

Sections *AddSection(const char *name)
{
	Sections *nowa, *curr;

	if (strcasecmp(name, "global") == 0)
		return NULL;

	nowa = SeekSect(name);

	if (nowa != NULL)
		return nowa;

	nowa = (Sections*)malloc(sizeof(Sections));
	nowa->next = NULL;
	nowa->pars = NULL;
	if (name)
		nowa->name = strdup(name);
	else
		nowa->name = strdup("");

sz_log(8, "AddSection(): section name \"%s\"", name);

	if (!sect) {
		sect = nowa;
		return nowa;
	}

	curr = sect;
	while (curr->next)
		curr = curr->next;
	curr->next = nowa;

	return nowa;
}

void AddPar(Par ** list, const char *name, const char *content)
{
	Par *nowa, *curr;

	/* replace existing parameter content if param already exists */
	curr = *list;
	while (curr) {
		if (!strcmp(curr->name, name)) {
			free(curr->content);
			if (content)
				curr->content = strdup(content);
			else
				curr->content = strdup("");
			return;
		}
		curr = curr->next;
	}

	/* add new param */
	nowa = (Par*) malloc(sizeof(Par));
	nowa->next = NULL;
	if (name)
		nowa->name = strdup(name);
	else
		nowa->name = strdup("");
	if (content)
		nowa->content = strdup(content);
	else
		nowa->content = strdup("");

sz_log(8, "AddPar(): added parametr \"%s\" to last section", name);

	if (!(*list)) {
		*list = nowa;
		return;
	}

	curr = *list;
	while (curr->next)
		curr = curr->next;
	curr->next = nowa;
}

void DeleteParList(Par ** list)
{
	Par *curr, *nextcurr;

	if (!(*list))
		return;
	curr = *list;
	while (curr) {
		nextcurr = curr->next;
		free(curr->name);
		free(curr->content);
		free(curr);
		curr = nextcurr;
	}
	*list = NULL;
}

void DeleteSectList()
{
	Sections *curr, *nextcurr;

	if (!sect)
		return;
	curr = sect;
	while (curr) {
		nextcurr = curr->next;
		DeleteParList(&(curr->pars));
		free(curr->name);
		free(curr);
		curr = nextcurr;
	}
	sect = NULL;
}

char *SeekPar(Par * list, const char *name)
{
	Par *curr;
	int found;

	if (!list)
		return NULL;
	found = 0;
	curr = list;
	while (curr) {
		if (!strcmp(curr->name, name)) {
			found = 1;
			break;
		}
		curr = curr->next;
	}
	if (found)
		return curr->content;
	else
		return NULL;
}

Sections *SeekSect(const char *name)
{
	Sections *curr;
	int found;

	if (!sect)
		return NULL;
	found = 0;
	curr = sect;
	while (curr) {
		if (!strcmp(curr->name, name)) {
			found = 1;
			break;
		}
		curr = curr->next;
	}
	if (found)
		return curr;
	else
		return NULL;
}

/* Interface */
#ifndef MINGW32
void libpar_init()
{
	libpar_init_with_filename(NULL, 1);
}
#endif

/* 
 * Parses commandline for variables definitions
 * (arguments like "-Dname=value")
 */
void libpar_read_cmdline(int *argc, char *argv[])
{
	int i, j;
	int l;
	char *endptr;
	char *name, *val;

	/* Scan commandline */
	for (i = 1; i < *argc; i++) {
		l = strlen(argv[i]);
		if (l < 5)
			continue;
		if (argv[i][0] != '-')
			continue;
		if (argv[i][1] != 'D')
			continue;
		j = 2;
		while ((j < l) && (argv[i][j] != '='))
			j++;
		if (argv[i][j] != '=')
			continue;
		if (j < 3)
			continue;
		if (j >= (l - 1))
			continue;
		/* Wyciete sprawdzanie zawartosci zmiennej - moze byc
		 * cokolwiek. */

		/*
		   if (!(isalpha(argv[i][j+1])))
		   continue;
		   a = 1;
		   for (k = j+2; k < l; k++)
		   if (!( isalnum(argv[i][k]) || (argv[i][k] == '_')) )
		   a = 0;
		   if (!a)
		   continue; */
		name = (char *) malloc(j - 1);
		strncpy(name, argv[i] + 2, j - 2);
		name[j - 2] = 0;
		val = (char *) malloc(l - j);
		strncpy(val, argv[i] + j + 1, l - j - 1);
		val[l - j - 1] = 0;

		setvalue(name, val);
		AddPar(&command_line_pars, name, val);

		free(name);
		free(val);

		/* Shift commandline */
		(*argc)--;
		if (i == *argc)
			continue;
		endptr = argv[i];
		memmove(argv + i, argv + i + 1,
			(*argc - i) * (sizeof(char *)));
		argv[*argc] = endptr;
		i--;
	}
	/* add argc, argvn variables */
	asprintf(&val, "%d", *argc);
	setvalue("argc", val);
	free(val);
	for (i = 0; i < *argc; i++) {
		asprintf(&name, "argv%d", i);
		setvalue(name, argv[i]);
		free(name);
	}
}

/**
 * Version of libpar_read_cmdline for wide-characters strings in argv
 */
void libpar_read_cmdline_w(int *argc, wchar_t *argv[])
{
	int i, j;
	int l;
	wchar_t *endptr;
	wchar_t *name, *val;
#define MAXLEN 127
	char cname[MAXLEN+1], cval[MAXLEN+1];

	/* Scan commandline */
	for (i = 1; i < *argc; i++) {
		l = wcslen(argv[i]);
		if (l < 5)
			continue;
		if (argv[i][0] != L'-')
			continue;
		if (argv[i][1] != L'D')
			continue;
		j = 2;
		while ((j < l) && (argv[i][j] != L'='))
			j++;
		if (argv[i][j] != L'=')
			continue;
		if (j < 3)
			continue;
		if (j >= (l - 1))
			continue;

		name = (wchar_t *) malloc((j - 1) * sizeof(wchar_t));
		wcsncpy(name, argv[i] + 2, j - 2);
		name[j - 2] = L'\0';
		val = (wchar_t *) malloc((l - j) * sizeof(wchar_t));
		wcsncpy(val, argv[i] + j + 1, l - j - 1);
		val[l - j - 1] = 0;

		cname[0] = 0;
		cval[0] = 0;
		wcstombs(cname, name, MAXLEN);
		wcstombs(cval, val, MAXLEN);
		
		setvalue(cname, cval);
		AddPar(&command_line_pars, cname, cval);

		free(name);
		free(val);

		/* Shift commandline */
		(*argc)--;
		if (i == *argc)
			continue;
		endptr = argv[i];
		memmove(argv + i, argv + i + 1,
			(*argc - i) * (sizeof(wchar_t *)));
		argv[*argc] = endptr;
		i--;
	}
	/* add argc, argvn variables */
	snprintf(cval, MAXLEN, "%d", *argc);
	setvalue("argc", cval);
	for (i = 0; i < *argc; i++) {
		snprintf(cname, MAXLEN, "argv%d", i);
		wcstombs(cval, argv[i], MAXLEN);
		setvalue(cname, cval);
	}
}

int libpar_init_with_filename(const char *filename, int exit_on_error)
{
    FILE *desc;
    
    if (!filename) {
#ifdef MINGW32
	sz_log(0,
		"libpar_init_with_filename: filename can't be NULL on Windows platform");
	if (exit_on_error)
	    exit(1);
	else
	    return -1;
#else
	desc = fopen("/etc/szarp/" CFGNAME, "r");
	if (!desc) {
	    sz_log(0, "Cannot open config file: \"%s\"!", "/etc/szarp/" CFGNAME);
	    if (exit_on_error)
		exit(1);
	    else
		return -1;
	}
#endif
    } else {
	desc = fopen(filename, "r");
	if (!desc) {
	    sz_log(0, "Cannot open config file \"%s\" (errno %d)", filename, errno);
	    if (exit_on_error)
		exit(1);
	    else
		return -1;
	}
    }
    
    int parse_ret = parse(desc);
    fclose(desc);

    return parse_ret;
}

void libpar_done()
{
	DeleteParList(&globals);
	DeleteParList(&command_line_pars);
	DeleteSectList();
	parser_destroy();
}

void libpar_getkey(const char *section, const char *par, char **buf)
{
	Sections *s;
	char *c;

	s = SeekSect(section);
	if (s) {
		c = SeekPar(s->pars, par);
		if (!c)
			c = SeekPar(globals, par);
		/* Sekcja jest, nie ma par., szukaj w globalnych */
	}
	else			/* Nie ma sekcji, szukaj w globalnych */
		c = SeekPar(globals, par);

	*buf = c;
}

void libpar_readpar(const char *section, const char *par, char *buf,
		    int size, int exit_on_error)
{
	char *c;

	libpar_getkey(section, par, &c);
	if (c)
		strncpy(buf, c, size);
	else {
		if (exit_on_error) {
		sz_log(0,
			    "libpar: parameter '%s' not found in config file, exiting",
			    par);
			exit(1);
		}
	}
}

char *libpar_getpar(const char *section, const char *par, int exit_on_error)
{
	char *c;

	libpar_getkey(section, par, &c);
	if (c)
		return strdup(c);
	if (exit_on_error) {
	sz_log(0,
		    "libpar: parameter '%s' not found in config file, exiting",
		    par);
		exit(1);
	}
	return NULL;
}

void libpar_reset() {
	int args_count = 0;
	int i;
	char** argv = NULL;
	Par* par;
	char buf[10];
	char* argc = getvalue("argc");

	if (strlen(argc) == 0)  {
		free(argc);
		argc = NULL;
	} else
		args_count = atoi(argc);

	if (argc) {
		args_count = atoi(argc);

		argv = (char**) malloc(args_count * sizeof(char*));

		for (i = 0;i < args_count; ++i) {
			sprintf(buf, "argv%d" ,i);
			argv[i] = getvalue(buf);
		}
	}

	DeleteParList(&globals);
	DeleteSectList();
	parser_reset();

	if (argc) {
		for (i = 0;i < args_count ; ++i) {
			sprintf(buf, "argv%d" ,i);
			setvalue(buf, argv[i]);
			free(argv[i]);
		}
		setvalue("argc", argc);
		free(argv);
		free(argc);
	}

	par = command_line_pars;
	while (par) {
		setvalue(par->name, par->content);
		par = par->next;
	}


}

#ifndef MINGW32 
void libpar_reinit() {
	libpar_reset();
	libpar_init();
}
#endif

void libpar_hard_reset() {
	libpar_reset();
	parser_hard_reset();
}

void libpar_reinit_with_filename(const char *name, int exit_on_error) {
	libpar_reset();
	libpar_init_with_filename(name, exit_on_error);
}

#ifndef MINGW32

void libpar_setXenvironment(const char *programname)
{
#define XENV "XENVIRONMENT"
	char *c;
	static char buf[1000];

	libpar_getkey(XENV, programname, &c);
	if (!c) {
		WarnMessage(7, programname);
		return;
	}
	strcpy(buf, XENV);
	strcat(buf, "=");
	strncat(buf, c, sizeof(buf) - sizeof(XENV) - 2);
	putenv(buf);
}

void libpar_printfile(char *programname, char *printcmd, char *filename)
{
	char *c, *ss;

	libpar_getkey(programname, printcmd, &c);
	if (!c) {
		WarnMessage(7, printcmd);
		return;
	};

	ss = (char*) malloc(strlen(c) + strlen(filename) + 1);
	sprintf(ss, c, filename);
	execute(ss);
	free(ss);
}
#endif
