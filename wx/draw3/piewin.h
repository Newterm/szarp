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
/* * draw3 
 * * SZARP
 
 *
 * $Id$
 */

#ifndef __PIEWIN_H__
#define __PIEWIN_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/statline.h>

#include "drawobs.h"
class Draw;
class DrawPanel;

/**Display summary values of currently displayed draws*/
class PieWindow : public wxFrame, public DrawObserver {

	void PaintGraphControl(wxDC &dc);

	class GraphControl : public wxControl {
		PieWindow *m_pie_window;
		wxSize m_best_size;
	protected:
		virtual wxSize DoGetBestSize() const { return m_best_size; }
	public:
		void SetBestSize(int w, int h);
		GraphControl(PieWindow *pie_window);
		void OnPaint(wxPaintEvent &event);
		DECLARE_EVENT_TABLE();
	};

	GraphControl *m_graph;

	wxMenuItem *m_pie_item;

	/**This structure holds values of displayed draws*/
	struct ObservedDraw {
		ObservedDraw(Draw *_draw);
		/**@see Draw which values are displayed*/
		Draw *draw;

		bool piedraw;
	};

	WX_DEFINE_ARRAY(ObservedDraw*, ObservedDrawPtrArray);

	/**Indicates if we shall update window*/
	bool m_update;

	/**Indicates if window is 'active' - is shown and attached to draws*/
	bool m_active;

	/**Parent @see DrawPanel*/
	DrawPanel *m_panel;

	/**Size that we want window to be resized in idle event*/
	wxSize m_requested_size;

	/**Draw values array*/
	ObservedDrawPtrArray m_draws;

	/**Activates widget - attaches to @see Draw objects as an observer*/
	void Activate();

	/**Deactivate widgets - detaches from @see Draw objects*/
	void Deactivate();

	/**Hides window*/
	void OnClose(wxCloseEvent &event);

	/**Draws window*/
	void OnPaint(wxPaintEvent &event);

	/**Cauces window repaint*/
	void UpdateDraw(Draw *d);

	/**Resizes window (if neccesary)*/
	void OnIdle(wxIdleEvent &event);

	void OnHelpButton(wxCommandEvent &event);

	void PaintGraphControl(wxPaintDC &dc);

	/**Number of draws in current set that has 'piewin' attribute set*/
	int m_proper_draws_count;

	public:
	PieWindow(wxWindow *parent, DrawPanel *panel, wxMenuItem *pie_item);

	/**Displays window and activates object*/
	virtual bool Show(bool show = true);

	/**Removes @see Draw from observed draw's list*/
	virtual void Detach(Draw* draw);

	/**Adds @see Draw to observer draws' list*/
	virtual void Attach(Draw* draw);

	/**Causes recalulation of summary values of notifing draw (in OnIdle handler)*/
	virtual void DrawInfoChanged(Draw *draw);

	/**Causes recalulation of summary values of notifing draw (in OnIdle handler)*/
	virtual void PeriodChanged(Draw *draw, PeriodType period);

	/**Causes window repaint*/
	virtual void EnableChanged(Draw *draw);

	/**Causes window repaint*/
	virtual void StatsChanged(Draw *draw);

	virtual ~PieWindow();

	DECLARE_EVENT_TABLE()
};

#endif
