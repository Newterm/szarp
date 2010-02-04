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
 * Remote Parametr Setter GUI
 * SZARP
 *
 * $Id$
 */

#include "pscgui.h"
#include <wx/valgen.h>
#include <openssl/err.h> 
#include <wx/xrc/xmlres.h>
#include <wx/listimpl.cpp>
#include <wx/spinctrl.h>
#include <wx/hashmap.h>
#include <wx/choicdlg.h>
#include "cconv.h"
#include "biowxsock.h"
#include "authdiag.h"
#include "xmlutils.h"

#include "../../resources/wx/icons/szast.xpm"


#define X (xmlChar*)

namespace {

wxString IntToStr(int val, int prec) {
	double dval = val;
	for (int i = 0; i < prec; ++i)
		dval /= 10;

	wxString s;
	s.Printf(_T("%.*f"), prec, dval);

	return s;
}

bool StrToInt(wxString str, int prec, int *ret) {
	double val;
	if (str.ToDouble(&val) == false)
		return false;

	for (int i = 0; i < prec; ++i) 
		val *= 10;
	*ret = (int)val;

	return true;
}

}

namespace {

wxString DayNumberToName(int day) {
	switch (day)
	{
		case  1:
			return _("Monday");
		case  2:
			return _("Tuesday");
		case  3:
			return _("Wednesday");
		case  4:
			return _("Thursday");
		case  5:
			return _("Friday");
		case  6:
			return _("Saturday");
		case  7:
			return _("Sunday");
		default:
			assert(false);	
	}
}

wxString FormatPackDate(const PackTime& pt) {
	wxString time = wxString::Format(_T("%.2d:%.2d"), pt.hour, pt.minute);
	if (pt.day)
		time = DayNumberToName(pt.day) 
			+ _T(" ") 
			+ time;
	return time;
}

}

PscFrame::PscFrame(wxWindow * parent, wxWindowID id)
{
	wxXmlResource::Get()->LoadFrame(this, NULL, _T("psc"));

	m_packs_widget = new PscPacksWidget(this);

	m_notebook = wxDynamicCast(FindWindow(XRCID("psc_notebook")), wxNotebook); 
	assert(m_notebook);

	wxPanel *panel_c = wxDynamicCast(m_notebook->FindWindow(XRCID("psc_constants_notebook_pane")), wxPanel);
	m_const_grid = new wxGrid(panel_c, -1, wxDefaultPosition, wxSize(600, 250));

	m_const_grid->CreateGrid(50, 4);
	m_const_grid->EnableEditing(true);
	wxGridCellAttr* ro = new wxGridCellAttr();
	ro->SetReadOnly(true);

	m_const_grid->SetColAttr(0, ro);
	m_const_grid->SetColAttr(2, ro);
	m_const_grid->SetColAttr(3, ro);
	m_const_grid->SetColLabelValue(0, _("Number"));
	m_const_grid->SetColLabelValue(1, _("Value"));
	m_const_grid->SetColLabelValue(2, _T(" "));
	m_const_grid->SetColLabelValue(3, _("Description"));
	m_const_grid->SetColSize(2, 30);
	m_const_grid->SetColSize(3, 500);

	panel_c->GetSizer()->Add(m_const_grid, 1, wxEXPAND);

	wxPanel *panel_p = wxDynamicCast(m_notebook->FindWindow(XRCID("psc_packs_notebook_pane")), wxPanel);
	m_pack_grid = new wxGrid(panel_p, -1, wxDefaultPosition, wxSize(600, 250));
	m_pack_grid->EnableEditing(false);

	m_pack_grid->CreateGrid(50, 2);
	m_pack_grid->SetColLabelValue(0, _("Number"));
	m_pack_grid->SetColLabelValue(1, _("Description"));
	m_pack_grid->SetColSize(1, 500);
	panel_p->GetSizer()->Add(m_pack_grid, 1, wxEXPAND);

	wxIcon icon(szast_xpm);
	if (icon.Ok())
		SetIcon(icon);
	
	m_main_sizer = m_notebook->GetContainingSizer();

	SetSize(900, 500);

}

void PscFrame::OnPSetDResponse(PSetDResponse &event) {
	DoHandlePsetDResponse(event.Ok(), event.GetResponse());
}

void PscFrame::OnCloseMenuItem(wxCommandEvent &event) {
	DoHandleCloseMenuItem(event);
}

void PscFrame::OnReloadMenuItem(wxCommandEvent &event) {
	DoHandleReload(event);
}

void PscFrame::OnConfigureMenuItem(wxCommandEvent &event) {
	DoHandleConfigure(event);
}

void PscFrame::OnSetTypeMenuItem(wxCommandEvent &event) {
	const wxString wd[] = { _("WEEK"), _("DAY") };
	const wxString dw[] = { _("DAY"), _("WEEK") };

	wxString r = wxGetSingleChoice(_("Choose type of packs\n(All packs values will be reset)"), 
			_("Choose packs' type"),
			2,
			m_pack_type == WEEKS ? wd : dw);

	if (r == wxEmptyString)
		return;

	PackType pt = r == _("WEEK") ? WEEKS : DAYS;
		 
	DoHandleSetPacksType(pt);
}

void PscFrame::OnResetMenutItem(wxCommandEvent &event) {
	DoHandleReset(event);
}

void PscFrame::OnSetPacksMenuItem(wxCommandEvent &event) {
	DoHandleSetPacks(event);
}

void PscFrame::OnSetConstantsMenuItem(wxCommandEvent &event) {
	DoHandleSetConstants(event);
}

void PscFrame::OnSaveReportMenuItem(wxCommandEvent& event) {
	DoHandleSaveReport(event);

}

void PscFrame::OnGetReportMenuItem(wxCommandEvent& event) {
	DoHandleGetReport(event);
}


void PscFrame::SetConstsValues(std::map<int, int> &const_values) {
	ParamHolder::SetConstsValues(const_values);

	wxGrid *grid = m_const_grid;
	grid->ClearGrid();

	int grid_line = 0;
	for (std::map<int, int>::iterator i = m_consts_values.begin();
			i != m_consts_values.end();
			++i) {

		if (m_consts_info.find(i->first) == m_consts_info.end())
			continue;

		PscParamInfo *d = m_consts_info[i->first];
		grid->SetCellValue(grid_line, 0, wxString::Format(_T("%d"), i->first));
		grid->SetCellValue(grid_line, 1, IntToStr(i->second, d->GetPrec()));
		grid->SetCellValue(grid_line, 2, d->GetUnit());
		grid->SetCellValue(grid_line, 3, d->GetDescription());
		grid_line++;
	}


}

void PscFrame::SetPacksValues(PacksParamMapper &packs) {
	ParamHolder::SetPacksValues(packs);

	wxGrid *grid = m_pack_grid;
	grid->ClearGrid();

	int grid_line = 0;
	for (std::map<int, PscParamInfo*>::iterator i = m_packs_info.begin();
			i != m_packs_info.end();
			i++, grid_line++) {

		PscParamInfo* pi = i->second;
		grid->SetCellValue(grid_line, 0, wxString::Format(_T("%d"), i->first));
		grid->SetCellValue(grid_line, 1, pi->GetDescription());
	}
}

void PscFrame::SetPacksType(PackType pt) {
	ParamHolder::SetPacksType(pt);

	wxStatusBar* sb = GetStatusBar();

	wxString sbs(_("Packs type:"));
	sbs += _T(" ");
	if (pt == WEEKS)
		sbs += _("weekly");
	else
		sbs += _("daily");

	sb->SetStatusText(sbs);
}


namespace {

struct ErrorMessage {
	const wxChar* server_response;	
	const wxChar* message;
}

const ErrorMessages[] = {
        { _T("*Regulator*communication*error*"), _("Regulator communication error.") },
        { _T("*failed to find daemon*"), _("Failed to communicate with daemon - is SZARP running?")},
        { _T("*failed to stop daemon*"), _("Failed to stop daemon - this is potentially serious error. Please contact PRATERM IT department asap!")},
        { _T("Unable to open port*"), _("Failed to open port - hardware or configuration error.") },
        { _T("*Regualtor*not*respond*"), _("Regulator does not respond to queries.") },
        { _T("*Wrong*username*password*"), _("Bad username or password.") },
        { _T("*Invalid*credentials*"), _("You are not permitted to change parameters for this object.") },
        { _T("*Unable to communiate with the regulator*"), _("Cannot read settings from regulator.") }
};

}

wxString PSetError(wxString error) {
	const size_t count = sizeof(ErrorMessages)/sizeof(ErrorMessage);

	for (size_t i = 0; i < count; ++i) {
		const ErrorMessage& msg = ErrorMessages[i];
		if (error.Matches(msg.server_response)) {
			return wxGetTranslation(msg.message);
		}
	}

	return error;
}

void PscFrame::OnGridEdited(wxGridEvent & evt) {
	wxGrid* grid = (wxGrid*) evt.GetEventObject();
	if (grid != m_const_grid)
		return;

	int row = evt.GetRow();
	int col = evt.GetCol();

	if (row >= (int)m_consts_info.size())
		return;

	std::map<int, PscParamInfo*>::iterator i = m_consts_info.begin();
	std::advance(i, evt.GetRow());

	PscParamInfo* pi = i->second;

	int max = pi->GetMax();
	int min = pi->GetMin();
	int number = pi->GetNumber();
	int prec = pi->GetPrec();
		
	wxString sval = grid->GetCellValue(row, col);
	int val;
	if (StrToInt(sval, prec, &val) == false) {
		wxMessageBox(_("Value must be a number."), _("Illegal value."));
		return;
	}

	if (val > max) 
		wxMessageBox(_("Value must be no greater than: ") + IntToStr(max, prec), _("Out of range"));
	else if (val < min) 
		wxMessageBox(_("Value must be no less than: ") + IntToStr(min, prec), _("Out of range"));
	else {
		m_consts_values[number] = val;
		evt.Skip();
	}

	grid->SetCellValue(row, col, IntToStr(m_consts_values[number], prec));
}

void PscFrame::OnGridEdit(wxGridEvent & evt)
{
	wxGrid* grid = wxDynamicCast(evt.GetEventObject(), wxGrid);
	evt.Skip();
	if (grid != m_pack_grid) 
		return;

	int row = evt.GetRow();
	if (row >= (int) m_packs_info.size())
		return;

	grid->SelectRow(row);

	std::map<int, PscParamInfo*>::iterator pi = m_packs_info.begin();
	std::advance(pi, row);

	m_packs_widget->ShowModal(&m_packs_values,
			&m_packs_info,
			pi->second->GetNumber(), 
			m_pack_type);
}

void PscFrame::StartWaiting() {
	wxList children = GetChildren();
	for (wxList::Node *node = children.GetFirst(); node; node = node->GetNext()) {
		wxWindow *current = (wxWindow*) node->GetData();
		current->Disable();
	}

	wxMenuBar* menu = GetMenuBar();
	menu->Enable(XRCID("psc_settings_item"), false);
	menu->Enable(XRCID("psc_refresh_item"), false);
	menu->Enable(XRCID("psc_set_constants_item"), false);
	menu->Enable(XRCID("psc_set_packs_type_item"), false);
	menu->Enable(XRCID("psc_set_packs_item"), false);
	menu->Enable(XRCID("psc_reset_setting_item"), false);

	wxStatusBar *stat_bar = GetStatusBar();
	stat_bar->SetStatusText(_("Awaiting response."), 0);

	SetCursor(*wxHOURGLASS_CURSOR);
}

void PscFrame::StopWaiting() {
	wxList children = GetChildren();
	for (wxList::Node *node = children.GetFirst(); node; node = node->GetNext()) {
		wxWindow *current = (wxWindow*) node->GetData();
		current->Enable(true);
	}

	wxMenuBar* menu = GetMenuBar();
	menu->Enable(XRCID("psc_settings_item"), true);
	menu->Enable(XRCID("psc_refresh_item"), true);
	menu->Enable(XRCID("psc_set_constants_item"), true);
	menu->Enable(XRCID("psc_set_packs_type_item"), true);
	menu->Enable(XRCID("psc_set_packs_item"), true);
	menu->Enable(XRCID("psc_reset_setting_item"), true);

	SetCursor(*wxSTANDARD_CURSOR);

}

void PscFrame::EnableEditingControls(bool enable) {
	for (size_t i = 1; i < 3; i++) {
		wxWindow* p = m_notebook->GetPage(i);
		wxList children = p->GetChildren();
		for (wxList::Node *node = children.GetFirst(); node; node = node->GetNext()) {
			wxWindow *current = (wxWindow*) node->GetData();
			current->Enable(enable);
		}
	}

	wxMenuBar* menu = GetMenuBar();
	menu->Enable(XRCID("psc_set_constants_item"), enable);
	menu->Enable(XRCID("psc_set_packs_type_item"), enable);
	menu->Enable(XRCID("psc_set_packs_item"), enable);
	menu->Enable(XRCID("psc_reset_setting_item"), enable);
}

PscParamInfo::PscParamInfo(bool is_pack, int number, int min, int max, int prec, wxString unit, wxString desc, wxString param_name)
	: m_is_pack(is_pack), m_number(number), m_min(min), m_max(max), m_prec(prec), m_unit(unit), m_description(desc), m_param_name(param_name)
{}

wxString PscParamInfo::GetDescription() {
	wxString description = m_description;
	if (description == wxEmptyString) {
		int idx = m_param_name.Find(L':', true);
		if (idx >= 0)
			description = m_param_name.Mid(idx + 1);
		else
			description = m_param_name;
	}
	return description;
}

PscPacksWidget::PscPacksWidget(PscFrame* parent)
	{

	wxXmlResource::Get()->LoadDialog(this, parent, _T("psc_pack_edit"));
	m_pack_edit = new PscPackEdit(this);
	m_tav_list = XRCCTRL(*this, "list_tav", wxListCtrl);
	m_context = ((wxMenuBar*) wxXmlResource::Get()->LoadObject(this,_T("psc_menubar"),_T("wxMenuBar")))->GetMenu(0);

	SetSize(440, 500);
	SetColumns();
}

void PscPacksWidget::SetColumns() {
	m_tav_list->InsertColumn(0, _("Beginning"));
	m_tav_list->SetColumnWidth(0, 150);
	m_tav_list->InsertColumn(1, _("Value"));
}

void PscPacksWidget::OnContextMenu(wxListEvent& evt) {
	m_index = evt.GetIndex();
	PopupMenu(m_context);
}

void PscPacksWidget::OnMenuEdit(wxCommandEvent& evt) {
	Edit();
}

void PscPacksWidget::OnMenuAdd(wxCommandEvent& evt) {

	int ret = m_pack_edit->ShowModal(m_pack_type,  (*m_packs_info)[m_number]);
	if (ret != wxID_OK)
		return;

	int val;
	PackTime pt; 
	pt.day = m_pack_edit->GetDay();
	pt.hour = m_pack_edit->GetHour();
	pt.minute = m_pack_edit->GetMinute();
	val = m_pack_edit->GetValue();

	m_mapper->Insert(pt, m_number, val);
	m_mapper->Canonicalize();

	UpdateGrid();
}

PackTime PscPacksWidget::GetCurrentTime() {
	std::map<PackTime, int>::iterator i = m_pack_vals.begin();
	std::advance(i, m_index);
	return i->first;
}

void PscPacksWidget::OnMenuRemove(wxCommandEvent& evt) {
	if (m_index < 0)
		return;

	if (m_pack_vals.size() == 1) {
		wxMessageDialog dlg(this, 
					_("At least one pack must be defined."),
					_("Error"),
					wxOK | wxICON_HAND);
	
		dlg.ShowModal();
		return;
	}

	wxMessageDialog dlg(this, 
				_("Do you really want to remove this pack?"),
				_("Pack removal"),
				wxYES_NO);

	if (dlg.ShowModal() != wxID_YES) 
		return;

	m_mapper->Remove(GetCurrentTime(), m_number);
	m_mapper->Canonicalize();

	UpdateGrid();
}

void PscPacksWidget::Edit() {
	PackTime pt = GetCurrentTime(); 
	int val = m_pack_vals[pt];

	int ret = m_pack_edit->ShowModal(pt.day,
					pt.hour,
					pt.minute,
					val,
					(*m_packs_info)[m_number]);
	if (ret != wxID_OK)
		return;

	PackTime npt;
	npt.day = m_pack_edit->GetDay();
	npt.hour = m_pack_edit->GetHour();
	npt.minute = m_pack_edit->GetMinute();
	val = m_pack_edit->GetValue();

	m_mapper->Remove(pt, m_number);
	m_mapper->Insert(npt, m_number, val);
	m_mapper->Canonicalize();

	UpdateGrid();
}

void PscPacksWidget::OnSelected(wxListEvent& evt) {
	m_index = evt.GetIndex();
}

void PscPacksWidget::UpdateGrid() {
	m_tav_list->ClearAll();
	SetColumns();

	wxString prev_value;
	m_pack_vals = m_mapper->GetParamValues(m_number);

	PscParamInfo *pi = (*m_packs_info)[m_number];
	int prec = pi->GetPrec();

	int j = 0;
	for (std::map<PackTime, int>::iterator i = m_pack_vals.begin();
		       i != m_pack_vals.end();
		       i++, j++) {

		long item = m_tav_list->InsertItem(j, FormatPackDate(i->first));
		m_tav_list->SetItem(item, 1, IntToStr(i->second, prec) + _T(" ") + pi->GetUnit());
	}
}

int PscPacksWidget::ShowModal(PacksParamMapper* mapper, 
			std::map<int, PscParamInfo*>* packs,
			int number, 
			PackType pack_type) {
	m_mapper = mapper;
	m_packs_info = packs;

	m_index = -1;
	m_number = number;
	m_pack_type = pack_type;

	UpdateGrid();
	return wxDialog::ShowModal();
}

void PscPacksWidget::OnClose(wxCommandEvent &event) {
	if (IsModal()) 
		EndModal(wxID_OK);
	else {
		SetReturnCode(wxID_OK);
		Show(false);
	}
}

PscPackEdit::PscPackEdit(PscPacksWidget* parent) {
	wxXmlResource::Get()->LoadDialog(this, parent, _T("psc_date_edit"));
	m_day = XRCCTRL(*this, "combo_day", wxComboBox);
	m_hour = XRCCTRL(*this, "spin_ctrl_hour", wxSpinCtrl);
	m_minute = XRCCTRL(*this, "spin_ctrl_minute", wxSpinCtrl);
	m_value_text = XRCCTRL(*this, "text_value", wxTextCtrl);
}


bool PscPackEdit::TransferDataFromWindow() {
	wxString sval = m_value_text->GetValue();

	int max = m_info->GetMax();
	int min = m_info->GetMin();
	int prec = m_info->GetPrec();

	int rval = 0;
	if (StrToInt(sval, prec, &rval) == false)
		wxMessageBox(_("Value must be number."), _("Must be number."), wxOK);
	else if (rval > max)
		wxMessageBox(wxString(_("Value must be not greater than: ")) + IntToStr(max, prec), _("Out of range"), wxOK);
	else if (rval < min) 
		wxMessageBox(wxString(_("Value must be not less than: ")) + IntToStr(min, prec), _("Out of range"), wxOK);
	else {
		m_value = rval;
		return true;
	}

	return false;

}

void PscPackEdit::UpdateBounds() {
	wxStaticText* min = XRCCTRL(*this, "psc_min_value_label", wxStaticText);
	wxStaticText* max = XRCCTRL(*this, "psc_max_value_label", wxStaticText);

	min->SetLabel(IntToStr(m_info->GetMin(), m_info->GetPrec()) + _T(" ") + m_info->GetUnit());
	max->SetLabel(IntToStr(m_info->GetMax(), m_info->GetPrec()) + _T(" ") + m_info->GetUnit());

}

int PscPackEdit::ShowModal(int day, int hour, int minute, int value, PscParamInfo *info) {
	m_info = info;
	if (day != 0) {
		m_day->Enable();
		m_day->SetSelection(day - 1);
	} else
		m_day->Disable();

	UpdateBounds();

	m_hour->SetValue(hour);
	m_minute->SetValue(minute);
	m_value_text->SetValue(IntToStr(value, m_info->GetPrec()));

	return  wxDialog::ShowModal();
}

int PscPackEdit::ShowModal(PackType pack_type, PscParamInfo *info) {
	m_info = info;
	if (pack_type == WEEKS) {
		m_day->Enable();
		m_day->SetSelection(0);
	} else
		m_day->Disable();

	UpdateBounds();

	m_hour->SetValue(0);
	m_minute->SetValue(0);
	m_value_text->SetValue(_T(""));

	return  wxDialog::ShowModal();
}

int PscPackEdit::ShowModal() {
	return wxDialog::ShowModal();
}

int PscPackEdit::GetDay() {
	if (m_day->IsEnabled() == false)
		return 0;

	return m_day->GetSelection() + 1;
}

int PscPackEdit::GetHour() {
	return m_hour->GetValue();
}

int PscPackEdit::GetMinute() {
	return m_minute->GetValue();
}

int PscPackEdit::GetValue() {
	return m_value;
}

BEGIN_EVENT_TABLE(PscFrame, wxFrame)
    EVT_MENU(XRCID("psc_set_constants_item"), PscFrame::OnSetConstantsMenuItem)
    EVT_MENU(XRCID("psc_set_packs_type_item"), PscFrame::OnSetTypeMenuItem)
    EVT_MENU(XRCID("psc_set_packs_item"), PscFrame::OnSetPacksMenuItem)
    EVT_MENU(XRCID("psc_close_item"), PscFrame::OnCloseMenuItem)
    EVT_MENU(XRCID("psc_refresh_item"), PscFrame::OnReloadMenuItem)
    EVT_MENU(XRCID("psc_settings_item"), PscFrame::OnConfigureMenuItem)
    EVT_MENU(XRCID("psc_reset_setting_item"), PscFrame::OnResetMenutItem)
    EVT_MENU(XRCID("psc_fetch_raport"), PscFrame::OnGetReportMenuItem)
    EVT_BUTTON(wxID_SAVE, PscFrame::OnSaveReportMenuItem)
    EVT_BUTTON(wxID_REFRESH, PscFrame::OnGetReportMenuItem)
    EVT_PSETD_RESP(-1, PscFrame::OnPSetDResponse)
    EVT_GRID_SELECT_CELL(PscFrame::OnGridEdit)
    EVT_GRID_CELL_CHANGE(PscFrame::OnGridEdited)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(PscPacksWidget, wxDialog)
    EVT_LIST_ITEM_SELECTED(XRCID("list_tav"), PscPacksWidget::OnSelected)
    EVT_LIST_ITEM_ACTIVATED(XRCID("list_tav"), PscPacksWidget::OnContextMenu)
    EVT_LIST_ITEM_RIGHT_CLICK(XRCID("list_tav"), PscPacksWidget::OnContextMenu)
    EVT_MENU(XRCID("Add"), PscPacksWidget::OnMenuAdd)
    EVT_MENU(XRCID("Remove"), PscPacksWidget::OnMenuRemove)
    EVT_MENU(XRCID("Edit"), PscPacksWidget::OnMenuEdit)
    EVT_BUTTON(wxID_CLOSE, PscPacksWidget::OnClose)
    EVT_BUTTON(wxID_ADD, PscPacksWidget::OnMenuAdd)
    EVT_BUTTON(wxID_DELETE, PscPacksWidget::OnMenuRemove)
END_EVENT_TABLE()

const int PSetDConnection::max_buffer_size = 512 * 16;

PSetDConnection::PSetDConnection(const wxIPV4address &ip, wxEvtHandler *receiver) : 
	m_ip(ip),
	m_buffer(NULL),
	m_buffer_pos(0),
	m_buffer_size(0),
	m_socket(NULL), 
	m_receiver(receiver),
	m_state(IDLE)
	{}

void PSetDConnection::Cleanup() {
	FreeBuffer();

	m_socket->Destroy();
	m_socket = NULL;

	m_state = IDLE;

}

void PSetDConnection::FreeBuffer() {

	if (m_buffer) {
		xmlFree(m_buffer);
		m_buffer = NULL;
	}

	m_buffer_pos = 0;
	m_buffer_size = 0;

}
	

bool PSetDConnection::WriteToServer() {

	while (m_buffer_pos < m_buffer_size) {
		ssize_t ret = m_socket->Write((char*)m_buffer + m_buffer_pos, m_buffer_size - m_buffer_pos);

		if (ret <= 0) {
			if (m_socket->LastError() != wxSOCKET_WOULDBLOCK)
				Error();
			return false;
		}

		m_buffer_pos += ret;
	}

	return true;
}	

bool PSetDConnection::ReadFromServer() {

	while (true) {
		ssize_t ret = m_socket->Read((char*) m_buffer + m_buffer_pos, m_buffer_size - m_buffer_pos);

		if (ret <= 0) {
			if (m_socket->LastError() != wxSOCKET_WOULDBLOCK)
				Error();
			return false;
		}

		m_buffer_pos += ret;

		if (m_buffer[m_buffer_pos - 1] == '\0')
			break;

		if (m_buffer_pos == m_buffer_size) {
			m_buffer_size *= 2;
			m_buffer = (xmlChar* )xmlRealloc(m_buffer, m_buffer_size);

			if (m_buffer_size > max_buffer_size) {
				Error();
				return false;
			}
		}
	}

	return true;

}

void PSetDConnection::Connect() {
	m_socket->Connect();
}

bool PSetDConnection::QueryServer(xmlDocPtr query) {
	if (m_state != IDLE)
		return false;

	xmlDocDumpMemory(query, &m_buffer, &m_buffer_size);
	m_buffer_size++;
	m_buffer = (xmlChar*) xmlRealloc(m_buffer, m_buffer_size);
	m_buffer[m_buffer_size - 1] = '\0';

	m_socket = new SSLSocketConnection(m_ip, this);

	m_state = CONNECTING;

	Drive();

	return true;
};

void PSetDConnection::Drive() {
	switch (m_state) {
		case CONNECTING:
			Connect();
			return;
		case WRITING:
			if (!WriteToServer())
				return;
			m_buffer_pos = 0;
			m_state = READING;
		case READING:
			if (!ReadFromServer())
				return;
			Finish();
			m_state = IDLE;
		case IDLE:
			break;
	}
}
					

void PSetDConnection::Cancel() {
	Cleanup();
}

void PSetDConnection::Finish() {
	xmlDocPtr doc = xmlParseMemory((char*)m_buffer, m_buffer_pos);
	if (doc == NULL) {
		Error();
		return;
	}
	xmlDocFormatDump(stdout, doc, 1);
	PSetDResponse event(true, doc);
	wxPostEvent(m_receiver, event);
	Cleanup();
}

void PSetDConnection::Error() {
	wxLogInfo(_T("Failed to fetch data drom server"));
	PSetDResponse event(false, NULL);
	wxPostEvent(m_receiver, event);
	Cleanup();
}

void PSetDConnection::SocketEvent(wxSocketEvent& event) {
	if (m_state == IDLE) 
		return;

	wxSocketNotify type = event.GetSocketEvent();
	if (type == wxSOCKET_LOST) {
		Error();
		return;
	}

	if (m_state == CONNECTING) {
		assert(type == wxSOCKET_CONNECTION);
		m_state = WRITING;
	}

	Drive();
}	

BEGIN_EVENT_TABLE(PSetDConnection, wxEvtHandler)
    EVT_SOCKET(wxID_ANY, PSetDConnection::SocketEvent)
END_EVENT_TABLE()

DEFINE_EVENT_TYPE(PSETD_RESP)

PSetDResponse::PSetDResponse(bool ok, xmlDocPtr response) : wxCommandEvent(PSETD_RESP, wxID_ANY),
	m_ok(ok), m_response(response) {}

wxEvent* PSetDResponse::Clone() const {
	return new PSetDResponse(m_ok, m_response);
}

bool PSetDResponse::Ok() const {
	return m_ok;
}

xmlDocPtr PSetDResponse::GetResponse() const {
	return m_response;
}

PacksParamMapper::PacksParamMapper() {}

void PacksParamMapper::Reset() {
	m_packs.clear();
}

bool PacksParamMapper::HasPacks() {
	return m_params.size() > 0;
}

void PacksParamMapper::Canonicalize() {

	std::map<PackTime, std::map<int,int> >::iterator i =  m_packs.begin();
	std::map<int, int> prev_vals = i->second;
	std::map<PackTime, std::map<int, int> > result;

	result[i->first] = i->second;

	for (; i != m_packs.end(); ++i) {

		std::map<int, int> result_vals;
		std::map<int, int>& current_vals = i->second;

		for (std::map<int, int>::iterator j = current_vals.begin();
				j != current_vals.end();	
				j++) {

			int param_no = j->first;
			int param_val = j->second;

			if (param_val != prev_vals[param_no]) 
				result_vals[param_no] = param_val;
		}

		if (result_vals.size() == 0)
			continue;

		result[i->first] = result_vals;
		
		for (std::map<int, int>::iterator j = result_vals.begin();
				j != result_vals.end();
				j++) 
			prev_vals[j->first] = j->second;

	}

	m_packs = result;

}

void PacksParamMapper::Insert(const PackTime &pt, int param, int value) {
	m_params.insert(param);
	m_packs[pt][param] = value;
}

void PacksParamMapper::Remove(const PackTime &pt, int param) {
	std::map<int, int>& params = m_packs[pt];
	params.erase(param);
	if (params.size() == 0)
		m_packs.erase(pt);
}

std::map<PackTime, int> PacksParamMapper::GetParamValues(int param) {
	std::map<PackTime, int> result;

	for (std::map<PackTime, std::map<int,int> >::iterator i =  m_packs.begin();
			i != m_packs.end();
			++i) {

		std::map<int, int>& params = i->second;
		if (params.find(param) == params.end())
			continue;

		result.insert(std::pair<PackTime, int>(i->first, params[param]));

	}

	return result;
}

bool PackTime::operator<(const PackTime &pt) const {
	if (day < pt.day)
		return true;
	if (day > pt.day)
		return false;

	if (hour < pt.hour)
		return true;
	if (hour > pt.hour)
		return false;

	if (minute < pt.minute)
		return true;
	if (minute > pt.minute)
		return false;

	return false;

}

bool PacksParamMapper::FindFirstValue(int param, int *val) {
	std::map<PackTime, std::map<int, int> >::iterator i = m_packs.begin();
	if (i->second.find(param) != i->second.end()) {
		*val = i->second[param];
		return true;
	}

	for (std::map<PackTime, std::map<int, int> >::reverse_iterator ri = m_packs.rbegin();
			ri != m_packs.rend();
			ri++)
		if (ri->second.find(param) != ri->second.end()) {
			*val = ri->second[param];
			return true;
		}

	return false;
}

std::map<PackTime, std::map<int, int> > PacksParamMapper::GetMapping() {
	std::map<int, int> all_vals;

	for (std::set<int>::iterator i = m_params.begin(); 
			i != m_params.end();
			++i) {
		int val;
		FindFirstValue(*i, &val);
		all_vals[*i] = val;
	}

	std::map<PackTime, std::map<int, int> > result;

	for (std::map<PackTime, std::map<int,int> >::iterator i = m_packs.begin();
			i != m_packs.end();
			++i) {

		std::map<int, int>& vals = i->second;
		for (std::map<int, int>::iterator j = vals.begin();
				j != vals.end();
				j++)
			all_vals[j->first] = j->second;

		result[i->first] = all_vals;

	}

	return result;
}

bool PscRegulatorData::ParseResponse(xmlDocPtr document) {
	m_ok = false;

	xmlDocDump(stderr, document);

	xmlNodePtr root = document->xmlChildrenNode;
	if (xmlStrcmp(root->name, X "message") == 0) {
		xmlChar *_type = xmlGetProp(root, X "type");
		wxString type = SC::U2S(_type);
		xmlFree(_type);

		if (type == _T("regulator_settings")) {
			m_ok = true;
		} else {
			root = root->xmlChildrenNode;
			while (xmlStrcmp(root->name, X "reason") != 0)
				root = root->next;
			root = root->xmlChildrenNode;
			while (xmlStrcmp(root->name, X "text") != 0)
				root = root->next;
			m_error = PSetError(SC::U2S(XML_GET_CONTENT(root)));

			xmlFreeDoc(document);

			return true;
		}

	} else {
		xmlFreeDoc(document);
		return false;
	}

	xmlNodePtr node;
	for (node = root->xmlChildrenNode;
			node != NULL;
			node = node->next) 
		if (!xmlStrcmp(node->name, X "unit"))
			break;

	bool ret = true;

	if (node != NULL) {
	
		m_eprom = m_program = m_library = m_build = _T("");
	
		xmlChar *_eprom = xmlGetProp(node, X "nE");
		if (_eprom)
			m_eprom = SC::U2S(_eprom);
		xmlFree(_eprom);
	
		xmlChar *_program = xmlGetProp(node, X "nP");
		if (_program)
			m_program = SC::U2S(_program);
		xmlFree(_program);
	
		xmlChar *_library = xmlGetProp(node, X "nL");
		if (_library)
			m_library = SC::U2S(_library);
		xmlFree(_library);
	
		xmlChar *_build = xmlGetProp(node, X "nB");
		if (_build)
			m_build = SC::U2S(_build);
		xmlFree(_build);
	
		ParseConstsResponse(root);
		ParsePacksResponse(root);

	} else
		ret = false;

	xmlFreeDoc(document);
	return ret;
}

void PscRegulatorData::ParseConstsResponse(xmlNodePtr root) {
	xmlNodePtr consts;
	for (consts = root->xmlChildrenNode;
			consts != NULL && xmlStrcmp(consts->name, X "constants");
			consts = consts->next);

	if (consts == NULL)
		return;

	for (xmlNodePtr child = consts->xmlChildrenNode;
			child != NULL;
			child = child->next) {

		if (xmlStrcmp(child->name, X "param"))
			continue;

		xmlChar* _number = xmlGetProp(child, X "number");
		xmlChar* _value = xmlGetProp(child, X "value");

		int number = atoi((char*) _number);
		int value = atoi((char*) _value);

		xmlFree(_number);
		xmlFree(_value);

		m_consts[number] = value;
	}
}

void PscRegulatorData::ParsePacksResponse(xmlNodePtr root) {
	xmlNodePtr packs;
	for (packs = root->xmlChildrenNode; 
			packs != NULL && xmlStrcmp(packs->name, X "packs"); 
			packs = packs->next);

	if (packs == NULL)
		return;

	xmlChar *type = xmlGetProp(packs, X "type");
	if (!xmlStrcmp(type, X "DAY"))
		m_pack_type = DAYS;
	else if (!xmlStrcmp(type, X "WEEK")) 
		m_pack_type = WEEKS;
	else
		assert(false);
	xmlFree(type);

	m_packs.Reset();

	for (xmlNodePtr child = packs->xmlChildrenNode;
			child != NULL;
			child = child->next) {

		if (xmlStrcmp(child->name, X "pack"))
			continue;

		PackTime pt;

		if (m_pack_type == WEEKS) {
			xmlChar *_day = xmlGetProp(child, X "day");
			pt.day = atoi((char*) _day);
			xmlFree(_day);
			if (pt.day == 0)
				pt.day = 1;
		} else
			pt.day = 0;

		xmlChar* _hour = xmlGetProp(child, X "hour");
		xmlChar* _minute = xmlGetProp(child, X "minute");

		pt.hour = atoi((char*) _hour);
		pt.minute = atoi((char*) _minute);

		xmlFree(_hour);
		xmlFree(_minute);

		for (xmlNodePtr grand_child = child->xmlChildrenNode; 
				grand_child != NULL; 
				grand_child = grand_child->next) {

			if (xmlStrcmp(grand_child->name, X "param"))
				continue;

			xmlChar* _number = xmlGetProp(grand_child, X "number");
			xmlChar* _value = xmlGetProp(grand_child, X "value");

			int number = atoi((char*) _number);
			int value = atoi((char*) _value);

			xmlFree(_number);
			xmlFree(_value);

			m_packs.Insert(pt, number, value);

		}
	}

	if (m_packs.HasPacks())
		m_packs.Canonicalize();

}

bool PscReport::ParseResponse(xmlDocPtr doc) {
	if (doc == NULL)
		return false;

	xmlNodePtr root = doc->xmlChildrenNode;
	if (root == NULL)
		return false;

	if (xmlStrcmp(root->name, X"message"))
		return false;

	xmlChar *_type = xmlGetProp(root, X"type");
	if (_type == NULL || xmlStrcmp(_type, X"report")) {
		xmlFree(_type);
		xmlFreeDoc(doc);
		return false;
	}
	xmlFree(_type);

	xmlChar* _year = xmlGetProp(root, X"year");
	xmlChar* _month = xmlGetProp(root, X"month");
	xmlChar* _day = xmlGetProp(root, X"day");
	xmlChar* _hour = xmlGetProp(root, X"hour");
	xmlChar* _minute = xmlGetProp(root, X"minute");


	if (_year == NULL || _month == NULL || _day == NULL || _minute == NULL) {
		xmlFree(_year); xmlFree(_month); xmlFree(_day); xmlFree(_hour); xmlFree(_minute);
		xmlFreeDoc(doc);
		return false;
	}

	wxDateTime rtime(
			atoi((char*) _day),
			wxDateTime::Month(atoi((char*) _month) - 1),
			atoi((char*) _year) + 2000,
			atoi((char*) _hour),
			atoi((char*) _minute));

	xmlFree(_year); xmlFree(_month); xmlFree(_day); xmlFree(_hour); xmlFree(_minute);

	if (rtime.IsValid() == false) {
		xmlFreeDoc(doc);
		return false;
	}

	wxString nE, nL, nb, np;

	xmlChar* _nE = xmlGetProp(root, X"nE");
	xmlChar* _nL = xmlGetProp(root, X"np");
	xmlChar* _nb = xmlGetProp(root, X"nb");
	xmlChar* _np = xmlGetProp(root, X"np");

	if (_nE) 
		nE = SC::U2S(_nE);
	if (_nL)
		nL = SC::U2S(_nL);
	if (_nb)
		nb = SC::U2S(_nb);
	if (_np)
		np = SC::U2S(_np);

	xmlFree(_nE); xmlFree(_nL); xmlFree(_nb); xmlFree(_np);


	std::vector<short> v;
	for (xmlNodePtr param = root->xmlChildrenNode; param; param = param->next) {
		if (xmlStrcmp(param->name, X"param"))
			continue;

		xmlChar *_v = xmlGetProp(param, X"value");
		if (_v == NULL) {
			xmlFreeDoc(doc);
			return false;
		}

		v.push_back(atoi((char*)_v));
		xmlFree(_v);

	}
	xmlFreeDoc(doc);

	time = rtime;
	values = v;
	this->nE = nE;
	this->nL = nL;
	this->np = np;
	this->nb = nb;

	return true;

}

MessagesGenerator::MessagesGenerator(wxString username, wxString password, wxString path, wxString speed, wxString id) :
	m_path(path), m_speed(speed), m_id(id), m_has_credentials(true), m_username(username), m_password(password) {}

MessagesGenerator::MessagesGenerator(wxString path, wxString speed, wxString id) :
	m_path(path), m_speed(speed), m_id(id), m_has_credentials(false) {}

xmlDocPtr MessagesGenerator::StartMessage(xmlChar *type) {
	xmlDocPtr root = xmlNewDoc(X "1.0");
	root->children = xmlNewDocNode(root, NULL, X "message", NULL);
	xmlSetProp(root->children, X "type", type);

	xmlNodePtr device = xmlNewChild(root->children, NULL, X "device", NULL);
	xmlSetProp(device, X "path", SC::S2U(m_path).c_str());
	xmlSetProp(device, X "speed", SC::S2U(m_speed).c_str());
	xmlSetProp(device, X "id", SC::S2U(m_id).c_str());

	if (m_has_credentials) {
		xmlNodePtr cred = xmlNewChild(root->children, NULL, X "credentials", NULL);
		xmlSetProp(cred, X "username", SC::S2U(m_username).c_str());
		xmlSetProp(cred, X "password", SC::S2U(m_password).c_str());
	}

	return root;

}

xmlDocPtr MessagesGenerator::CreateGetReportMessage() {
	return StartMessage(X "get_report");
}

xmlDocPtr MessagesGenerator::CreateResetSettingsMessage() {
	return StartMessage(X "reset_packs");
}

xmlDocPtr MessagesGenerator::CreateRegulatorSettingsMessage() {
	return StartMessage(X "get_values");
}

xmlDocPtr MessagesGenerator::CreateSetPacksTypeMessage(PackType type) {
	xmlDocPtr root = StartMessage(X "set_packs_type");
	xmlNodePtr packs = xmlNewChild(root->children, NULL, X "packs", NULL);
	if (type == WEEKS)
		xmlSetProp(packs, X "type", X "WEEK");
	else
		xmlSetProp(packs, X "type", X "DAY");

	return root;
}

xmlDocPtr MessagesGenerator::CreateSetPacksMessage(PacksParamMapper &vals, PackType pack_type) {
	xmlDocPtr root = StartMessage(X "set_packs");
	xmlNodePtr packs = xmlNewChild(root->children, NULL, X "packs", NULL);

	if (pack_type == WEEKS)
		xmlSetProp(packs, X "type", X "WEEK");
	else 
		xmlSetProp(packs, X "type", X "DAY");

	// Generating xml
	std::map<PackTime, std::map<int, int> > map = vals.GetMapping();

	for (std::map<PackTime, std::map<int, int> >::iterator i = map.begin();
			i != map.end();
			i++) {
		xmlNodePtr pack = xmlNewChild(packs, NULL, X "pack", NULL);

		const PackTime &pt = i->first;
		std::map<int, int>& pvals = i->second;

		xmlSetProp(pack, X "day", SC::S2U(wxString::Format(_T("%d"), pt.day)).c_str());
		xmlSetProp(pack, X "hour", SC::S2U(wxString::Format(_T("%d"), pt.hour)).c_str());
		xmlSetProp(pack, X "minute", SC::S2U(wxString::Format(_T("%d"), pt.minute)).c_str());

		for (std::map<int, int>::iterator j = pvals.begin();
				j != pvals.end();
				j++) {
			xmlNodePtr param = xmlNewChild(pack, NULL, X "param", NULL);
			xmlSetProp(param, X "number", 
				SC::S2U(wxString::Format(_T("%d"), j->first)).c_str());
			xmlSetProp(param, X "value", 
				SC::S2U(wxString::Format(_T("%d"), j->second)).c_str());
		}
	
	}

	return root;
}

xmlDocPtr MessagesGenerator::CreateSetConstsMessage(std::map<int, int> &vals) {
	xmlDocPtr root = StartMessage(X "set_constants");
	xmlNodePtr constants = xmlNewChild(root->children, NULL, X "constants", NULL);

	for (std::map<int, int>::iterator i = vals.begin();
			i != vals.end();
			++i) {

		xmlNodePtr param = xmlNewChild(constants, NULL, X "param", NULL);
		xmlSetProp(param, X "number", SC::S2U(wxString::Format(_T("%d"), i->first)).c_str());
		xmlSetProp(param, X "value", SC::S2U(wxString::Format(_T("%d"), i->second)).c_str());

	}

	return root;
}

PacksParamMapper& ParamHolder::GetPacksValues() {
	return m_packs_values;
}

std::map<int, int>& ParamHolder::GetConstsValues() {
	return m_consts_values;
}

void ParamHolder::SetPacksType(PackType pt) {
	m_pack_type = pt;
}

PackType ParamHolder::GetPacksType() {
	return m_pack_type;
}

std::map<int, PscParamInfo*>& ParamHolder::GetConstsInfo() {
	return m_consts_info;
}

std::map<int, PscParamInfo*>& ParamHolder::GetPacksInfo() {
	return m_packs_info;
}

std::vector<PscParamInfo*> ParamHolder::GetParamsInfo() {
	std::vector<PscParamInfo*> ret;

	for (std::map<int, PscParamInfo*>::iterator i = m_packs_info.begin();
			i != m_packs_info.end();
			i++)
		ret.push_back(i->second);

	for (std::map<int, PscParamInfo*>::iterator i = m_consts_info.begin();
			i != m_consts_info.end();
			i++)
		ret.push_back(i->second);

	return ret;
}

void ParamHolder::SetConstsInfo(const std::map<int, PscParamInfo*>& pi) {
	m_consts_info = pi;
}

void ParamHolder::SetPacksInfo(const std::map<int, PscParamInfo*>& pi) {
	m_packs_info = pi;
}

void ParamHolder::SetConstsValues(std::map<int,int>& vals) {
	m_consts_values = vals;
}

void ParamHolder::SetPacksValues(PacksParamMapper &vals) {
	m_packs_values = vals;
}

void PscConfigurationUnit::ParseDefinition(xmlNodePtr device) {

	xmlChar *_id = xmlGetProp(device, X "unit");
	if (_id != NULL)
		m_id = SC::U2S(_id);
	else
		m_id = _T("1"); // default
	xmlFree(_id);

	xmlChar *_path = xmlGetProp(device, X "path");
	if (_path)
		m_path = SC::U2S(_path);
	xmlFree(_path);

	xmlChar *_eprom = xmlGetProp(device, X "nE");
	if (_eprom)
		m_eprom = SC::U2S(_eprom);
	xmlFree(_eprom);

	xmlChar *_program = xmlGetProp(device, X "nP");
	if (_program)
		m_program = SC::U2S(_program);
	xmlFree(_program);

	xmlChar *_library = xmlGetProp(device, X "nL");
	if (_library)
		m_library = SC::U2S(_library);
	xmlFree(_library);

	xmlChar *_build = xmlGetProp(device, X "nb");
	if (_build)
		m_build = SC::U2S(_build);
	xmlFree(_build);
	
	for (xmlNodePtr param = device->xmlChildrenNode; param != NULL; param = param->next)
		ParseParam(param);

}

void PscConfigurationUnit::ParseParam(xmlNodePtr node) {
	bool is_pack;
	if (xmlStrcmp(node->name, X "pack") == 0)
		is_pack = true;
	else if (xmlStrcmp(node->name, X "const") == 0)
		is_pack = false;
	else
		return;
		
	xmlChar* _number = xmlGetProp(node, X "number");
	xmlChar *_min = xmlGetProp(node, X "min");
	xmlChar *_max = xmlGetProp(node, X "max");
	xmlChar* _prec = xmlGetProp(node, X "prec");

	int number = atoi((char*)_number);
	int min = atoi((char*)_min);
	int max = atoi((char*)_max);
	int prec = atoi((char*)_prec);

	wxString unit, param_str, description;
		
	xmlChar* _unit = xmlGetProp(node, X "unit");
	if (_unit)
		unit = SC::U2S(_unit);
	else
		unit = _T("1");

	xmlChar* _param_str = xmlGetProp(node, X "connected_to");
	if (_param_str	!= NULL)
		param_str = SC::U2S(_param_str);

	xmlChar* _description = xmlGetProp(node, X "description");
	if (_description != NULL)
		description = SC::U2S(_description);

	if (!description.IsEmpty() || !param_str.IsEmpty()) {
		PscParamInfo *pi = new PscParamInfo(is_pack,
					number,
					min,
					max,
					prec,
					unit,
					description,
					param_str);

		if (is_pack)
			m_packs_info[number] = pi;
		else
			m_consts_info[number] = pi;
	}

	xmlFree(_number);
	xmlFree(_min);
	xmlFree(_max);
	xmlFree(_prec);
	xmlFree(_unit);
	xmlFree(_param_str);
}

bool PscConfigurationUnit::SwallowResponse(PscRegulatorData &rd) {
	if (rd.ResponseOK() == false)
		return false;

	if (m_eprom != rd.GetEprom()
			|| m_program != rd.GetProgram()
			|| m_library != rd.GetLibrary()
			|| m_build != rd.GetBuild())
		return false;

	m_consts_values = rd.GetConstsValues();
	m_packs_values = rd.GetPacksValues();
	m_pack_type = rd.GetPacksType();

	return true;
}

std::vector<PscConfigurationUnit*> ParsePscSystemConfiguration(wxString path) {
	std::vector<PscConfigurationUnit*> ret;

	xmlDocPtr doc = xmlParseFile(SC::S2A(path).c_str());
	if (doc == NULL)
		return ret;

	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(doc);
	xmlXPathObjectPtr xpath_obj = xmlXPathEvalExpression(BAD_CAST "//device", xpath_ctx);
	assert(xpath_obj != NULL);

	if (xpath_obj->nodesetval) for (int i = 0; i < xpath_obj->nodesetval->nodeNr; i++) {
		xmlNodePtr node = xpath_obj->nodesetval->nodeTab[i];
		if (node == NULL)
			continue;

		PscConfigurationUnit* psc = new PscConfigurationUnit();
		psc->ParseDefinition(node);
		ret.push_back(psc);

	}

	xmlXPathFreeObject(xpath_obj);
	xmlXPathFreeContext(xpath_ctx);
	xmlFreeDoc(doc);

	return ret;

}

