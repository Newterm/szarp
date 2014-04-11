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
 * $Id$
 */


// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifndef __WXGRAPHS_H__
#define __WXGRAPHS_H__

#include "config.h"

#include <wx/datetime.h>
#include <wx/colour.h>
#include <wx/dynarray.h>
#include <wx/arrimpl.cpp>
#include <wx/dc.h>

#ifndef NO_GSTREAMER
#include <gst/gst.h>
#endif

/** MUST BE ODD!! */
#define CURSOR_RECTANGLE_SIZE 9

class WxGraphs : public wxWindow, public DrawGraphs, public SetInfoDropReceiver {
	bool m_right_down;

	/** Internal array of @see GraphVIew objects . */
	std::vector<GraphView*> m_graphs;

	/**View attached to currently selected to @see Draw, draws the background*/
	BackgroundView *m_bg_view;

	DrawPtrArray m_draws;

#ifndef NO_GSTREAMER
	/**Vector holding spectrum values of currently played songs.*/
	std::vector<float> m_spectrum_vals;
#endif

#ifndef NO_GSTREAMER
	/**If set true the slected draw is dancing*/
	bool m_dancing;

	/**gstreamer object responsible for playing music*/
	GstElement *m_gstreamer_bin;

	/**Starts graph dance*/
	void StartDance();

	/**Stops graph dance*/
	void StopDance();

	/**Draws one frame*/
	void DrawDance(wxDC *dc);

#endif
	/**Region that shall be repainted upon reception of idle event*/
	wxRegion m_invalid_region;

	wxSize m_size; /**<size of the widget*/

	bool m_draw_current_draw_name;

	ConfigManager *m_cfg_mgr;
       
	struct {
		int leftmargin;
		int rightmargin;
		int topmargin;
		int bottommargin;
		int infotopmargin;
	} m_screen_margins;

	/** Draws window name and param's short names. But only if that part of widget was damaged
	 * @param dc dc to draw this information onto
	 * @param damaged region*/
	void DrawWindowInfo(wxDC *dc, const wxRegion& region);

	void SetMargins();

	void DrawCurrentParamName(wxDC *dc);

	void ResetDraws(DrawsController *controller);
public:
        WxGraphs(wxWindow* parent, ConfigManager *cfg);

	/**Causes given region to be repainted upon reception of idle event
	 * @param region region to be repainted*/
	void UpdateArea(const wxRegion& region);

	/** Switches window */
	virtual wxDragResult SetSetInfo(wxCoord x, wxCoord y, wxString window, wxString prefix, time_t time, PeriodType pt, wxDragResult def);

	virtual void Refresh();

	virtual void StartDrawingParamName();

	virtual void StopDrawingParamName();

	virtual void FullRefresh();

	virtual void DrawInfoChanged(Draw *draw);

	virtual void SetFocus();

	virtual void DrawDeselected(Draw *d);

	virtual void DrawSelected(Draw *d);

	virtual	void FilterChanged(DrawsController *draws_controller);

	virtual void EnableChanged(Draw *draw);

	virtual void PeriodChanged(Draw *draw, PeriodType period);

	virtual void ScreenMoved(Draw* draw, const wxDateTime &start_date);

	virtual void NumberOfValuesChanged(DrawsController *draws_controller);

	virtual void NewData(Draw* draw, int i);

	virtual void NewRemarks(Draw *draw);

	virtual void AverageValueCalculationMethodChanged(Draw *draw);

	virtual void DoubleCursorChanged(DrawsController *draws_controller);

	virtual void CurrentProbeChanged(Draw* draw, int pi, int ni, int d);

	virtual void DrawsSorted(DrawsController* controller);

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

	void OnMouseRightDown(wxMouseEvent& event);

	void OnMouseRightUp(wxMouseEvent& event);

	void OnMouseMove(wxMouseEvent& event);

	void OnMouseLeave(wxMouseEvent& event);

	void OnSize(wxSizeEvent& event);

	void OnSetFocus(wxFocusEvent& event);

	void OnEraseBackground(wxEraseEvent& event);

	void OnIdle(wxIdleEvent& event);

	virtual ~WxGraphs();

	DECLARE_EVENT_TABLE()
};


#endif 
