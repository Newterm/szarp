/* 
  SZARP: SCADA software 

*/
/*
 *  Pawe³ Pa³ucha 2002
 * Include file for line<x>.cfg parser.
 *
 * $Id$
 */

#ifndef __LINE_CFG_H__
#define __LINE_CFG_H__

#ifdef __cplusplus
	extern "C" {
#endif

/**
 * Info about single comunication unit.
 */
typedef struct phUnit tUnit;
struct phUnit {
	char id;	/**< Unit id within line */
	int type;	/**< Raport type */
	int subtype;	/**< Raport subtype */
	int bufsize;	/**< Av. buffer size */
	int input;	/**< Number of input params */
	int output;	/**< Number of output params */
	tUnit *next;	/**< Next list element */
};
		
typedef struct phRadio tRadio;		
struct phRadio {
	char *id;	/**< Radio modem id */
	int num;	/**< Number of units within radio line */
	tUnit *units;	/**< Radio line info */
	tRadio *next;	/**< Next list element */
};

/**
 * Information about communication units within line
 */
struct phLineConf {
	int is_radio;
	int special;
	union {
		struct {
			int num;
			tRadio	*radios;
		} radio;
		struct {
			int num;
			int special_value;
			tUnit	*units;
		} units;
	} data;
};
typedef struct phLineConf tLineConf;

tLineConf *parse_line_cfg(const char *path);

void free_line_cfg(tLineConf *c);

int lcfglex(void);

#ifdef __cplusplus
	}
#endif

#endif
