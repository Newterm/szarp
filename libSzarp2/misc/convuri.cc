/* 
  SZARP: SCADA software 

*/
/*
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * Function for unescaping HTTP URI strings.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "convuri.h"

char *convURI(const char *uri)
{
	char *buf;
	int i, j, k, l;

	buf = (char *) malloc (sizeof(char) * (strlen(uri)+1));

	i = 0;
	j = 0;
	while (uri[i] != 0) {
		if (uri[i] == '+') {
			buf[j++] = ' ';
			i++;
			continue;
		}
		if (uri[i] != '%') {
			buf[j++] = uri[i++];
			continue;
		}
		l = uri[i+1];
		if (l == 0)
			break;
		k = uri[i+2];
		if (k == 0)
			break;
		if ((l >= '0') && (l <= '9'))
			l = l - '0';
		else if (l >= 'A' && l <= 'F')
			l = l - 'A' + 10;
		else {
			buf[j++] = uri[i++];
			buf[j++] = uri[i++];
			continue;
		}
		if ((k >= '0') && (k <= '9'))
			k = k - '0';
		else if (k >= 'A' && k <= 'F')
			k = k - 'A' + 10;
		else {
			buf[j++] = uri[i++];
			buf[j++] = uri[i++];
			buf[j++] = uri[i++];
			continue;
		}
		buf[j++] = l * 16 + k;
		i += 3;
	}
	buf[j] = 0;
	return buf;
}

