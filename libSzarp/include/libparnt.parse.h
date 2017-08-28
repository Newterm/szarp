/* 
  libSzarp - SZARP library

*/
/*
 * $Id$
 *
 * Pawe³ Pa³ucha (pawel@praterm.com.pl)
 * 25.04.2001
 *
 * SZARP 2.1
 *
 * Plik nag³owkowy parsera plików konfiguracyjnych SZARP'a.
 *
 */

typedef union {
	int ival;
	char *str;
} YYSTYPE;

#define	if_dir 		301
#define else_dir	302
#define elseif_dir	303
#define end_dir		304
#define string_token		305
#define parse_error	306
#define not_equal_op	307
#define section_token		308
#define param_token		309
#define param_content	310
#define id_token		311
#define variable	312
#define becomes		313

/**< Frees flex memory (Flex 2.5.9 required) */
void parser_destroy(void);

