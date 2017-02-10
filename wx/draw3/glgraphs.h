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

#ifndef __GLGRAPHS_H__
#define __GLGRAPHS_H__

#include "config.h"

#ifdef HAVE_GLCANVAS
#ifdef HAVE_FTGL

#include <wx/datetime.h>
#include <wx/colour.h>
#include <wx/dynarray.h>
#include <wx/glcanvas.h>
#include <FTGL/ftgl.h>
#include <GL/glu.h>

class GLGraphs : public wxGLCanvas, public DrawGraphs, public SetInfoDropReceiver {
	bool m_refresh;

	DrawPtrArray m_draws;

	wxSize m_size; /**<size of the widget*/

	static bool _gl_ready;

	bool m_widget_initialized;

	bool m_lists_compiled;

	GLuint m_back_pane_list;

	static GLuint _circle_list;

	static GLuint _line_list;

	static GLuint _back1_tex;

	static GLuint _line1_tex;

	struct {
		int leftmargin;
		int rightmargin;
		int topmargin;
		int bottommargin;
		int infotopmargin;
	} m_screen_margins;

	struct GraphState {
		GraphState();
		enum { STILL, FADING, EMERGING } fade_state;
		float fade_level;
	};

	std::vector<GraphState> m_graphs_states;

	wxTimer *m_timer;

	wxGLContext *m_gl_context;

	ConfigManager* m_cfg_mgr;

	static FTFont *_font;

	bool m_recalulate_margins;

	void InitGL();

	void SetUpScene();

	void DrawBackground();

	void DrawYAxisVals(Draw *draw);

	void DrawGraphs();

	float GetX(int i, const size_t& pc);

	float GetY(double value, DrawInfo *di);

	void DrawGraph(Draw *d);

	void DrawCursor(Draw *d);

	void GeneratePaneDisplayList();

	void DestroyPaneDisplayList();

	void SetLight();

	void DrawYAxis();

	void DrawYAxis(Draw *draw);

	void DrawXAxis();

	void DrawXAxisVals(Draw *d);

	void DrawShortNames();

	int GetIndex(size_t i,  int x) const;

	void GetDistance(size_t draw_index, int x, int y, double &d, wxDateTime &time);

	bool AlternateColor(Draw *d, int idx);

	void DrawSeasonLimitInfo(Draw *d, int i, int month, int day, bool summer);

	void DrawSeasonLimits();

	void DrawUnit(Draw *d);

	void RecalculateMargins();

	void DoPaint();

	void GenerateCircleDisplayList();

	void GenerateLineList();

	void DrawRemarksTriangles();

	void Init();

	void DrawCurrentParamName();

	void ResetGraphs(DrawsController* controller);

	bool m_draw_current_draw_name;
public:

        GLGraphs(wxWindow* parent, ConfigManager *cfg);

	/** Switches window */
	virtual wxDragResult SetSetInfo(wxCoord x, wxCoord y, wxString window, wxString prefix, time_t time, PeriodType pt, wxDragResult def);

	virtual void Deselected(int i);

	virtual void Selected(int i);

	virtual void Refresh() { m_refresh = true; }

	virtual void FullRefresh() { Refresh(); }

	virtual void SetFocus();

	void OnPaint(wxPaintEvent&);

	void OnTimer(wxTimerEvent&);

	virtual void StartDrawingParamName();

	virtual void StopDrawingParamName();

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

	virtual void FilterChanged(Draw *draw) { Refresh(); }

	virtual void PeriodChanged(Draw *draw, PeriodType period);

	virtual void EnableChanged(Draw *draw);

	virtual void NewRemarks(Draw *draw);

	virtual void AverageValueCalculationMethodChanged(Draw *draw);

	virtual void DrawInfoChanged(Draw *draw);

	virtual void DrawSelected(Draw *draw);

	virtual void DrawsSorted(DrawsController* controller);

	virtual ~GLGraphs();

	DECLARE_EVENT_TABLE()
};

#endif
#endif

#endif
