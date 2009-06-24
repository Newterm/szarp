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
/* draw3 
 * SZARP
 
 *
 * $Id$
 */

#ifndef __DRAWDND_H__
#define __DRAWDND_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/dnd.h>

#include "ids.h"

class DrawInfo;
class StatDialog;
class ConfigManager;

/**Dataobject objects holding reference to @see DrawInfo object*/
class DrawInfoDataObject : public wxDataObject {
	/** pointer to a carried DrawInfo object*/
	DrawInfo *m_draw;
	/** format of data - pointer in this case*/
	wxDataFormat m_draw_format;
	public:
	DrawInfoDataObject(DrawInfo *draw = NULL);
	/**Return prefered data format - DrawInfo in this case*/
	virtual wxDataFormat GetPreferredFormat(Direction dir) const;
	/**@return number of supported formats, this is only one */
	virtual size_t GetFormatCount(Direction dir) const;
	/**@retrn all supported formats - only one DrawInfo pointer*/
	virtual void GetAllFormats(wxDataFormat *formats, Direction dir) const;
	/**@return size of data*/
	virtual size_t GetDataSize(const wxDataFormat& format) const;
	/**Puts hold data into provided buffer*/
	virtual bool GetDataHere(const wxDataFormat& format, void *buffer) const;
	/**Sucks in provided data
	 * @param format format of data stored at buffer
	 * @param len length of data located at buffer
	 * @param buf pointer to buffer with data*/
	virtual bool SetData(const wxDataFormat& format, size_t len, const void *buf);

	/**@return carried @see DrawInfo object*/
	DrawInfo* GetDrawInfo() const;

	/**Our custom format identifier taht is poninter to a DrawInfo object*/
	static const wxChar* draw_info_format_id;
};

/**Interface for widgets that recive drops of DrawInfo objects*/
class DrawInfoDropReceiver {
	public:
	/**Method called upon drop of an DrawInfo object
	 * @param x x widget corrdinate where drop took place
	 * @param y y widget corrdinate where drop took place
	 * @param draw_info dropped draw info object
	 * @param def type of drop
	 * @return status of a drop operation shall be returned*/
	virtual wxDragResult SetDrawInfo(wxCoord x, wxCoord y, DrawInfo *draw_info, wxDragResult def) = 0;
	virtual ~DrawInfoDropReceiver() {}
};

/**Object implementing DrawInfo DND 'logic'.*/
class DrawInfoDropTarget : public wxDropTarget {
	/**Pointer to a object receiving drops*/
	DrawInfoDropReceiver *m_receiver;
	public:
	DrawInfoDropTarget(DrawInfoDropReceiver *receiver);
	/**Called by wxWidgets when drop is received
	 * @param x x widget corrdinate where drop took place
	 * @param y y widget corrdinate where drop took place
	 * @param def type of drop */
	virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);
};

/**Dataobject that hold window reference info i.e. configuration prefix, set name, period and time*/
class SetInfoDataObject : public wxDataObject {
	/**Prefix of a configuration*/
	wxString m_prefix;
	/**Set name*/
	wxString m_set;
	/**Period type*/
	PeriodType m_period;
	/**Link time*/
	time_t m_time;
	/**Graph number*/
	int m_selected_draw;

	public:
	/**@return reference to a url*/
	wxString GetUrl() const;

	SetInfoDataObject(wxString prefix, wxString window, PeriodType period, time_t time, int selected_draw);
	/**Return prefered data format we support*/
	virtual wxDataFormat GetPreferredFormat(Direction dir) const;
	/**@return number of supported formats*/
	virtual size_t GetFormatCount(Direction dir) const;
	/**@retrn all supported formats - only one DrawInfo pointer*/
	virtual void GetAllFormats(wxDataFormat *formats, Direction dir) const;
	/**@return size of data*/
	virtual size_t GetDataSize(const wxDataFormat& format) const;
	/**Puts hold data into provided buffer*/
	virtual bool GetDataHere(const wxDataFormat& format, void *buffer) const;
	/**Sucks in provided data
	 * @param format format of data stored at buffer
	 * @param len length of data located at buffer
	 * @param buf pointer to buffer with data*/
	virtual bool SetData(const wxDataFormat& format, size_t len, const void *buf);

	/**@return set name this object describes*/
	wxString GetSet() { return m_set; }
	/**@return prefix of a configuration  this object describes*/
	wxString GetPrefix() { return m_prefix; }
	/**@return period of window this object describes*/
	PeriodType GetPeriod() { return m_period; }
	/**@return time this objects desribes*/
	time_t GetTime() { return m_time; }
	/**@return time this objects desribes*/
	int GetSelectedDraw() { return m_selected_draw; }

};

/**Interface for widgets that recive drops of SetInfo objects*/
class SetInfoDropReceiver {
	public:
	/**Method called upon drop of an window reference object
	 * @param x x widget corrdinate where drop took place
	 * @param y y widget corrdinate where drop took place
	 * @param set set name
	 * @param prefix prefix of the configuration
	 * @param time time of window
	 * @param pt type of period
	 * @param def type of drop
	 * @return status of a drop operation shall be returned*/
	virtual wxDragResult SetSetInfo(wxCoord x, 
			wxCoord y, 
			wxString set, 
			wxString prefix, 
			time_t time,
			PeriodType pt, 
			wxDragResult def) = 0;
	virtual ~SetInfoDropReceiver() {}
};

/**Object implementing window reference DND 'logic'.*/
class SetInfoDropTarget : public wxDropTarget {
	/**reference to object receiving drops*/
	SetInfoDropReceiver *m_receiver;
	public:
	SetInfoDropTarget(SetInfoDropReceiver *receiver);
	/**Called by wxWidgets when drop of window refernce is received
	 * @param x x widget corrdinate where drop took place
	 * @param y y widget corrdinate where drop took place
	 * @param def type of drop */
	virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);
};

#endif
