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
 * $Id$
 */

#ifndef __DRAWURL_H__
#define __DRAWURL_H__

/**Decodes window reference from url
 * @param url url to decode
 * @param prefix output param, decoded prefix
 * @param window output param, decoded window
 * @param pt output param decoded period
 * @param time output param decoded time
 * @return true if url was successfully decoded. i.e. has proper syntax*/
bool decode_url(wxString url, wxString& prefix, wxString &window, PeriodType& pt, time_t& time, int& selected_draw, bool xml = false);

/**Decodes string encoded in 'percent chars' notation. I don't know what's that notation name, but it is used for encoding non-printable 
 * (and some printable :)) charaters in urls.*/
wxString decode_string(const unsigned char *c);

wxString decode_string(wxString str);

wxString encode_string(const wxString &toc, bool allow_colon = false);

#endif
