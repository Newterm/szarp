/* 
  SZARP: SCADA software 

*/
/*
 * IPK
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * ISO8859-2 / UTF8 conversion functions.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "latin2.h"
#include "liblog.h"
#include <iconv.h>
#include <string.h>

static int sz_toUTF8_bufsize = 0;
static char *sz_toUTF8_buffer = NULL;

xmlChar *toUTF8(const char *strorig)
{
	char *strbuff = strdup(strorig);
	char *str = strbuff;
	
	iconv_t cd;
	static char *tmp;
	size_t i, o, r;
	
	if (!str)
		return NULL;
	i = strlen(str) + 1;
	if ((i * 2) > sz_toUTF8_bufsize) {
		sz_toUTF8_bufsize = i * 2;
		sz_toUTF8_buffer = (char*)realloc(
				sz_toUTF8_buffer, 
				sz_toUTF8_bufsize * sizeof(char));
	}
	tmp = sz_toUTF8_buffer;
	o = sz_toUTF8_bufsize;

#ifdef MINGW32
	cd = iconv_open("UTF-8", "CP1250");
#else
	cd = iconv_open("UTF-8", "ISO-8859-2");
#endif
	r = iconv(cd, &str, &i, &tmp, &o);
	iconv_close(cd);
	if (r < 0) {
		strcpy(sz_toUTF8_buffer, "ERR");
	} 
	sz_toUTF8_buffer[sz_toUTF8_bufsize - 1] = 0;
	free(strbuff);
	return (xmlChar*)sz_toUTF8_buffer;
}

unsigned char* fromUTF8(unsigned char *str)
{
	iconv_t cd;
	size_t i, o, r;
	char *tmp, *_tmp; 
	char *_str = (char*)str;
	
	if (!str)
		return NULL;
	i = strlen(_str) + 1;

	tmp = _tmp = (char*)malloc(i);
	o = i;
	
#ifdef MINGW32
	cd = iconv_open("CP1250", "UTF-8");
#else
	cd = iconv_open("ISO-8859-2", "UTF-8");
#endif
	r = iconv(cd, &_str, &i, &_tmp, &o);
	iconv_close(cd);
	if (r == (size_t) -1) {
		*str = 0;
	} else { 
		strcpy((char*)str, tmp);
	}
	free(tmp);

	return str;
}

