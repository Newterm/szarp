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
 * raporter3 program
 * SZARP
 *
 * $Id$
 */
#include "filedump.h"

BEGIN_EVENT_TABLE(FileDumpDialog, wxDialog)
	EVT_BUTTON(ID_OkBtn, FileDumpDialog::OnOkBtn)
	EVT_BUTTON(ID_CancelBtn, FileDumpDialog::OnCancelBtn)
	EVT_DIRPICKER_CHANGED(ID_DirChanged, FileDumpDialog::OnUpdateDestination)
	EVT_CLOSE(FileDumpDialog::OnClose)
END_EVENT_TABLE()

FileDumpDialog::FileDumpDialog(wxFrame *parent, wxString *report_name) 
	: wxDialog(
			parent, 
			ID_FileDumpDialog, 
			wxString(_("File dump options")), 
			wxDefaultPosition, 
			wxSize(wxDefaultSize.GetWidth(), 290)), 
	reportname(report_name)
{
	wxBoxSizer *main_sizer = new wxBoxSizer(wxVERTICAL);

	main_sizer->Add(new wxStaticText(this, 	wxID_ANY, _("Destination dir:")), 
			0, 
			wxALIGN_CENTER | wxALL, 
			10);

	dir_picker = new wxDirPickerCtrl(this, ID_DirChanged, wxEmptyString, _("Select a folder"));
	main_sizer->Add(dir_picker, 
			0, 
			wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 
			10);

	wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

	sizer->Add(new wxStaticText(this, wxID_ANY, _("Maximum file size (in MB):")), 
			0, 
			wxALIGN_CENTER | wxALL, 
			10);

	spinMB = new wxSpinCtrl(this, wxID_ANY, _T(""));
	sizer->Add(spinMB, 
			0, 
			wxALIGN_CENTER | wxALL, 
			10);

	main_sizer->Add(sizer, 0, wxALIGN_CENTER | wxALL, 10);

	enabled_cb = new wxCheckBox(this, wxID_ANY, _("Enable dump to a file"));
	main_sizer->Add(enabled_cb, 
			0, 
			wxALIGN_CENTER | wxALL, 
			10);

	destinantion_tb = new wxTextCtrl(this, wxID_ANY, _T(""), 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxTE_CENTRE | wxTE_READONLY);
	destinantion_tb->Disable();
	main_sizer->Add(destinantion_tb, 
			0, 
			wxEXPAND | wxALL, 
			10);

	sizer = new wxBoxSizer(wxHORIZONTAL);

	ok_btn = new wxButton(this, ID_OkBtn, _("Ok"));
	sizer->Add(ok_btn, 
			0, 
			wxALIGN_RIGHT | wxALL, 
			10);

	cancel_btn = new wxButton(this, ID_CancelBtn, _("Cancel"));
	sizer->Add(cancel_btn, 
			0, 
			wxALIGN_RIGHT | wxALL, 
			10);

	main_sizer->Add(sizer, 
			0, 
			wxALIGN_CENTER | wxALL, 
			10);

	SetSizer(main_sizer);

	LoadConfiguration();

	UpdateDestination();
}

void FileDumpDialog::OnOkBtn(wxCommandEvent& WXUNUSED(event))
{
	SaveConfiguration();
	Close();
}

void FileDumpDialog::OnCancelBtn(wxCommandEvent& WXUNUSED(event))
{
	Close();
}

void FileDumpDialog::OnClose(wxCloseEvent& event)
{
	Destroy();
}

void FileDumpDialog::LoadConfiguration()
{
	wxString path;
	if (!FileDump::GetDir().IsEmpty()) {
		dir_picker->SetPath(FileDump::GetDir());
	} else {
		dir_picker->SetPath(wxFileName::GetHomeDir());
	}
	enabled_cb->SetValue(FileDump::DumpEnabled());
	spinMB->SetValue(FileDump::GetMaxSize());
}

void FileDumpDialog::SaveConfiguration()
{
	wxConfig::Get()->Write(FILE_DUMP_DIR, dir_picker->GetPath());
	wxConfig::Get()->Write(FILE_DUMP_ENABLED, enabled_cb->GetValue());
	wxConfig::Get()->Write(FILE_DUMP_MAXSIZE, spinMB->GetValue());
}

void FileDumpDialog::OnUpdateDestination(wxFileDirPickerEvent& event)
{
	UpdateDestination();
}

void FileDumpDialog::UpdateDestination()
{
	assert(this->reportname);
	if (FileDump::GetFileName(dir_picker->GetPath(), *this->reportname).GetFullPath().IsEmpty()) {
		destinantion_tb->SetValue(_("No report name"));
	} else {
		destinantion_tb->SetValue(FileDump::GetFileName(
					dir_picker->GetPath(), *this->reportname).GetFullPath());
	}
}

void FileDump::WriteEOL(wxFile& file)
{
#ifdef MINGW32
	file.Write(_T("\r\n"), wxConvLocal);
#else
	file.Write(_T("\n"), wxConvLocal);
#endif
}

void FileDump::DumpValues(szParList *params, wxString report_name)
{
	if (!DumpEnabled()) {
		return;
	}

	bool writeheader = false;
	wxFileName filename = GetFileName(report_name);

	writeheader = !filename.FileExists();

	if (!writeheader && filename.GetSize() / (1024*1024) > GetMaxSize()) {
		return;
	}

	wxFile file(filename.GetFullPath(), wxFile::write_append);

	if (!file.IsOpened()) {
		return;
	}

	if (writeheader) {
		for (size_t i = 0; i < params->Count(); i++) {
			wxString shortcut = params->GetExtraProp(i, SZ_REPORTS_NS_URI, _T("short_name"));
			file.Write(shortcut, wxConvLocal);
			if (i < params->Count() - 1) {
				file.Write(_T(",\t"), wxConvLocal);
			}
		}
		WriteEOL(file);
		for (size_t i = 0; i < params->Count(); i++) {
			wxString unit = params->GetExtraProp(i, SZ_REPORTS_NS_URI, _T("unit"));
			file.Write(unit, wxConvLocal);
			if (i < params->Count() - 1) {
				file.Write(_T(",\t"), wxConvLocal);
			}
		}
		WriteEOL(file);
	}

	for (size_t i = 0; i < params->Count(); i++) {
		wxString val = params->GetExtraProp(i, SZ_REPORTS_NS_URI, _T("value"));
		file.Write(val, wxConvLocal);
		file.Write(_T(",\t"), wxConvLocal);
	}
	file.Write(wxDateTime::Now().FormatISODate(), wxConvLocal);
	file.Write(_T(" "), wxConvLocal);
	file.Write(wxDateTime::Now().FormatISOTime(), wxConvLocal);
	WriteEOL(file);
}

bool FileDump::DumpEnabled()
{
	bool enabled = false;
	wxConfig::Get()->Read(FILE_DUMP_ENABLED, &enabled, false);
	return enabled;
}

wxFileName FileDump::GetFileName(wxString report_name)
{
	wxString dir = GetDir();
	return wxFileName(dir + wxString(wxFileName::GetPathSeparator()) + report_name + _T(".csv"));
}

wxFileName FileDump::GetFileName(wxString dir, wxString report_name)
{
	if (report_name.IsEmpty())
		return wxFileName();
	return wxFileName(dir + wxString(wxFileName::GetPathSeparator()) + report_name + _T(".csv"));
}

wxString FileDump::GetDir()
{
	wxString temp;
	wxConfig::Get()->Read(FILE_DUMP_DIR, &temp);
	return temp;
}

long FileDump::GetMaxSize()
{
	long temp;
	if (!wxConfig::Get()->Read(FILE_DUMP_MAXSIZE, &temp))
		temp = 10;
	return temp;
}

