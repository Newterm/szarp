/* 
  libSzarp - SZARP library

*/
/*
 * $Id: libparnt.lex 6199 2008-11-20 08:28:37Z reksio $
 * 
 * SZARP 2.1
 *
 * Pawe³ Pa³ucha (pawel@praterm.com.pl)
 * 25.04.2001
 *
 * Plik z regulami dla flex'a - analizator leksykalny pliku konfiguracyjnego
 * SZARPA (szarp.cfg).
 */

%{
	/* error handling */
	#include "liblog.h"
	#include <errno.h>
	#include "libparnt.parse.h"
	#include <string.h>
    
	#include <stdlib.h>
	
	#define YYERROR_VERBOSE

	extern YYSTYPE yylval;

	/* "$include" directive handling */
	#define MAX_INCLUDE_DEPTH 10
	YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
	int include_stack_ptr = 0;
	int i;
	
%}
%x FORMULA
%x STRING
%x PARAM
%x INCLUDE
%x INCLUDE_NAME
%x COMMENT

%array
%option noyywrap

%%

<INITIAL>"#"	{
			/* Start of the comment */
			BEGIN(COMMENT);
		}
<COMMENT>"\\\n"		{
		/* ignore end-of-line in comment because of backslash */
		}
<COMMENT>"\n"	{
			/* end of comment */
			BEGIN(INITIAL);
		}
<COMMENT>.	{
			/* eat up comment */
		}


<INITIAL>"$include"	{
			/* Start processing "$include" directive */
			BEGIN(INCLUDE);
		}
	
<INITIAL>"$"[a-zA-Z_][a-zA-Z_0-9]*"$" {
			/* Variable */
			BEGIN(FORMULA);
			if (yylval.str)
				free(yylval.str);
			yylval.str = strdup(yytext+1);
			yylval.str[yyleng-2] = 0;
			return variable; 
		}
		
<INITIAL>"$"	{	
			/* Start processing another directive (formula) */
			BEGIN(FORMULA);
		}

<FORMULA>"\\\n"	{
			/* continue on next line */
		}
<FORMULA>"if"[ \t]+	{
			/* Start processing "$if" directive */
			return if_dir;
		}
<FORMULA>"else"	{
			/* "else" directive */
			return else_dir;
		}
<FORMULA>"elseif"[ \t]+	{
			/* Start processing "$elseif" directive */
			return elseif_dir;
		}
<FORMULA>"end"	{
			/* End directive */
			return end_dir;
		}
<FORMULA>["]	{
			BEGIN(STRING);
		}
<STRING>"\\\\"	{
			yyleng--;
			yytext[yyleng-1] = '\\';
			yytext[yyleng] = 0;
			yymore();
		}
<STRING>"\\\""	{
			yyleng--;
			yytext[yyleng-1] = '"';
			yytext[yyleng] = 0;
			yymore();
		}
<STRING>"\\\n"	{
			yyleng -= 2;
			yytext[yyleng] = 0;	
			yymore();
		}
<STRING>"\\n"	{
			yyleng--;
			yytext[yyleng-1] = '\n';
			yytext[yyleng] = 0;	
			yymore();
		}
<STRING>"\\t"	{
			yyleng--;
			yytext[yyleng-1] = '\t';
			yytext[yyleng] = 0;	
			yymore();
		}
<STRING>"\""	{
			BEGIN(FORMULA);
			yytext[yyleng-1] = 0;
			if (yylval.str)
				free(yylval.str);
			yylval.str = strdup(yytext);
			return string;
		}
<STRING>"\n"	{
			sz_log(1, "Parse error: end-of-line in string");
			return parse_error;
		}
<STRING>.	{
			yymore();
		}
<FORMULA>"\n"	{
			/* end of directive line */
			BEGIN(INITIAL);
		}
<FORMULA>[ \t]	{
			/* ignore white spaces */
		}
<FORMULA>"<>"	{
			return not_equal_op;
		}
<FORMULA>"="	{
			return '=';
		}
<FORMULA>"("	{
			return '('; 
		}
<FORMULA>")"	{
			return ')';
		}
		
<FORMULA>":="	{
			return becomes;
		}

<FORMULA>"$"[a-zA-z_][a-zA-Z0-9_]*"$" {
			/* Variable */
			if (yylval.str)
				free(yylval.str);
			yylval.str = strdup(yytext+1);
			yylval.str[yyleng-2] = 0;
			return variable; 
		}

<FORMULA>[a-zA-z_][a-zA-Z0-9_]* {
			if (yylval.str)
				free(yylval.str);
			yylval.str = strdup(yytext);
			return id; 
		}
<FORMULA>.	{
		sz_log(1, "Parse error: unknown character (%s)", yytext);
			return parse_error;
		}
		
<INCLUDE>[ \t]*"\""	{
			/* eat up white spaces after "$include" */
			BEGIN(INCLUDE_NAME);
		}
<INCLUDE>.	{
			/* quoted file name expected */
		sz_log(1, "Parse error: file name expected (%s)", yytext);
			return parse_error;
		}

<INCLUDE_NAME>[^"\n]+"\""	{
			/* include file name catched */
			if (include_stack_ptr >= MAX_INCLUDE_DEPTH) {
			sz_log(1, "Parse error: include nested to deeply (%d)",
						include_stack_ptr);
				return parse_error;
			}
			include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;
			yytext[yyleng-1] = 0;
			yyin = fopen( yytext, "r" );
			if ( ! yyin ) {
			sz_log(1, "Parse error: cannot open file \"%s\" (errno %d)", yytext, errno);
					return parse_error;
			}
			yy_switch_to_buffer( yy_create_buffer (yyin, 
				YY_BUF_SIZE ) );
			BEGIN(INITIAL);
		}
			
<*><<EOF>> 	{
			/* EOF - step down in "include" stack */
			if (include_stack_ptr == 0) {
				if (yylval.str)
					free(yylval.str);
				yyterminate();
			} else {
				--include_stack_ptr;
				fclose(yyin);	
				yy_delete_buffer( YY_CURRENT_BUFFER );
				yy_switch_to_buffer(
					include_stack[include_stack_ptr] );
			}
		}


<INITIAL>":"[^ \t\n]+	{	
			/* Section name */
			if (yylval.str)
				free(yylval.str);
			yylval.str = strdup(yytext+1);
			return section;
		}

<INITIAL>[^ \t\n:#=]+"="	{
			/* Param name */
			BEGIN(PARAM);			
			yytext[yyleng-1] = 0;
			if (yylval.str)
				free(yylval.str); 
			yylval.str = strdup(yytext);
			return param;
		}

<PARAM>[^\n]*"\\\n"	{
			yyleng -= 2;
			yytext[yyleng] = 0;
			yymore();
		}

<PARAM>[^\n]*[\n] {
			/* End of param */
			yytext[yyleng-1] = 0;
			i = yyleng - 2;
			while ((i >= 0) && ((yytext[i] == ' ') || (yytext[i] == '\t')))
				yytext[i--] = 0;
			BEGIN(INITIAL);
			if (yylval.str)
				free(yylval.str);
			yylval.str = strdup(yytext);
			return param_content;
		}
<PARAM>[^\n]* {
			BEGIN(INITIAL);
			i = yyleng - 1;
			while ((i >= 0) && ((yytext[i] == ' ') || (yytext[i] == '\t')))
				yytext[i--] = 0;
			if (yylval.str)
				free(yylval.str);
			yylval.str = strdup(yytext);
			unput('\n');
			return param_content;
		}

<INITIAL>[ \t\n]	{
				/* ignore white spaces */
		}

<INITIAL>.	{
			/*fatal("Parser error: unknown character \"%s\" in line %d\n", yytext,  line_num);*/
		sz_log(1, "Parse error: unknown character (%s)", yytext);
			return parse_error;
		}

%%


