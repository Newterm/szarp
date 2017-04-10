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
#include "dbinquirer.h"
#include "drawfrm.h"
#include "drawpnl.h"
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

RemarksFetcher::RemarksFetcher(RemarksHandler *handler, DrawFrame *draw_frame) {
	m_remarks_handler = handler;
	m_draw_frame = draw_frame;
	m_awaiting_for_whole_base = false;

	handler->AddRemarkReceiver(this);
}

void RemarksFetcher::Fetch() {
	if (!m_remarks_handler->Configured())
		return;

	Draw* d = m_draws_controller->GetSelectedDraw();
	if (d == NULL)
		return;

	wxDateTime start = d->GetTimeOfIndex(0);
	if (!start.IsValid())
		return;

	wxDateTime end = d->GetTimeOfIndex(d->GetValuesTable().size());
	DrawInfo *di = d->GetDrawInfo();

	m_remarks_handler->GetStorage()->GetRemarks(
				di->GetBasePrefix(),
				di->GetSetName(),
				start.GetTicks(),
				end.GetTicks(),
				this);

}


void RemarksFetcher::Attach(DrawsController *d) {

	m_draws_controller = d;

	d->AttachObserver(this);

	m_remarks.clear();

	Fetch();
	
}

void RemarksFetcher::Detach(DrawsController *d) {
	m_remarks.clear();

	d->DetachObserver(this);

}

void RemarksFetcher::RemarksReceived(std::vector<Remark>& remarks) {

	Draw* d = m_draws_controller->GetSelectedDraw();
	if (d == NULL)
		return;

	DrawInfo* di = d->GetDrawInfo();

	wxDateTime start = d->GetTimeOfIndex(0);
	wxDateTime end = d->GetTimeOfIndex(d->GetValuesTable().size());

	wxString prefix = di->GetBasePrefix();
	wxString set = di->GetSetName();
	std::vector<wxDateTime> remarks_times;

	for (std::vector<Remark>::iterator i = remarks.begin(); i != remarks.end(); i++) {
		wxDateTime rt = i->GetTime();

		std::wstring rset = i->GetSet();

		if (prefix != i->GetAttachedPrefix()
				|| (!rset.empty() && set != rset)
				|| rt < start
				|| rt >= end)
			continue;

		remarks_times.push_back(rt);
		m_remarks[i->GetId()] = *i;

	}

	if (remarks_times.size()) {
		d->SetRemarksTimes(remarks_times);
	}
}

void RemarksFetcher::OnRemarksResponse(RemarksResponseEvent &e) {
	std::vector<Remark>& remarks = e.GetRemarks();

	Draw* d = m_draws_controller->GetSelectedDraw();
	if (d == NULL)
		return;

	ConfigNameHash& cnh = d->GetDrawInfo()->GetDrawsSets()->GetParentManager()->GetConfigTitles();
	for (std::vector<Remark>::iterator i = remarks.begin();
			i != remarks.end();
			i++) {
		ConfigNameHash::iterator j = cnh.find(i->GetPrefix());

		if (j != cnh.end())
			i->SetBaseName(j->second.wc_str());
	}

	if (e.GetResponseType() == RemarksResponseEvent::BASE_RESPONSE) {
		if (!m_awaiting_for_whole_base)
			return;
		m_awaiting_for_whole_base = false;


		RemarksListDialog *d = new RemarksListDialog(m_draw_frame, m_remarks_handler);
		d->SetViewedRemarks(remarks);
		d->ShowModal();
		d->Destroy();
	} else 
		RemarksReceived(remarks);
}

void RemarksFetcher::DrawSelected(Draw *draw) {
	m_remarks.clear();
	Fetch();
}

void RemarksFetcher::ScreenMoved(Draw* draw, const wxDateTime &start_time) {
	if (!m_remarks_handler->Configured())
		return;

	wxDateTime end_time = draw->GetTimeOfIndex(draw->GetValuesTable().size());

	std::vector<Remark::ID> to_remove;
	for (std::map<Remark::ID, Remark>::iterator i = m_remarks.begin();
			i != m_remarks.end();
			i++) {
		wxDateTime rt = i->second.GetTime();
		if (rt < start_time || rt >= end_time)
			to_remove.push_back(i->first);

	}

	for (std::vector<Remark::ID>::iterator i = to_remove.begin();
			i != to_remove.end();
			i++)
		m_remarks.erase(*i);

	Fetch();
}

void RemarksFetcher::ShowRemarks(const wxDateTime& from_time, const wxDateTime &to_time) {
	std::vector<Remark> remarks;

	for (std::map<Remark::ID, Remark>::iterator i = m_remarks.begin();
			i != m_remarks.end();
			i++) {
		wxDateTime dt = i->second.GetTime();
		if (dt >= from_time && dt < to_time)
			remarks.push_back(i->second);
	}

	RemarksListDialog *d = new RemarksListDialog(m_draw_frame, m_remarks_handler);
	d->SetViewedRemarks(remarks);
	d->ShowModal();
	d->Destroy();

}

void RemarksFetcher::ShowRemarks() {
	if (!m_remarks_handler->Configured())
		return;
	m_awaiting_for_whole_base = true;
	Draw* d = m_draws_controller->GetSelectedDraw();
	if (d == NULL)
		return;
	m_remarks_handler->GetStorage()->GetAllRemarks(d->GetDrawInfo()->GetBasePrefix(), this);
}

void RemarksFetcher::DrawInfoChanged(Draw *d) {
	if (d->GetSelected()) {
		m_remarks.clear();
		Fetch();
	}
}

RemarksFetcher::~RemarksFetcher() {
	m_remarks_handler->RemoveRemarkReceiver(this);
}

BEGIN_EVENT_TABLE(RemarksFetcher, wxEvtHandler)
	EVT_REMARKS_RESPONSE(wxID_ANY, RemarksFetcher::OnRemarksResponse)
END_EVENT_TABLE()
