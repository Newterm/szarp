/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/*
 * Pawe³ Pa³ucha 2002
 * HTTP base64 decoding
 *
 * $Id$
 *
 * This code is taken from PHP version 4 sources, by Jim Winstead
 * (jimw@php.net).
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>

#include "base64.h"

/** Initial table for performing base64 decoding. */
static char base64_table[] =
	{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	  'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', '\0'
	};
/** Base64 padding character. */
static char base64_pad = '=';

/**
 * Function decodes string coded in base64 HTTP encoding.
 * @param str encoded string, 0 terminated
 * @return newly allocated buffer containing decoded string, 0 terminated; it's
 * up to user to free the memory; on error NULL is returned (no memory or
 * invalid string) */
unsigned char *base64_decode(const unsigned char *str) 
{
	const unsigned char *current = str;
	int ch, i = 0, j = 0, k;
	int length;
	unsigned char *result;
	char reverse_table[256];
	char *chp;

	length = strlen((char*)str);
	for(ch = 0; ch < 256; ch++) {
		chp = strchr(base64_table, ch);
		if (chp) 
			reverse_table[ch] = chp - base64_table;
		else
			reverse_table[ch] = -1;
	}

	result = (unsigned char *) malloc(length + 1);
	if (result == NULL) 
		return NULL;

	while ((ch = *current++) != '\0') {
		if (ch == base64_pad) break;

		if (ch == ' ') ch = '+'; 

		ch = reverse_table[ch];
		if (ch < 0) continue;

		switch(i % 4) {
		case 0:
			result[j] = ch << 2;
			break;
		case 1:
			result[j++] |= ch >> 4;
			result[j] = (ch & 0x0f) << 4;
			break;
		case 2:
			result[j++] |= ch >>2;
			result[j] = (ch & 0x03) << 6;
			break;
		case 3:
			result[j++] |= ch;
			break;
		}
		i++;
	}

	k = j;
	if (ch == base64_pad) {
		switch(i % 4) {
		case 0:
		case 1:
			free(result);
			return NULL;
		case 2:
			k++;
		case 3:
			result[k++] = 0;
		}
	}
	result[k] = '\0';
	return result;
}

