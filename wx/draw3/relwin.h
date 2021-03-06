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

#ifndef __RELWIN_H__
#define __RELWIN_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/statline.h>

#include <map>

/**Display summary values of currently displayed draws*/
class RelWindow : public wxDialog, public DrawObserver {

	/**Default border width*/
	static const int default_border_width;
	/**Default dialog width*/
	static const int default_dialog_width;
	/**Default dialog height*/
	static const int default_dialog_height;

	/**This structure holds values of displayed draws*/
	struct ObservedDraw {
		ObservedDraw(Draw *_draw);
		/**@see Draw which values are displayed*/
		Draw *draw;

		bool rel;
	};

	/**Indicates if we shall update value label*/
	bool m_update;

	/**Indicates if window is 'active' - is shown and attached to draws*/
	bool m_active;

	/**parent @see DrawPanel*/
	DrawPanel *m_panel;

	std::map<size_t, ObservedDraw*> m_draws;

	/**Widget displaying value*/
	wxStaticText *m_label;

	/**Number of draws with rel attribute set*/
	int m_proper_draws_count;

	DrawsController *m_draws_controller;

	/**Recalculates summary values if necessary and/or updates labales values*/
	void OnIdle(wxIdleEvent& event);

	/**Activates widget - attaches to @see Draw objects as an observer*/
	void Activate();

	/**Deactivate widgets - detaches from @see Draw objects*/
	void Deactivate();

	/**Hides window*/
	void OnClose(wxCloseEvent &event);

	/**Updates displayed value*/
	void Update(Draw *draw);

	void Resize();

	void OnHelpButton(wxCommandEvent &event);

	void SetDraw(Draw *draw);

	void ResetDraw(Draw *draw);
	public:
	RelWindow(wxWindow *parent, DrawPanel *panel);

	/**Displays window and activates object*/
	virtual bool Show(bool show = true);

	/**Removes @see Draw from observed draw's list*/
	virtual void Detach(DrawsController* draws_controllerk);

	/**Adds @see Draw to observer draws' list*/
	virtual void Attach(DrawsController* draws_controller);

	/**Causes recalulation of summary values of notifing draw (in OnIdle handler)*/
	virtual void DrawInfoChanged(Draw *draw);

	/**Causes window repaint*/
	virtual void StatsChanged(Draw *draw);

	virtual void AverageValueCalculationMethodChanged(Draw *draw);

	virtual void DoubleCursorChanged(DrawsController *draw);

	virtual ~RelWindow();

	DECLARE_EVENT_TABLE()
};

#endif
