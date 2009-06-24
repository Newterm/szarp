/* 
  SZARP: SCADA software 

*/
/*
 *  Pawe³ Pa³ucha 2002
 * Include file for definable parser.
 *
 * $Id$
 */

#ifndef __DEFINABLE_PARSER_H__
#define __DEFINABLE_PARSER_H__

#include "ptt_act.h"
		
typedef struct definable_str definable_t;
/** Definable params' list structure. */
struct definable_str {
	PTT_param_t param;
			/**< Param data. */
	char *formula;
			/**< Param's formula */
	definable_t *next;
			/**< Next list element */
};

/** Definable params' table structure. */
struct definable_table_str {
	int params;	/**< All definable params count. */
	int offset;	/**< Offset of definable params. */
	definable_t *definable_list;
			/**< Params' list. */
};
typedef struct definable_table_str definable_table_t;

/**
 * Main parser function. Returns parsed definable params' table.
 * @param def_path path to definable file
 * @return pointer to newly created definable params' table; NULL on parse 
 * error or when file is not found
 */
definable_table_t *parse_definable(const char *definable_path);

/**
 * Free memory used by definable_table structure
 */
void free_definable(definable_table_t *d);

int deflex(void);

#endif
