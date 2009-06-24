/* 
  SZARP: SCADA software 

*/
/*
 * IPK
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * ISO8859-2 or CP1250 / UTF8 conversion functions.
 *
 * $Id$
 */

#ifndef __LATIN2_H__
#define __LATIN2_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <libxml/tree.h>
/**
 * Converts ISO-8859-2 (or CP1250 - under Windows) string to UTF8 encoding.
 * It's not thread safe! It uses static growable buffer. On error it
 * returns string "ERR" and writes error message to log.
 * @param str string to recode
 * @return pointer to static buffer containing recoded string, NULL if str was
 * NULL
 */
xmlChar *toUTF8(const char *str);

/**
 * Recodes UTF-8 coded string to ISO-8859-2 or CP1250. 
 * Changes argument in place.
 * @param str UTF-8 encoded string
 * @return pointer to param string
 */
unsigned char *fromUTF8(unsigned char *str);

#ifdef __cplusplus
	}
#endif

#endif

