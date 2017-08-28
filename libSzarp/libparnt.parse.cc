/* 
  libSzarp - SZARP library

*/
/*
 * $Id$
 *
 * SZARP 2.1
 * 
 * Pawe³ Pa³ucha
 * pawel@praterm.com.pl
 *
 * 25.04.2001
 *
 * Analizator skladniowy i parser dla nowej wersji libpara - parsowanie
 * plikow konfiguracyjnych SZARP'a.
 * Obsluguje dyrektywy "$include" oraz "$if ... $elseif ... $else ... $end".
 *
 * Obs³uga zmiennych (postaci $nazwa$).
 * Przypisanie warto¶ci (string lub wywolanie funkcji): 
 * $nazwa$ := wartosc
 * 
 * Nazwa zmiennej jest rozwijana w stringach, nazwach sekcji i parametrow,
 * zawarto¶ciach parametrów, nazwach funkcji. Pojedynczy znak $ uzyskujemy
 * przez $$.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libparnt.parse.h"
#include "liblog.h"
#include "libpar_int.h"
#include "execute.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_IF_LEVEL	10

#ifdef HAVE_YYLEX_DESTROY
	int yylex_destroy(void);
#endif

void yylex_reinit_globals(void);

/* lexical analyzer input stream */
extern FILE *yyin;

/*****************************************************************************/
/* "if" block parsing structs */
/*****************************************************************************/
struct parse_str {
		int active;		/* block active */
		int done;		/* some branch already executed */
		int else_st;	/* "else" branch already parsed */
};

/* stack for "if" blocks */
struct parse_str parse_stack[MAX_IF_LEVEL];

/* current "if" block level */
int parse_level = 0;

/*****************************************************************************/
/* registered functions data */
/****************************************************************************/

/* number of registered functions */
int nofunc = 0;

/* type of functions */
typedef char *(func_type)(char *);

#define MAX_FUNCTION_NAME_LENGTH	30
struct func_str {
		char name[MAX_FUNCTION_NAME_LENGTH];
		func_type *func;
};

/* table of registered functions */
struct func_str *registered_funcs = NULL;

/******************************************************************************/
/* Registered functions */
/******************************************************************************/

/* 
 * Register function f with name "name" (max. name length is 30),
 * return 0 on success, -1 on error.
 * If there was already a function with that name registered, it's
 * replaced by f.
 */
int register_function(const char *name, func_type *f)
{
		struct func_str * tmp;
		int i;
		
		/* check for other functions with that name */
		for (i = 0; i < nofunc; i++) 
				if (!strcmp(registered_funcs[i].name, name)) {
						registered_funcs[i].func = f;
						return 0;
				}
		/* create new table */
		tmp = (struct func_str*) malloc ((nofunc + 1) * sizeof(struct func_str));
		if (!tmp) {
			sz_log(1, "Not enough memory to register function \"%s\"", 
								name);
				return -1;
		}
		if (nofunc > 0) {
				/* copy old table to new one */
				memcpy(tmp, registered_funcs, nofunc * sizeof(struct func_str));
				free(registered_funcs);
		}
		/* add new function */
		strncpy(tmp[nofunc].name, name, MAX_FUNCTION_NAME_LENGTH-1);
		tmp[nofunc].name[MAX_FUNCTION_NAME_LENGTH-1] = 0;
		tmp[nofunc].func = f;
		registered_funcs = tmp;
		nofunc++;
		return 0;
}

/*
 * Executes function registered with name "name", with params "params".
 * Returns NULL on error or string returned by function.
 */
char *execute_reg_func(char * name, char * params)
{
		int i;
		/* Find function by name */
		for (i = 0; i < nofunc; i++)
				if (!strcmp(registered_funcs[i].name, name))
						return registered_funcs[i].func(params);
	sz_log(1, "Could not find function registered with name \"%s\"", name);
		return NULL;
}


/*****************************************************************************/
/* "if" statement parsing */
/*****************************************************************************/

/* push new "if" block on stack */
int push_level(void)
{
		if (parse_level >= MAX_IF_LEVEL) {
			sz_log(1, "Parse error: \"$if\" block nested to deeply");
				return -1;
		}
		parse_stack[parse_level].active = 0;
		parse_stack[parse_level].done = 0;
		parse_stack[parse_level].else_st = 0;
		parse_level++;
		return 0;
}

/* Pop one "if" block from stack */
int pop_level(void)
{
		if (parse_level <= 0) {
			sz_log(1, "Parse error: unexpected \"$end\"");
				return -1;
		}
		parse_level--;
		return 0;
}

/* Some not-interesting functions */
int get_level(void)
{
		return parse_level;
}

int level_done(void)
{
		if (parse_level <= 0)
				return 0;
		return parse_stack[parse_level - 1].done;
}

void set_done(void)
{
		if (parse_level > 0) 
				parse_stack[parse_level -1 ].done = 1;
}

int level_active(void)
{
		if (parse_level <= 0)
				return 1;
		return parse_stack[parse_level - 1].active;
}

int level_else(void)
{
		if (parse_level <= 0)
				return 1;
		return parse_stack[parse_level - 1].else_st;
}

void set_else(void)
{
		if (parse_level > 0)
				parse_stack[parse_level - 1 ].else_st = 1;
}

void set_active(void)
{
		if (parse_level > 0) 
				parse_stack[parse_level -1 ].active = 1;
}

/*****************************************************************************
 * Variables
 *****************************************************************************/

/* Initial size of variables table */
#define INIT_VARS_SIZE 10
/* Current size of vars table */
int vars_size = 0;
/* Current number of defined variables */
int vars_num = 0;
/* Strucuture for holding variables */
typedef struct vars_str vars_t;
struct vars_str {
	char *name;
	char *val;
};
/* Global table of defined variables */
vars_t *vars = NULL;

/* Returns index of variable in table or -1 if variable not exist. */
int findvar(const char *name)
{
	int i;
	
	for (i = 0; i < vars_num; i++)
		if (!strcmp(vars[i].name, name))
			return i;
	return -1;
}

/* Adds new variable to the end of the table */
void newvar(const char *name, const char *val)
{
	vars_t *tmp;
	
	if (vars == NULL) {
		vars_size = INIT_VARS_SIZE;
		vars = (vars_t *) malloc (sizeof(vars_t) * INIT_VARS_SIZE);
	}
	if (vars_num >= vars_size) {
		/* Double the size of table */
		tmp = (vars_t *) malloc (sizeof(vars_t) * 2 * vars_size);
		memcpy(tmp, vars, vars_size * (sizeof(vars_t)));
		free(vars);
		vars = tmp;
		vars_size *= 2;
	}
	vars[vars_num].name = strdup(name);
	vars[vars_num].val = strdup(val);
	vars_num++;
}

/* Set value to variable. If variable not exist, it is created. */
void setvalue(const char *name, const char *val)
{
	int i;

	i = findvar(name);
	if (i < 0)
		newvar(name, val);
	else {
		free(vars[i].val);
		vars[i].val = strdup(val);
	}
}

/* Returns copy of variable value. If variable is not defined, zero-length
 * string is returned. */
char *getvalue(const char *name)
{
	int i;
	i = findvar(name);
	if (i < 0)
		return (strdup(""));
	return (strdup(vars[i].val));
}

/* If str contains variable name ($name$), variable name is evaluated to
 * its value. Original string is FREED and new one is returned. '$$' is
 * evaluated to '$'. Variable name must start with letter or underscore,
 * can also contain digits. If variable name is incorrect, no substitution
 * is made.
 */
char *eval(char *str)
{
	char *d, *name;
	int l, i, j;
	
	l = strlen(str);
	i = 0;
begin:
	while ((str[i] != 0) && (str[i] != '$'))
		i++;
	if (str[i] == 0) 
		return str;
	/* '$' spotted */
	if (str[i+1] == 0) 
		/* end of string */
		return str;
	if (str[i+1] == '$') {
		/* double $, cat one off */
		i++;
		d = (char *) malloc(l);
		l--;
		strncpy(d, str, i);
		strcpy(d+i, str+i+1);
		free(str);
		str = d;
		goto begin;
	}
	j = i+1;
	while (isalnum(str[j]) || (str[j] == '_'))
		/* read id chars */
		j++;
	if (str[j] != '$') {
		i = j;
		goto begin;
	}
	/* Now, we have a variable name */
	/* Terminate var name */
	str[j] = 0;
	/* Get var value */
	name = getvalue(str+i+1);
	/* Compute new string length */
	l = l + strlen(name) - 1 - j + i;
	/* Make new string */
	d = (char *) malloc (l+1);
	strncpy(d, str, i);
	strcpy(d+i, name);
	i += strlen(name);
	strcpy(d+i, str+j+1);
	free(name);
	free(str);
	str = d;
	goto begin;
}

/*****************************************************************************
 * Main parser functions
 *****************************************************************************/
  
/* Last scaned lexem */
int lex;

int yylex(void);

YYSTYPE yylval;

/* Scan function */
void scan(void)
{
		lex = yylex();
}

/* Scan and test for x */
int test(int x)
{
		scan();
		if (lex != x)
				return 0;
		return 1;
}

/* 
 * Parses argument of conditional expresion (string or function). 
 * Return parsed string or value returned by function
 */
char *parse_arg(void)
{
	char * tmp, *arg, *ret;
	
	switch (lex) {
		case variable :
			tmp = getvalue(yylval.str);
			scan();
			return tmp;
		case string_token :
			/* Copy string */
			tmp = eval(strdup(yylval.str));
			scan();
			return tmp;
		case id_token :
			/* Copy name of the function */
			tmp = eval(strdup(yylval.str));
			/* '(' expected */
			if (!test('(')) {
			sz_log(1, "Parse error: '(' expected");
				return NULL;
			}
			/* Process function argument */
			scan();
			arg = parse_arg();
			if (!arg) {
			sz_log(1, "Parse error: wrong function argument");
				return NULL;
			}
			/* ')' expected */
			if (lex != ')') {
			sz_log(1, "Parse error: ')' expected");
				return NULL;
			}
			/* execute function */
			ret = execute_reg_func(tmp, arg);
			free(tmp);
			free(arg);
			scan();
			return ret;
		default :
		sz_log(1, "Parse error: string or function expected, got %d",
					lex);
			return NULL;
	}
}

int debug_mode = 0;

/* 
 * Set parser to "debug" mode - sections and parameteres are not added but
 * only printed to stdout
 */
void set_debug_mode(void)
{
		debug_mode = 1;
}

/*
 * This is frontend for function execute_subst, that removes one end-of-line
 * from the end of its output.
 */
#ifndef MINGW32
char *execute_subst_noeol(char *params)
{
	char *out;
	int l;
	
	out = execute_subst(params);
	if ((out != NULL) && ((l = strlen(out)) > 0) && (out[l-1]=='\n')) 
		out[l-1] = '\0';
	return out;
}
#endif

/*
 * This is a function that return platform name, currently "linux" on Linux
 * platform and "windows" on MS Windows.
 */
char *platform(char *params)
{
#ifdef MINGW32
        return strdup("windows");
#else
        return strdup("linux");
#endif
}

/* This is empty function used instead of 'execute_stub*' on Windows platform.
 * It returns empty string.
 */
#ifdef MINGW32
char *execute_empty(char *params)
{
        return strdup("");
}
#endif

/*
 * Main parsing function. Returns 0 on success, -1 on error.
 */
int parse(FILE *stream)
{
	char *tmp, *tmp2;
	int op;
	int eq;
	int done;
	/* section curently used for adding parameters */
	Sections *curr = NULL;
	
	/* Set input fo lexical analyzer */
	yyin = stream;
	/* This is needed by lexical analyzer to work correctly */
	yylval.str = NULL;
	/* Register functions */
#ifdef MINGW32
	register_function("execute", execute_empty);
	register_function("exec", execute_empty);
#else
	register_function("execute", execute_subst);
	register_function("exec", execute_subst_noeol);
#endif
        register_function("platform", platform);
	/* We always want to have one lexem scanned */
	scan();
	while (1) {
		/* Debug info on level 10 */
	sz_log(10, "PARSE: level: %d, active: %d, done: %d, else: %d, lex: %d\n", 
				get_level(), level_active(), level_done(), 
				level_else(), lex);
		switch (lex) {
			case 0 :
				/* 
				 * end of file, check if all "if" statements are
				 * properly closed
				 */
				if (get_level() == 0)
					return 0;
			sz_log(1, "Parse error: \"$end\" expected");
				return -1;
			case parse_error : 
				/* return error from */
				return -1;
			case section_token :
				/* section name spotted, check if current statement
				 * is active
				 */
				if (level_active()) {
					tmp = eval(strdup(yylval.str));
					if (debug_mode)
						printf("SECTION: \"%s\"\n",
							       tmp);
					else
						curr = AddSection(yylval.str);
					free(tmp);
				}
				scan();
				break;
			case param_token :
				/* Param name spotted, copy it */
				tmp = eval(strdup(yylval.str));
				/* Look for param content */
				scan();
				if (lex != param_content) {
				sz_log(1, "Parse error: no content for param \"%s\"",
							tmp);
					free(tmp);
					return -1;
				}
				/* Check if current statement is active */
				if (level_active()) {
					tmp2 = eval(strdup(yylval.str));
					if (debug_mode)
						printf("PARAM: \"%s\" = \"%s\"\n", 
								tmp, tmp2);
					else {
						if (!curr)
							AddPar(&globals, tmp, 
									tmp2);
						else
							AddPar(&(curr->pars), 
									tmp,
									tmp2);
					}
					free(tmp2);
				}
				free(tmp);
				scan();
				break;
			case if_dir :
				/* "$if" keyword spotted, make new statement */
				scan();
				/* first argument of conditional expresion */
				tmp = parse_arg();
				if (!tmp) 
					return -1;
				/* operator "=" or "<>" expected */
				op = lex;
				if ((op != '=') && (op != not_equal_op)) {
				sz_log(1, "Parse error: operator expected");
					free(tmp);
					return -1;
				}
				/* second argument */
				scan();
				tmp2 = parse_arg();
				if (!tmp2) {
					free(tmp);
					return -1;
				}
				/* if current statement is active, evaluate the condition
				 * and mark statement active if condition is true
				 */
				if (level_active()) {
					eq = !strcmp(tmp, tmp2);
					if (op == not_equal_op)
						eq = !eq;
					push_level();
					if (eq) {
						set_active();
						/* set information for "else" statement */
						set_done();
					}
				} else {
					push_level();
				}
				free(tmp);
				free(tmp2);
				break;
			case elseif_dir :
				/* "$elseif" keyword, must have been followed by "$if"
				 * - not allowed on 0 level 
				 */
				if (level_else()) {
				sz_log(1, "Parse error: unexpected \"$elseif\"");
					return -1;
				}
				/* parse condition */
				scan();
				tmp = parse_arg();
				if (!tmp)
					return -1;
				op = lex;
				if ((op != '=') && (op != not_equal_op)) {
				sz_log(1, "Parse error: operator expected");
					free(tmp);
					return -1;
				}
				scan();
				tmp2 = parse_arg();
				if (!tmp2) {
					free(tmp);
					return -1;
				}
				done = level_done();
				pop_level();
				/* evaluate condition only if current statement is active
				 * and previous branches where not executed
				 */
				if (level_active() && !done) {
					eq = !strcmp(tmp, tmp2);
					if (op == not_equal_op)
						eq = !eq;
					push_level();
					if (eq) {
						set_active();
						set_done();
					}
				} else {
					push_level();
					if (done)
						set_done();
				}
				free(tmp);
				free(tmp2);
				break;
			case else_dir :
				/* "$else" keyword, only one allowed on each level */
				if (level_else()) {
				sz_log(1, "Parse error: second \"$else\" on current level");
					return -1;
				}
				scan();
				done = level_done();
				pop_level();
				if (level_active() && !done) {
					push_level();
					set_active();
					set_done();
					set_else();
				} else {
					push_level();
					set_else();
					if (done)
						set_done();
				}
				break;
			case end_dir :
				/* "$end" keyword - and of "if" statement - one level up */
				pop_level();
				scan();
				break;
			case variable :
				/* Bind value to variable */
				/* Copy name of variable */
				tmp = strdup(yylval.str);
				if (!test(becomes)) {
				sz_log(1, "Parse error: \":=\" expected after variable name (\"%s\")", tmp);
					free(tmp);
					return -1;
				}
				scan();
				tmp2 = parse_arg();
				if (!tmp2) {
					free(tmp);
					return -1;
				}
				if (level_active())
					setvalue(tmp, tmp2);
				/* setvalue always make copies */
				free(tmp); free(tmp2);
				break;
			default :
				/* This should never happen */
			sz_log(1, "Parse error: unknown lexem (%d)!",
						lex);
				return -1;
				break;
		}
	}
}

/**usuwa zarejestrowane zmienne oraz funkcje*/ 
void parser_reset(void) {
	int i;

	if (vars) {
		for (i = 0; i < vars_num; i++) {
			free(vars[i].name);
			free(vars[i].val);
		}
		free(vars);
		vars = NULL;
	}
	vars_num = 0;
	if (registered_funcs) {
		free(registered_funcs);
		registered_funcs = NULL;
	}
	nofunc = 0;

}

void parser_hard_reset(void) {
	parser_reset();
	yylex_reinit_globals();
}

void parser_destroy(void)
{
	parser_reset();
#ifdef HAVE_YYLEX_DESTROY
	yylex_destroy();
#endif
}

