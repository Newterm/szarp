#include "ids.h"
#include "classes.h"
#include "drawobs.h"
#include "drawpnl.h"
#include "dbtypeswin.h"

BEGIN_EVENT_TABLE(BaseLabel, wxWindow)
	EVT_PAINT(BaseLabel::OnPaint)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(DatabaseTypesWindow, wxDialog)
	EVT_CLOSE(DatabaseTypesWindow::OnClose)
END_EVENT_TABLE()

BaseLabel::BaseLabel(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size) :
	wxWindow(parent, id, pos, size, wxFULL_REPAINT_ON_RESIZE)
{}

wxSize BaseLabel::DoGetBestSize() const {
	if (base_name.IsEmpty())
		return wxSize(0, 0);

	wxClientDC dc(const_cast<BaseLabel*>(this));
	dc.SetFont(GetFont());
	int w1, h1;
	dc.GetTextExtent(base_name, &w1, &h1);

	int w2, h2;
	dc.GetTextExtent(base_type, &w2, &h2);

	return wxSize(w1 + w2 + 6, std::max(h1, h2) + 4);
}

void BaseLabel::SetBaseNameText(const wxString &base_name) {
	this->base_name = base_name;
	Refresh();
}

void BaseLabel::SetBaseTypeText(const wxString &base_type) {
	this->base_type = base_type;
	Refresh();
}

void BaseLabel::OnPaint(wxPaintEvent &event) {
	wxClientDC dc(this);
	wxColour bg = GetBackgroundColour();

	dc.SetBackground(bg);
	dc.Clear();

	wxFont font(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
					wxFONTWEIGHT_NORMAL, false);
	dc.SetFont(font);

	int w, h, tw, th;

	GetClientSize(&w, &h);

	dc.GetTextExtent(base_type, &tw, &th);

	dc.DrawText(base_type, w - tw - margin_right, h / 2 - th / 2);

	dc.GetTextExtent(base_name, &tw, &th);

	dc.DrawText(base_name, margin_left, h / 2 - th / 2);
}

DatabaseTypesWindow::DatabaseTypesWindow(DrawPanel *draw_panel, wxWindow *parent) : 
	wxDialog(NULL, drawID_DBTYPES, _("Databases types"), wxDefaultPosition, wxSize(250, 200), 
			wxDEFAULT_DIALOG_STYLE | wxDIALOG_NO_PARENT | wxSTAY_ON_TOP)
{
	tabsCount = 0;
	overallWidth = 450;
	overallHeight = 10;

	this->draw_panel = draw_panel;

	sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);
}

bool DatabaseTypesWindow::Show(bool show) {
	if (show == IsShown())
		return false;

	bool ret = wxDialog::Show(show);

	return ret;
}

void DatabaseTypesWindow::MakeLabelsFromMap(std::map<wxString, wxString> *tabMap) {

	if (tabsCount != tabMap->size()) {
		sizer->Clear();
		overallWidth = 450;
		overallHeight = 10;
		for (int j = 0; j < tabsCount; ++j) {
			delete base_labels[j];
		}

		tabsCount = tabMap->size();
		base_labels.resize(tabsCount, NULL);

		{
			int labelWidth, labelHeight;
			int i = 0;
			for (auto it = tabMap->begin(); it != tabMap->end(); ++it, ++i){
				base_labels[i] = new BaseLabel(this, wxID_ANY);
				base_labels[i]->Show(true);
				base_labels[i]->SetForegroundColour(*wxBLACK);
				if (i % 2) {
					base_labels[i]->SetBackgroundColour(*wxLIGHT_GREY);
				} else
					base_labels[i]->SetBackgroundColour(*wxWHITE);

				base_labels[i]->SetBaseNameText(it->first);
				base_labels[i]->SetBaseTypeText(it->second);

				base_labels[i]->GetClientSize(&labelWidth, &labelHeight);
				overallWidth = std::max(overallWidth, labelWidth);
				overallHeight += labelHeight;
				sizer->Add(base_labels[i], 1, wxEXPAND);
			}
		}

		sizer->SetMinSize(overallWidth,overallHeight);
		sizer->Fit(this);
	}
}

void DatabaseTypesWindow::OnClose(wxCloseEvent &event) {
	if (event.CanVeto()) {
		event.Veto();
		draw_panel->ShowBaseTypesWindow(false);
	} else
		Destroy();
}
