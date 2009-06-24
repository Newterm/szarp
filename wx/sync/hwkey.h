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
 * hwkey - Hardware Key Generator 
 * SZARP

 * Michal Blajerski <nameless@praterm.com.pl>
 *
 * $Id$
 */

#ifndef _HWKEY_H
#define _HWKEY_H

#define XOR 255
#define SPECIAL_WINDOWS_KEY "0000000000"
#define SPECIAL_LINUX_KEY "6666666666"

class HardwareKey {
public:
	HardwareKey();
	char * GetKey();
protected:
	void GenKey();
	char key[11];
};

#endif // _HWKEY_H


