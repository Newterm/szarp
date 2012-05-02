#include <wx/config.h>
#include <wx/filename.h>
#include <libxml/tree.h>
#include <wx/xrc/xmlres.h>

#include "szframe.h"
#include "szhlpctrl.h"
#include "xmlutils.h"

#include "ids.h"
#include "classes.h"
#include "sprecivedevnt.h"
#include "parameditctrl.h"
#include "seteditctrl.h"
#include "drawapp.h"
#include "drawobs.h"
#include "drawtime.h"
#include "coobs.h"
#include "cfgmgr.h"
#include "cconv.h"
#include "version.h"
#include "drawfrm.h"
#include "drawpnl.h"
#include "dbinquirer.h"
#include "database.h"
#include "draw.h"
#include "errfrm.h"
#include "md5.h"
#include "drawsctrl.h"
#include "remarks.h"
#include "defcfg.h"

#include "remarks.h"
#include "remarksfetcher.h"
#include "remarksdialog.h"



RemarksListDialog::RemarksListDialog(DrawFrame* parent, RemarksHandler *remarks_handler) {
	SetHelpText(_T("draw3-ext-remarks-browsing"));

	m_remarks_handler = remarks_handler;

	m_draw_frame = parent;

	bool loaded = wxXmlResource::Get()->LoadDialog(this, parent, _T("RemarksListDialog"));
	assert(loaded);

	m_list_ctrl = XRCCTRL(*this, "RemarksListCtrl", wxListCtrl);
	assert(m_list_ctrl);

	m_list_ctrl->InsertColumn(0, _("Date"), wxLIST_FORMAT_LEFT, -1);
	m_list_ctrl->InsertColumn(1, _("Title"), wxLIST_FORMAT_LEFT, -1);

}

void RemarksListDialog::SetViewedRemarks(std::vector<Remark> &remarks) {
	m_displayed_remarks = remarks;

	m_list_ctrl->Freeze();
	m_list_ctrl->DeleteAllItems();

	for (size_t i = 0; i < remarks.size(); i++) {
		Remark& r = remarks[i];

		m_list_ctrl->InsertItem(i, wxDateTime(r.GetTime()).Format(_T("%Y-%m-%d %H:%M")));
		m_list_ctrl->SetItem(i, 1, r.GetTitle());

	}

	int col_width = -1; //auto width, equal to maximum width of element in the column

	if (remarks.size() == 0)
		col_width = -2;		//auto width, eqal to width of column title

	m_list_ctrl->SetColumnWidth(0, col_width);
	m_list_ctrl->SetColumnWidth(1, col_width);

	m_list_ctrl->Thaw();

	m_list_ctrl->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

	m_list_ctrl->SetFocus();
}

void RemarksListDialog::OnCloseButton(wxCommandEvent &event) {
	EndModal(wxID_CLOSE);
}

void RemarksListDialog::ShowRemark(int index) {
	Remark& r = m_displayed_remarks.at(index);

	RemarkViewDialog* d = new RemarkViewDialog(this, m_remarks_handler);
	int ret = d->ShowRemark(r.GetBaseName(),
			r.GetSet(),
			r.GetAuthor(),
			r.GetTime(),
			r.GetTitle(),
			r.GetContent());
	d->Destroy();

	if (ret == wxID_FIND) {
		m_draw_frame->GetCurrentPanel()->Switch(r.GetSet(), r.GetAttachedPrefix(), r.GetTime());
		EndModal(wxID_CLOSE);
	}
}


void RemarksListDialog::OnOpenButton(wxCommandEvent &event) {

	long i = -1;
	
	i = m_list_ctrl->GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (i == -1)
		return;

	ShowRemark(i);
}

void RemarksListDialog::OnRemarkItemActivated(wxListEvent &event) {
	ShowRemark(event.GetIndex());
}

void RemarksListDialog::OnClose(wxCloseEvent& event) {
	EndModal(wxID_CLOSE);
}

void RemarksListDialog::OnHelpButton(wxCommandEvent &event) {
	wxHelpProvider::Get()->ShowHelp(this);
}

BEGIN_EVENT_TABLE(RemarksListDialog, wxDialog)
	LOG_EVT_BUTTON(wxID_OPEN, RemarksListDialog , OnOpenButton, "remarks:open" )
	LOG_EVT_BUTTON(wxID_CLOSE, RemarksListDialog , OnCloseButton, "remarks:close" )
	LOG_EVT_BUTTON(wxID_HELP, RemarksListDialog , OnHelpButton, "remarks:help" )
	LOG_EVT_CLOSE(RemarksListDialog , OnClose , "remarks:close")
	EVT_LIST_ITEM_ACTIVATED(XRCID("RemarksListCtrl"), RemarksListDialog::OnRemarkItemActivated)
END_EVENT_TABLE()
