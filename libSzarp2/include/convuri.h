/* 
  SZARP: SCADA software 

*/
/*
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * Function for unescaping HTTP URI strings.
 *
 * $Id$
 */

#ifndef __CONVURI_H__
#define __CONVURI_H__

#ifdef __cplusplus
	extern "C" {
#endif

/**
 * Converts '%xx' entities in string to corresponding values. Also replaces
 * '+' with spaces.
 * @param uri original string
 * @return newly allocated copy of parametr, with '%xx' entities converted to
 * corresponding chars, it's up to user to free the memory
 */
char *convURI(const char *uri);

#ifdef __cplusplus
	}
#endif

#endif
