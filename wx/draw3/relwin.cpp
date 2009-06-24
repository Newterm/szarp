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

#include "ids.h"
#include "relwin.h"
#include "draw.h"
#include "cconv.h"
#include "timeformat.h"
#include "drawfrm.h"
#include "cfgmgr.h"

#include <algorithm>
#include <sstream>
#include <wx/statline.h>

using std::max;


BEGIN_EVENT_TABLE(RelWindow, wxDialog)
    EVT_IDLE(RelWindow::OnIdle)
    EVT_BUTTON(wxID_HELP, RelWindow::OnHelpButton)
    EVT_CLOSE(RelWindow::OnClose)
END_EVENT_TABLE()

RelWindow::ObservedDraw::ObservedDraw(Draw *_draw) : draw(_draw), 
	rel(false)
{}

RelWindow::RelWindow(wxWindow *parent, DrawPanel *panel, wxMenuItem* pie_item) :
	wxDialog(parent, wxID_ANY, _("Values ratio"), wxDefaultPosition, wxSize(200, 100), 
			wxDEFAULT_DIALOG_STYLE),
	m_rel_item(pie_item),
	m_update(false),
	m_active(false),
	m_panel(panel),
	m_proper_draws_count(false)
{
	SetHelpText(_T("draw3-ext-valuesratio"));

	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText *text = new wxStaticText(this, wxID_ANY, _("Values ratio"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
	wxStaticLine *line = new wxStaticLine(this);
	m_label = new wxStaticText(this, wxID_ANY, _T(""), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);

	sizer->Add(text, 0, wxALIGN_CENTER | wxALL, 5);
	sizer->Add(line, 0, wxEXPAND, 5);
	sizer->Add(m_label, 0, wxALIGN_CENTER | wxALL, 5);
	sizer->Add(new wxStaticLine(this), 0, wxEXPAND);
	sizer->Add(new wxButton(this, wxID_HELP), 0, wxALIGN_CENTER);
	
	SetSizer(sizer);

	m_draws.SetCount(MAX_DRAWS_COUNT, NULL);
}

void RelWindow::OnIdle(wxIdleEvent &event) {

	if (!m_active)
		return;

	if (!m_update)
		return;

	if (m_proper_draws_count < 2) {
		m_label->SetLabel(_("There are no values ratio defined for these graphs"));
		m_update = false;
		Layout();
		Resize();
		return;
	}
	
	wxString unit1;
	wxString unit2;

	double first_value = nan("");
	double second_value = nan("");

	int prec = 0;

	bool got_first = false;

	for (size_t i = 0; i < m_draws.Count(); ++i) {
		if (m_draws[i] == NULL)
			break;

		ObservedDraw *od = m_draws[i];
		if (od->rel == false)
			continue;

		const Draw::VT & vt = od->draw->GetValuesTable();
		if (vt.m_count == 0)
			break;

		if (got_first) {
			unit2 = od->draw->GetDrawInfo()->GetUnit();
			second_value = vt.m_sum;
			break;
		}

		first_value = vt.m_sum;
		unit1 = od->draw->GetDrawInfo()->GetUnit();
		prec = od->draw->GetDrawInfo()->GetPrec();

		got_first = true;

	}

	if (!std::isnan(first_value)
			&& !std::isnan(second_value)
			&& second_value != 0)  {
		std::wstringstream wss;
		wss.precision(2);
		wss << (first_value / second_value);
		m_label->SetLabel(wxString::Format(_T("%s %s/%s"), wss.str().c_str(), first_value / second_value, unit1.c_str(), unit2.c_str()));
	} else
		m_label->SetLabel(_T(""));

	GetSizer()->Fit(this);
		
	m_update = false;
}

void RelWindow::Resize() {
	int w, h;
	GetSize(&w, &h);
	wxSize s = GetSizer()->GetMinSize();
	int wn = max(w, s.GetWidth());
	int hn = max(h, s.GetHeight());
	SetSize(wn, hn);
	Layout();
}

void RelWindow::Activate() {
	if (m_active)
		return;
	m_active = true;

       	for (size_t i = 0; i < m_draws.Count(); ++i) {
		ObservedDraw* od = m_draws[i];
		if (od) {
			od->draw->AttachObserver(this);
			od->rel = od->draw->GetDrawInfo()->GetSpecial() == TDraw::REL;
			if (od->rel)
				m_proper_draws_count++;
		}
	}

	m_update = true;

}

void RelWindow::Deactivate() {
	if (m_active == false) 
		return;

	m_active = false;

       	for (size_t i = 0; i < m_draws.Count(); ++i) {
		ObservedDraw* od = m_draws[i];
		if (od != NULL) {
			od->draw->DetachObserver(this);
			if (od->rel)
				m_proper_draws_count--;
		}
	}

}

void RelWindow::Detach(Draw *draw) {
	assert(draw);

	if (m_active) 
		draw->DetachObserver(this);

	int no = draw->GetDrawNo();

	if (m_draws[no] == NULL)
		return;

	if (m_draws[no]->rel)
		m_proper_draws_count--;

	delete m_draws[no];
	m_draws[no] = NULL;

	if (m_proper_draws_count < 2)
		m_update = true;
}

void RelWindow::Attach(Draw *draw) {

	if (m_active) 
		draw->AttachObserver(this);

	int no = draw->GetDrawNo();

	assert(m_draws[no] == NULL);

	m_draws[no] = new ObservedDraw(draw);
	m_draws[no]->rel = draw->GetDrawInfo()->GetSpecial() == TDraw::REL; 

	if (m_draws[no]->rel) {
		m_update = true;
		m_proper_draws_count++;
	}

}

void RelWindow::DrawInfoChanged(Draw *draw) {
	Detach(draw);
	Attach(draw);
}

void RelWindow::Update(Draw *draw) {
	int no = draw->GetDrawNo();

	ObservedDraw *od = m_draws[no];

	if (od == NULL)
		return;

	if (od->rel == false)
		return;

	m_update = true;
}

void RelWindow::StatsChanged(Draw *draw) {
	Update(draw);
}

void RelWindow::OnClose(wxCloseEvent &event) {
	if (event.CanVeto()) {
		event.Veto();
		m_panel->ShowRelWindow(false);
		m_rel_item->Check(false);
	} else
		Destroy();
}

RelWindow::~RelWindow() { 
	for (size_t i = 0; i < m_draws.size(); ++i)
		delete m_draws[i];
}

bool RelWindow::Show(bool show) {
	if (show == IsShown())
		return false;

	if (show) 
		Activate();
	else
		Deactivate();

	bool ret = wxDialog::Show(show);

	return ret;
}

void RelWindow::OnHelpButton(wxCommandEvent &event) {
	wxHelpProvider::Get()->ShowHelp(this);
}

