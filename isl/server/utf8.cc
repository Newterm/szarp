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
 * utf8.c - UTF8 recoding.
 * 
 * $Id$
 */

#include <ctype.h>
#include <string.h>
#include "utf8.h"

/**
 * Converts string ISO-8859-2 coded to "link" version (only ASCII, without
 * spaces).
 */
void lat2a(char *c)
{
	int i;

	if (!c)
		return;
	for (i = 0; c[i]; i++)
		switch (c[i]) {
			case '±' :
				c[i] = 'a';
				break;
			case '¡' :
				c[i] = 'A';
				break;
			case 'æ' :
				c[i] = 'c';
				break;
			case 'Æ' :
				c[i] = 'C';
				break;
			case 'ê' :
				c[i] = 'e';
				break;
			case 'Ê' :
				c[i] = 'E';
				break;
			case '³' :
				c[i] = 'l';
				break;
			case '£' :
				c[i] = 'L';
				break;
			case 'ñ' :
				c[i] = 'n';
				break;
			case 'Ñ' :
				c[i] = 'N';
				break;
			case 'ó' :
				c[i] = 'o';
				break;
			case 'Ó' :
				c[i] = 'O';
				break;
			case '¶' :
				c[i] = 's';
				break;
			case '¦' :
				c[i] = 'S';
				break;
			case '¼' :
			case '¿' :
				c[i] = 'z';
				break;
			case '¬' :
			case '¯' :
				c[i] = 'Z';
				break;
			default :
				if (((unsigned char )c[i] > 127) || (!isalnum(c[i]))) {
					c[i] = '_';
				} 
				break;
		}
}

/**
 * Recodes UTF-8 coded string to pure ASCII representation, without spaces. 
 * Changes argument in place.
 * @param uri UTF-8 encoded string
 */
void utf2a(unsigned char *str)
{
	int i, j;

	if (!str)
		return;
	i = 0; j = 0;
	while (str[i] != 0) {
		if (str[i] < 128) {
			if (!isalnum(str[i])) {
				str[j] = '_';
			} else {
				str[j] = str[i];
			}
			i++; j++;
			continue;
		}
		if (str[i] == 196) {
			if (str[i+1] == 133) {
				str[j] = 'a';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 132) {
				str[j] = 'A';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 135) {
				str[j] = 'c';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 134) {
				str[j] = 'C';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 153) {
				str[j] = 'e';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 152) {
				str[j] = 'E';
				j++; i+= 2;
				continue;
			}
		}
		if (str[i] == 197) {
			if (str[i+1] == 130) {
				str[j] = 'l';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 129) {
				str[j] = 'L';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 132) {
				str[j] = 'n';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 131) {
				str[j] = 'N';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 155) {
				str[j] = 's';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 154) {
				str[j] = 'S';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 186) {
				str[j] = 'z';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 185) {
				str[j] = 'Z';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 188) {
				str[j] = 'z';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 187) {
				str[j] = 'Z';
				j++; i+= 2;
				continue;
			}
		}
		if (str[i] == 195) {
			if (str[i+1] == 179) {
				str[j] = 'o';
				j++; i+= 2;
				continue;
			}
			if (str[i+1] == 147) {
				str[j] = 'O';
				j++; i+= 2;
				continue;
			}
		}
		str[j++] = str[i++];
	}
	str[j] = 0;
}

/**
 * Compares two string. First one is expected to have UTF-8 char encoding, the
 * second one - UTF-8 or ISO-8859-2 or plain ASCII. Only Polish specific characters are
 * recognized. It's used for URI parsing.
 * @param uri UTF-8 encoded string
 * @param name ISO-8859-2 or ASCII encoded string
 * @return 0 if string are equal, +1 or -1 if not (don't expect this to be a
 * locale based comparison)
 */

int cmpURI(const unsigned char *uri, const unsigned char *name)
{
	int i, j;
	
	if (!strcmp((char*)uri,(char*)name)) {
		return 0;
	}
	i = 0; j = 0;
	while ((uri[i] != 0) && (name[j] != 0)) {
		if (uri[i] < 128) {
			if (uri[i] == name[j]) {
				i++; j++;
				continue;
			} else if ((name[j] == '_') && !isalnum(uri[i])) {
				i++; j++;
				continue;
			}
			break;
		}
		
		if (uri[i] >= 195 || uri[i] <= 197) {
			if (uri[i] == name[j] && uri[i + 1] == name[j + 1]) {
				if (name[j + 1] == '\0') {
					i++;
					j++;
				} else {
					i += 2;
					j += 2;
					continue;
				}
			}
		}

		if (uri[i] == 196) {
			if (uri[i+1] == 133) {
				if ((name[j] == (unsigned char)'±') ||	
						(name[j] =='a')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 132) {
				if ((name[j] == (unsigned char)'¡') 
						|| (name[j] =='A')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 135) {
				if ((name[j] == (unsigned char)'æ')
						|| (name[j] =='c')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 134) {
				if ((name[j] == (unsigned char)'Æ') 
						|| (name[j] =='C')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 153) {
				if ((name[j] == (unsigned char)'ê')
						|| (name[j] =='e')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 152) {
				if ((name[j] == (unsigned char)'Ê') 
						|| (name[j] =='E')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			break;
		}
		if (uri[i] == 197) {
			if (uri[i+1] == 130) {
				if ((name[j] == (unsigned char)'³') 
						|| (name[j] =='l')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 129) {
				if ((name[j] == (unsigned char)'£') 
						|| (name[j] =='L')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 132) {
				if ((name[j] == (unsigned char)'ñ') 
						|| (name[j] =='n')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 131) {
				if ((name[j] == (unsigned char)'Ñ') 
						|| (name[j] =='N')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 155) {
				if ((name[j] == (unsigned char)'¶') 
						|| (name[j] =='s')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 154) {
				if ((name[j] == (unsigned char)'¦') 
						|| (name[j] =='S')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 186) {
				if ((name[j] == (unsigned char)'¼') 
						|| (name[j] =='z')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 185) {
				if ((name[j] == (unsigned char)'¬') 
						|| (name[j] =='Z')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 188) {
				if ((name[j] == (unsigned char)'¿') 
						|| (name[j] =='z')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 187) {
				if ((name[j] == (unsigned char)'¯') 
						|| (name[j] =='Z')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			break;
		}
		if (uri[i] == 195) {
			if (uri[i+1] == 179) {
				if ((name[j] == (unsigned char)'ó') 
						|| (name[j] =='o')) {
					i+=2; j++;
					continue;
				}
				break;
			}
			if (uri[i+1] == 147) {
				if ((name[j] == (unsigned char)'Ó') 
						|| (name[j] =='O')) {
					i+=2; j++;
					continue;
				}
				break;
			}
		}
		break;
	}
	
	return !(name[j] == '\0' && uri[i] == '\0');
}

