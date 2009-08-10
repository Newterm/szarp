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
 * draw3 
 * SZARP

 *
 * $Id: wxgraphs.h 1 2009-06-24 15:09:25Z isl $
 */

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifndef __GCDCGRAPHS_H__
#define __GCDCGRAPHS_H__

#include "config.h"

#include <wx/datetime.h>
#include <wx/colour.h>
#include <wx/graphics.h>
#include <wx/dynarray.h>

#include "drawswdg.h"
#include "drawobs.h"

class GCDCGraphs: public wxWindow, public DrawGraphs, public SetInfoDropReceiver, public DrawObserver {

	DrawPtrArray m_draws;

	ConfigManager *m_cfg_mgr;

	bool m_draw_param_name;

	bool m_refresh;

	bool m_recalulate_margins;

	struct {
		int leftmargin;
		int rightmargin;
		int topmargin;
		int bottommargin;
		int infotopmargin;
	} m_screen_margins;

	void DrawWindowInfo(wxGraphicsContext &dc, const wxRegion& region);

	void DrawBackground(wxGraphicsContext &dc);

	void DrawYAxisVals(Draw *draw);

	void DrawYAxis(wxGraphicsContext &dc);

	void DrawYAxisVals(wxGraphicsContext &dc, Draw *draw);

	void DrawXAxis(wxGraphicsContext &dc);

	void DrawXAxisVals(wxGraphicsContext &dc, Draw *d);

	void RecalculateMargins(wxGraphicsContext &dc);

	void DrawGraphs(wxGraphicsContext &dc);

	static const wxColour back1_col;

	static const wxColour back2_col;

	int GetX(int i);

	int GetY(double val, DrawInfo *di);

	void DrawGraph(wxGraphicsContext &dc, Draw* d);

	void DrawCursor(wxGraphicsContext &dc, Draw* d);

public:
        GCDCGraphs(wxWindow* parent, ConfigManager *cfg);

	virtual wxDragResult SetSetInfo(wxCoord x, wxCoord y, wxString window, wxString prefix, time_t time, PeriodType pt, wxDragResult def);

	virtual void SetDrawsChanged(DrawPtrArray draws);

	virtual void StartDrawingParamName();

	virtual void StopDrawingParamName();

	virtual void Selected(int i);

	virtual void Deselected(int i);

	virtual void Refresh();

	virtual void FullRefresh();
	
	virtual void SetFocus();

	void OnPaint(wxPaintEvent&);

	/** Event handler - called when left mouse button is pressed.
	 * Handles moving cursor and shifting screen.
	 */
	void OnMouseLeftDown(wxMouseEvent& event);

	/** Event handler - called on left mouse button double-click.
	 * Handles shifting screen.
	 */
	void OnMouseLeftDClick(wxMouseEvent& event);

	/** Event handler - called when left mouse button is released.
	 * Stops cursor movement..
	 */
	void OnMouseLeftUp(wxMouseEvent& event);

	/** Event handler - called when left mouse button is pressed.
	 * Handles moving cursor and shifting screen.
	 */
	void OnMouseRightDown(wxMouseEvent& event);

	void OnSize(wxSizeEvent& event);

	void OnSetFocus(wxFocusEvent& event);

	void OnEraseBackground(wxEraseEvent& event);

	void OnIdle(wxIdleEvent& event);

	virtual void ScreenMoved(Draw* draw, const wxDateTime &start_date) { Refresh(); }

	virtual void NewData(Draw* draw, int idx) { Refresh(); }

	virtual void StatsChanged(Draw *draw) { Refresh(); };

	virtual void CurrentProbeChanged(Draw *draw, int pi, int ni, int d) { Refresh(); }

	virtual void DrawInfoChanged(Draw *draw) { Refresh(); }

	virtual void FilterChanged(Draw *draw) { Refresh(); }

	virtual void PeriodChanged(Draw *draw, PeriodType period) { Refresh(); }

	virtual void EnableChanged(Draw *draw) { Refresh(); }

//	virtual void NewRemarks(Draw *draw);


	virtual ~GCDCGraphs();

	DECLARE_EVENT_TABLE()
};

#endif
