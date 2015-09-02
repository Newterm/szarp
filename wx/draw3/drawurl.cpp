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

#include <wx/wx.h>
#include <wx/tokenzr.h>
#include <libxml/tree.h>

#include <limits>

#include "ids.h"
#include "classes.h"

#include "string.h"
#include "cconv.h"
#include "drawurl.h"

namespace {

std::basic_string<unsigned char> unescape(const std::basic_string<unsigned char>& c) {
	int s = 0;
	unsigned char v = 0;
	unsigned char ch = 0;
	const unsigned char *i = c.c_str();
	std::basic_string<unsigned char> ret;

	while (*i) {
		ch = *i;
		switch (s) {
			case 0:
				if (ch != '%')
					ret += ch;
				else
					s = 1;
				break;
			case 1:
				ch = tolower(ch);
				if (ch <= '9')
					v = (ch - '0') << 4;
				else
					v = (ch - 'a' + 10) << 4;

				s = 2;
				break;
			case 2:
				ch = tolower(ch);
				if (ch <= '9')
					v |= (ch - '0');
				else
					v |= (ch - 'a' + 10);
				ret += v;
				s = 0;
				break;
		}
		i++;
	}

	return ret;

}

}

wxString decode_string(const unsigned char *c) {
	return SC::U2S(unescape(c));
}

wxString decode_string(wxString s) {
	std::basic_string<unsigned char> c = SC::S2U(s);
	return SC::U2S(unescape(c));
}

wxString decode_string_xml(wxString s) {
	std::basic_string<unsigned char> c = SC::S2U(s);
	return SC::U2S(unescape(c));
}

wxString encode_string(const wxString &toc, bool allow_colon) {
	wxString ret;

	std::basic_string<unsigned char> uc = SC::S2U(toc);

	for (size_t i = 0; i < uc.size(); i++) {
		switch (uc[i]) {
			case '0'...'9':
			case 'A'...'Z':
			case 'a'...'z':
			case '/':
				ret += wchar_t(uc[i]);
				break;
			default:
				if (allow_colon && uc[i] == ':')
					ret += L':';
				else 
					ret << wxString::Format(_T("%%%2x"), (unsigned) uc[i]);
				break;
		}
	}

	return ret;
}

bool decode_url(wxString surl, wxString& prefix, wxString &window, PeriodType& period, time_t& time, int& selected_draw, bool xml) {


	wxString url;
	if(xml)
		url = decode_string_xml(surl);
	else
		url = decode_string(surl);
	selected_draw = -1;

	url.Trim(true);
	url.Trim(false);

	/* draw://<prefix>/<window>/<period>/time */
	wxStringTokenizer tok(url, _T("/"));

	if (tok.CountTokens() != 6 && tok.CountTokens() != 7)
		return false;

	tok.GetNextToken();
	tok.GetNextToken();

	prefix = tok.GetNextToken();
	window = tok.GetNextToken();

	wxString pstr = tok.GetNextToken();
	if (pstr == _T("E")) 
		period = PERIOD_T_DECADE;
	else if (pstr == _T("Y")) 
		period = PERIOD_T_YEAR;
	else if (pstr == _T("M"))
		period = PERIOD_T_MONTH;
	else if (pstr == _T("W"))
		period = PERIOD_T_WEEK;
	else if (pstr == _T("S"))
		period = PERIOD_T_SEASON;
	else if (pstr == _T("D"))
		period = PERIOD_T_DAY;
	else if (pstr == _T("10M"))
		period = PERIOD_T_30MINUTE;
	else if (pstr == _T("5M"))
		period = PERIOD_T_3MINUTE;
	else if (pstr == _T("MIN"))
		period = PERIOD_T_MINUTE;
	else if (pstr == _T("3SEC"))
		period = PERIOD_T_3SEC;
	else
		return false;

	long rtime;
	if (tok.GetNextToken().ToLong(&rtime) == false)
		return false;

	wxDateTime dt(rtime);
	if (rtime <= 0 || 
#ifdef __WXMSW__
		 	rtime >= 2147483647
#else
			rtime == std::numeric_limits<time_t>::max()
#endif
			|| dt.GetYear() < 1980 || dt.GetYear() > 2037)
		time = wxDateTime::Now().GetTicks();
	else
		time = rtime;

	if (tok.HasMoreTokens()) {
		long tmpval;
		if (tok.GetNextToken().ToLong(&tmpval) == false)
			return false;
		selected_draw = tmpval;
	}

	return true;
}

