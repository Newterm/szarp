/* 
  SZARP: SCADA software 

*/
/*
 *  Pawe³ Pa³ucha 2002
 * Include file for SZARP DRAW 2.1 configuration parser.
 *
 * $Id$
 */

#ifndef __EKRNCOR_H__
#define __EKRNCOR_H__

#ifdef __cplusplus
	extern "C" {
#endif

/** Draw info */
typedef struct phDraw tDraw;
struct phDraw {
	char *shortName;	/**< Short name of param */
	char *name;		/**< Name of param draw */
	int prec;		/**< Precision of param value (scaling factor) 
				 */
	int color;		/**< Index of params color */
	int field;		/**< Field in base, starting from 0, -1 for
				params woth auto index */
	int rec;		/**< Record in base, starting from 1,
				-1 for param with auto index */
	char *str;		/** Full name of param for params with 
				  auto index, NULL for others */
	char *unit;		/**< Name of params unit */
	int axis;		/**< Index of axis (from 0) */
	double data;		/**< Aditional data (for internal use) */
};

/** Axis info */
typedef struct phAxisInfo tAxisInfo;
struct phAxisInfo {
	double minVal;		/**< Minimal value on axis */
	double maxVal;		/**< Maximal value of axis */
	int scaleProc;		/**< Percent of axis, which should be used
				  for [scaleMin, scaleMax] sector. If 0, no 
				  rescaling is made */
	double scaleMin;		/**< Start of rescaling sector */
	double scaleMax;		/**< End of rescaling sector */
};

/**
 * Single element of windows' list.
 */
typedef struct phDrawWindow tDrawWindow;
struct phDrawWindow {
	unsigned long id;	/**< Special bits info */
	char* title;		/**< Title of window */
	int axesCount;		/**< Number of axes */
	tAxisInfo* axes;	/**< Window axis */
	int firstDraw;		/**< Number of first draw */
	int drawsCount;		/**< Number of draws */
	tDraw* draws;		/**< Array of draws */
};

/** SZARP Draw 2.1 configuration info */
typedef struct phDrawInfo tDrawInfo;
struct phDrawInfo {
	char* title;		/**< Title of draw window */
	int windowsCount;	/**< Number of windows */
	tDrawWindow* windows;	/**< Array of windows */
};

/** Parses SzarpDraw configuration file.
 * @param path path to configuration file
 * @param is_cor 1 if it's 'cor' file type (with title in first line),
 * 0 if it's 'def' file (without title)
 * @return parsed configuration info, NULL on error
 */
tDrawInfo *parse_ekrncor(char *path, int is_cor);

/** Free memory used by tDrawinfo structure */
void free_ekrncor(tDrawInfo *e);

#ifdef __cplusplus
	}
#endif

#endif

// vim: ft=c:
