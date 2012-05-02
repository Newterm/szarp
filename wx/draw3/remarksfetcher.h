#ifndef __REMARKSFETCHER_H__
#define __REMARKSFETCHER_H__
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
/* Remarks handling code 
 *
 * $Id$
 */

#include <map>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class RemarksFetcher : public wxEvtHandler, public DrawObserver, public RemarksReceiver  {
	std::map<Remark::ID, Remark> m_remarks;

	DrawFrame *m_draw_frame;

	RemarksHandler *m_remarks_handler;

	DrawsController *m_draws_controller;

	bool m_awaiting_for_whole_base;

	void Fetch();

public:
	RemarksFetcher(RemarksHandler *remarks_handler, DrawFrame* m_draw_frame);

	virtual void ScreenMoved(Draw* draw, const wxDateTime &start_time);

	virtual void DrawInfoChanged(Draw *draw);

	virtual void Attach(DrawsController *d);

	virtual void Detach(DrawsController *d);

	virtual void DrawSelected(Draw *draw);

	void ShowRemarks();

	void OnShowRemarks(wxCommandEvent &e);

	void ShowRemarks(const wxDateTime &from_time, const wxDateTime &to_time);

	virtual void RemarksReceived(std::vector<Remark>& remarks);

	void OnRemarksResponse(RemarksResponseEvent &e);

	virtual ~RemarksFetcher();

	DECLARE_EVENT_TABLE()
};

#endif /* __REMARKSFETCHER_H__ */
