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

#include "cconv.h"
#include "ids.h"
#include "classes.h"
#include "drawdnd.h"
#include "drawurl.h"

const wxChar* DrawInfoDataObject::draw_info_format_id = _T("draw_info_pointer");

DrawInfoDataObject::DrawInfoDataObject(DrawInfo *draw) {
	m_draw = draw;
	m_draw_format.SetId(draw_info_format_id);
}

wxDataFormat DrawInfoDataObject::GetPreferredFormat(Direction dir) const {
	return m_draw_format;
}

size_t DrawInfoDataObject::GetFormatCount(Direction dir) const {
	return 1;
}

void DrawInfoDataObject::GetAllFormats(wxDataFormat *formats, Direction dir) const {
	formats[0] = m_draw_format;
}

size_t DrawInfoDataObject::GetDataSize(const wxDataFormat& format) const {
	if (format != m_draw_format)
		assert(false);

	return sizeof(m_draw);
}

bool DrawInfoDataObject::GetDataHere(const wxDataFormat& format, void *pbuf) const {
	if (format != m_draw_format)
		return false;

	memcpy(pbuf, &m_draw, sizeof(m_draw));

	return true;
}

bool DrawInfoDataObject::SetData(const wxDataFormat& format, size_t len, const void *buf) {
	if (format != m_draw_format)
		return false;

	if (len != sizeof(m_draw))
		return false;

	memcpy(&m_draw, buf, sizeof(m_draw));

	return true;
}

DrawInfo* DrawInfoDataObject::GetDrawInfo() const {
	return m_draw;
}

DrawInfoDropTarget::DrawInfoDropTarget(DrawInfoDropReceiver *receiver) : wxDropTarget(new DrawInfoDataObject) {
	m_receiver = receiver;
}

wxDragResult DrawInfoDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult def) {
	if (GetData() == false)
		return wxDragNone;

	DrawInfoDataObject* did =  (DrawInfoDataObject*)GetDataObject();

	return m_receiver->SetDrawInfo(x, y, did->GetDrawInfo(), def);
}

SetInfoDataObject::SetInfoDataObject(wxString prefix, wxString window, PeriodType period, time_t time, int selected_draw) :
		m_prefix(prefix),
		m_set(window), 
		m_period(period),
		m_time(time),
		m_selected_draw(selected_draw)
{}

wxDataFormat SetInfoDataObject::GetPreferredFormat(Direction dir) const {
	return wxDataFormat(wxDF_TEXT);
}

size_t SetInfoDataObject::GetFormatCount(Direction dir) const {
	return 3;
}

void SetInfoDataObject::GetAllFormats(wxDataFormat *formats, Direction dir) const {
	formats[0] = wxDataFormat(wxDF_TEXT);
	formats[1] = wxDataFormat(wxDF_FILENAME);
	formats[2] = wxDataFormat(wxDF_UNICODETEXT);
//	formats[3] = wxDataFormat(wxDF_OEMTEXT);
}

wxString SetInfoDataObject::GetUrl() const {
	wxString url = _T("draw:/");
	url << _T("/") << m_prefix;
	url << _T("/") << m_set;

	url << _T("/");
	switch (m_period) {
		case PERIOD_T_YEAR:
			url << _T("Y");
			break;
		case PERIOD_T_MONTH:
			url << _T("M");
			break;
		case PERIOD_T_WEEK:
			url << _T("W");
			break;
		case PERIOD_T_SEASON:
			url << _T("S");
			break;
		case PERIOD_T_DAY:
			url << _T("D");
			break;
		case PERIOD_T_30MINUTE:
			url << _T("10M");
			break;
		default:
			assert(false);
	}

#ifndef __WXMSW__
	url << _T("/") << m_time;
#else
	url << _T("/");
	wxString tstr;
	time_t t = m_time;
	while (t) {
		int digit = t % 10;
		tstr = wxString::Format(_T("%d"), digit) + tstr;
		t /= 10;
	}
	url << tstr;
#endif
	url << _T("/") << m_selected_draw;

	return encode_string(url, true);

}

size_t SetInfoDataObject::GetDataSize(const wxDataFormat& format) const {
	if (format.GetType() != wxDF_TEXT 
			&& format.GetType() != wxDF_FILENAME
//			&& format.GetType() != wxDF_OEMTEXT
			&& format.GetType() != wxDF_UNICODETEXT)
		assert(false);

	wxString url = GetUrl();

	size_t ret = url.length();

	if (format.GetType() == wxDF_FILENAME)
		ret += 1;

	return ret;
}

bool SetInfoDataObject::GetDataHere(const wxDataFormat& format, void *pbuf) const {
	if (format.GetType() != wxDF_TEXT 
			&& format.GetType() != wxDF_FILENAME
//			&& format.GetType() != wxDF_OEMTEXT
			&& format.GetType() != wxDF_UNICODETEXT)
		return false;

	wxString url = GetUrl();

	char *curl = strdup((char*)SC::S2U(url).c_str());
	size_t len = strlen(curl);
	if (format.GetType() == wxDF_FILENAME)
		len += 1;

	memcpy(pbuf, curl, len);
	free(curl);

	return true;
}

bool SetInfoDataObject::SetData(const wxDataFormat& format, size_t len, const void *buf) {
	if (format.GetType() != wxDF_TEXT 
			&& format.GetType() != wxDF_FILENAME
//			&& format.GetType() != wxDF_OEMTEXT
			&& format.GetType() != wxDF_UNICODETEXT)
		return false;

	unsigned char *escaped = (unsigned char*)malloc(len + 1);
	memcpy(escaped, buf, len);
	escaped[len] = '\0';
	wxString url = decode_string(escaped);
	free(escaped);

	url.Trim(true);
	url.Trim(false);

	if (!decode_url(url, m_prefix, m_set, m_period, m_time, m_selected_draw))
		return false;

	return true;
}

SetInfoDropTarget::SetInfoDropTarget(SetInfoDropReceiver *receiver) : wxDropTarget(new SetInfoDataObject(_T(""), _T(""), PERIOD_T_OTHER, -1, -1)) {
	m_receiver = receiver;
}

wxDragResult SetInfoDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult def) {
	if (GetData() == false)
		return wxDragNone;

	SetInfoDataObject* wid = (SetInfoDataObject*)GetDataObject();

	return m_receiver->SetSetInfo(x, 
			y, 
			wid->GetSet(), 
			wid->GetPrefix(),
			wid->GetTime(),
			wid->GetPeriod(),
			def);
}
