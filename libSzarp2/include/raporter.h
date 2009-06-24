/* 
  SZARP: SCADA software 

*/
/*
 *  Pawe³ Pa³ucha 2002
 * Include file for Raporter configuration parser.
 *
 * $Id$
 */

#ifndef __RAPORTER_H__
#define __RAPORTER_H__

/**
 * Single element of raport items' list.
 */
typedef struct phRaportItem tRaportItem;
struct phRaportItem {
	int ipcInd;	/**< IPC index of param */
	int prec;	/**< Precision of param's representation. */
	char *shortName;
			/**< Param's short name */
	char *desc;	/**< Param description for raport */
	tRaportItem* next;
			/**< Next list element. */
};
/**
 * Info about raport.
 */
typedef struct phRaport tRaport;
struct phRaport {
	char *name;	/**< Raport name */
	tRaportItem* items;
			/**< List of raport items. */
};
	
/**
 * Main parser function. Returns parsed raport info.
 * @param path path to *.rap file
 * @return pointer to newly created raport info; NULL on parse error or when
 * file is not found
 */
tRaport *parse_raport(const char *path);

/** Free memory used by tRaport structure. */
void free_raport(tRaport *r);

#endif
