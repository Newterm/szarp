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
/* * draw3 * SZARP
 
 *
 * $Id$
 */

#ifndef __SUMMWIN_H__
#define __SUMMWIN_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/statline.h>

/**Label displaying value of param*/
class TTLabel : public wxWindow {
	/**Param value*/
	wxString value;
	/**Param unit*/
	wxString unit;
protected:
	/**@return best size of widget*/
	virtual wxSize DoGetBestSize() const;
public:
	TTLabel(wxWindow *parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
	/**Set @see value*/
	void SetValueText(const wxString& value);
	/**Sets @see unit*/
	void SetUnitText(const wxString& unit); 
	/**Draws the widget content*/
	void OnPaint(wxPaintEvent &event);
	DECLARE_EVENT_TABLE()
};


/**Display summary values of currently displayed draws*/
class SummaryWindow : public wxDialog, public DrawObserver {
	/**This structure holds values of displayed draws*/
	struct ObservedDraw {
		ObservedDraw(Draw *_draw);
		/**@see Draw which values are displayed*/
		Draw *draw;
		/**Indicates if label shall be updated in IDLE event handler*/
		bool update;
		/**Indicates if tooltip shall be updated in IDLE event handler*/
		bool tooltip;
		/**Indicates if label is attached to a window*/
		bool attached;
	};

	/**Number of draws that has pie attribute set*/
	int m_summary_draws_count;

	WX_DEFINE_ARRAY(ObservedDraw*, ObservedDrawPtrArray);

	WX_DEFINE_ARRAY(TTLabel*, TLabelPtrArray);
	WX_DEFINE_ARRAY(wxStaticLine*, LinePtrArray);

	/**Indicates if label value of any draw shall be updated*/
	bool m_update;

	/**Indicates if window is 'active' - is shown and attached to draws*/
	bool m_active;

	/**Indicates if tooltip value of any draw shall be updated*/
	bool m_tooltip;

	/**Draw values array*/
	ObservedDrawPtrArray m_draws;

	/**Labels displaying values*/
	TLabelPtrArray m_labels;

	TTLabel* m_no_draws_label;

	wxStaticLine* m_no_draws_line;

	/**Line separating values*/
	LinePtrArray m_lines;

	/**Parent panel*/
	DrawPanel *draw_panel;

	DrawsController *draws_controller;

	wxBoxSizer* m_values_sizer;

	/**Adjusts window size to the size of a main sizer*/
	void Resize();

	/**Starts observing and dispalying value of draw of given number*/
	void StartDisplaying(int no);

	/**Stops observing and dispalying value of draw of given number*/
	void StopDisplaying(int no);

	/**Recalculates summary values if necessary and/or updates labales values*/
	void OnIdle(wxIdleEvent& event);

	/**Activates widget - attaches to @see Draw objects as an observer*/
	void Activate();

	/**Deactivate widgets - detaches from @see Draw objects*/
	void Deactivate();

	/**Hides window*/
	void OnClose(wxCloseEvent &event);

	void OnHelpButton(wxCommandEvent &event);

	void SetDraw(Draw *draw);

	public:
	SummaryWindow(DrawPanel* panel, wxWindow *parent);

	/**Displays window and activates object*/
	virtual bool Show(bool show = true);

	virtual void Detach(DrawsController* draws_controller);

	virtual void Attach(DrawsController* draws_controller);

	/**Causes recalulation of summary values of notifing draw (in OnIdle handler)*/
	virtual void DrawInfoChanged(Draw *draw);

	/**Causes refresh of summary values of notifing draw (in OnIdle handler)*/
	virtual void PeriodChanged(Draw *draw, PeriodType period);

	/**Causes refresh of summary values of notifing draw (in OnIdle handler)*/
	virtual void EnableChanged(Draw *draw);

	/**Causes refresh of summary values of notifing draw (in OnIdle handler)*/
	virtual void StatsChanged(Draw *draw);

	virtual void DoubleCursorChanged(DrawsController *draws_controller);

	virtual void NumberOfValuesChanged(DrawsController *draws_controller);

	/**Causes refresh of given param value*/
	void UpdateDraw(Draw *draw);

	virtual ~SummaryWindow();

	DECLARE_EVENT_TABLE()
};

#endif
