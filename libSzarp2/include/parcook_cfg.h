/* 
  SZARP: SCADA software 

*/
/*
 *  Pawe³ Pa³ucha 2002
 * Include file for parcook.cfg parser.
 *
 * $Id$
 */

#ifndef __PARCOOK_CFG_H__
#define __PARCOOK_CFG_H__

#ifdef __cplusplus
	extern "C" {
#endif

/**
 * Information about single communication line
 */
struct phLineInfo
{
	unsigned char lineNum;	/**< Line identifier */
	int parBase;		/**< Number of first parameter in line */
	int parTotal;		/**< Number of parameters in line */
	char *daemon;		/**< Path to daemon responsible for line */
	char *device;		/**< Controlled device (path) */
	int speed;		/**< Transmision speed (in bauds) */
	int stop;		/**< Stop bits number */
	int protocol;		/**< Protocol identifier */
	char *options;		/**< Extra options */
	int argc;		/**< Number of defined args for daemon 
				  (including path to daemon, without options). */
};
typedef struct phLineInfo tLineInfo;

/**
 * Type for formulas
 */
typedef char * tFormula;

/**
 * Information from parcook.cfg file
 */
struct phParcookInfo
{
	int linesCount;		/**< Number of lines */
	int lineParamsCount;	/**< Number of parameters in all lines */
	int freq;		/**< Read frequency (in seconds) */
	int definableCount;	/**< Number of definable parameters */
	int formulasCount;	/**< Number of formulas defining params */
	tLineInfo **lineInfo;	/**< Lines information */
	tFormula *formulas;	/**< Formulas defining parameters */
};
typedef struct phParcookInfo tParcookInfo;

tParcookInfo *parse_parcook_cfg(const char *path);

/** Free memory used by structure */
void free_parcook_cfg(tParcookInfo *p);

int pcfglex(void);

#ifdef __cplusplus
	}
#endif

#endif
