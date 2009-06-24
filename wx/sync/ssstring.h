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
#ifndef __SSSTRING_H_
#define __SSSTRING_H_

/**Some part of the code is shared by server and client.
 * Client uses wxStrings and server uses good old std::string
 * is these part*/
#ifdef USE_WX
#include <wx/string.h>
#include <cconv.h>

typedef wxString ssstring;
/**converts char* to wxString*/
#define csconv(string) (wxString(SC::A2S(string)))
/**converts wxString to const char**/
#define scconv(string) (SC::S2A(string).c_str())

#else //USE_WX

#include <string>

typedef std::string ssstring;
#define _(arg) arg
/**empty operation*/
#define csconv(string) string
/**converts std::string to char* */
#define scconv(str) (std::string(str).c_str())

#endif //USE_WX

#endif //__SSSTRING_H_
