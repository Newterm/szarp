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
#ifndef __LUA_STRINGS_EXTRACT_H__
#define __LUA_STRINGS_EXTRACT_H__

#ifdef LUA_PARAM_OPTIMISE

bool extract_strings_from_formula(const std::wstring& formula, std::vector<std::wstring> &strings);

#endif /* LUA_PARAM_OPTIMISE */

#endif /* __LUA_STRINGS_EXTRACT_H__ */
