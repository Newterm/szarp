/* 
  SZARP: SCADA software 

*/
/*
 *  Pawe³ Pa³ucha 2002
 * Include file for PTT.act parser.
 *
 * $Id$
 */

#ifndef __PTT_ACT_H__
#define __PTT_ACT_H__

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct PTT_param_str PTT_param_t;
/** Params' list structure. */
struct PTT_param_str {
	int base_ind;	/**< Index of param in base (starting with 1). */
	int ipc_ind;	/**< Index of param in shared memory (starting with 
			  0). */
	char *short_name;
			/**< Short param's name. */
	char *full_name;
			/**< Full param's name. */
	char *draw_name;
			/**< Alternative name for 'draw' program. */
	int prec;	/**< Precision (dott posision). */
	char *unit;	/**< Param's unit. */
	PTT_param_t *next;
			/**< Next list element. */
};

/** Params' table structure. */
struct PTT_table_str {
	int params;	/**< All params count. */
	int baseparams;	/**< How many params are saved to base. */
	PTT_param_t *param_list;
			/**< Params' list. */
};
typedef struct PTT_table_str PTT_table_t;

PTT_table_t *parse_PTT(char *ptt_path);

int ptplex(void);

/** Free memory used by PTT_param_t structure */
void free_param(PTT_param_t *p);

/** Free memory used by PTT_table_t structure */
void free_param_table(PTT_table_t *t);

#ifdef __cplusplus
	}
#endif

#endif
