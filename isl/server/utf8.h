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
 * utf8.h - UTF8 recoding.
 * 
 * $Id$
 */

#ifndef __UTF8_H_ZAZOLCGESLAJAZN__
#define __UTF8_H_ZAZOLCGESLAJAZN__

#ifdef __cplusplus
	extern "C" {
#endif
		
void lat2a(char *c);

void utf2a(unsigned char *str);

int cmpURI(const unsigned char *uri, const unsigned char *name);

#ifdef __cplusplus
	}
#endif

#endif

