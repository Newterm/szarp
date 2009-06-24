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
/**
 * Obs³uga zmiennej wielko¶co fontu.
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 * $Id$
 */

#include "fonts.h"

#include <wx/config.h>
#include <wx/fontdlg.h>

/* bug with non-virtual wxFont::SetNativeFontInfo */
#if wxCHECK_VERSION(2, 5, 4)
#else
#include <wx/fontutil.h>
#endif

#define FONT_KEYGROUP _T("Font/")
#define DEFFONT_KEYNAME	_T("Default")

szFontProvider* szFontProvider::m_defaultObject = NULL;

szFontDialogBtn::szFontDialogBtn(szFontProvider* fontProvider, 
		wxString fontName, wxWindow* parent, wxWindowID id, 
		const wxString& label, wxPoint pos, wxSize size, long style):                
	wxButton(parent, id, label, pos, size, style), m_fontProvider(fontProvider),
	m_fontName(fontName)
{ 
}

void szFontDialogBtn::OnConfigureFont(wxCommandEvent& evt)
{
	m_fontProvider->ConfigureFont(m_fontName);
}

void szFontDialogBtn::OnResetFont(wxCommandEvent& evt)
{
	m_fontProvider->ResetFont(m_fontName);
}

szFontProvider::szFontProvider()
{
	// add something to Fonts group because of bug in
	// wxFileConfig::DeleteEntry 
	if (wxConfig::Get()->GetNumberOfEntries() <= 0) {
		wxConfig::Get()->Write(_T("placeholder"), 
				_T("workaround for wxWindgets bug - don't remove"));
	}
}

szFontProvider::~szFontProvider()
{
	m_fontHash.clear();
	m_defFontHash.clear();
}

szFontProvider* szFontProvider::Get()
{
	if (m_defaultObject == NULL)
		m_defaultObject = new szFontProvider();
	return m_defaultObject;
}

void szFontProvider::Cleanup()
{
	delete m_defaultObject;
	m_defaultObject = NULL;
}

wxFont szFontProvider::GetFont(wxString name)
{
	/* check if font found */
	if (m_fontHash.find(name) != m_fontHash.end()) {
		return m_fontHash[name];
	}
		
	/* check for font in config file */
	wxString key = FONT_KEYGROUP;
	if (name == wxEmptyString)
		key += DEFFONT_KEYNAME;
	else
		key += name;
	if (wxConfig::Get()->HasEntry(key)) {
		wxFont font;
#if wxCHECK_VERSION(2, 5, 4)
		m_fontHash[name].SetNativeFontInfo(wxConfig::Get()->Read(key, wxEmptyString));
#else
		wxNativeFontInfo info;
		info.FromString(wxConfig::Get()->Read(key, wxEmptyString));
		m_fontHash[name].SetNativeFontInfo(info);
#endif
		return m_fontHash[name];
	}
	
	/* check default font */
	if (m_defFontHash.find(name) != m_defFontHash.end()) 
		return m_defFontHash[name];

	/* return default font */
	m_fontHash[name] = wxSystemSettings::GetFont(wxSYS_SYSTEM_FONT);
	//return wxSystemSettings::GetFont(wxSYS_SYSTEM_FONT);
	return m_fontHash[name];
}

void szFontProvider::SetFont(wxFont font, wxString name)
{
	m_fontHash.erase(name);
	m_fontHash[name] = font;
	
	if (name == wxEmptyString)
		name = DEFFONT_KEYNAME;
	wxString key = FONT_KEYGROUP;
	key += name;
	wxConfig::Get()->Write(key, font.GetNativeFontInfoDesc());
	wxConfig::Get()->Flush();
}

void szFontProvider::SetDefaultFont(wxFont font, wxString name)
{
	m_defFontHash.erase(name);
	m_defFontHash[name] = font;
}

void szFontProvider::ResetFont(wxString name)
{
	m_fontHash.erase(name);
	if (name == wxEmptyString)
		name = DEFFONT_KEYNAME;
	wxString key = FONT_KEYGROUP;
	key += name;
	wxConfig::Get()->DeleteEntry(key, TRUE);
	wxConfig::Get()->Flush();
}

void szFontProvider::SetWidgetFont(wxWindow* widget, wxString name)
{
	widget->SetFont(GetFont(name));
}

void szFontProvider::ShowFontsDialog(wxWindow *parent, 
		wxString name1,
		wxString descr1,
		wxString name2, 
		wxString descr2,
		wxString name3, 
		wxString descr3,
		wxString name4, 
		wxString descr4,
		wxString name5, 
		wxString descr5)
{
	wxArrayString fonts;
	if (descr1.IsEmpty())
		descr1 = _("Default font");
	fonts.Add(name1);
	fonts.Add(descr1);
	if (descr2.IsEmpty())
		goto end;
	fonts.Add(name2);
	fonts.Add(descr2);
	if (descr3.IsEmpty())
		goto end;
	fonts.Add(name3);
	fonts.Add(descr3);
	if (descr4.IsEmpty())
		goto end;
	fonts.Add(name4);
	fonts.Add(descr4);
	if (descr5.IsEmpty())
		goto end;
	fonts.Add(name5);
	fonts.Add(descr5);
end:
	ShowFontsDialog(parent, fonts);
}

void szFontProvider::ShowFontsDialog(wxWindow *parent, wxArrayString fonts)
{
	wxDialog dlg(parent, wxID_ANY, wxString(_("Font configuration")));
	m_dlg = &dlg;
	wxStaticText* warnText = new wxStaticText(&dlg, -1,
			_("You have to restart program to apply\nnew fonts settings."));
	szSetDefFont(warnText);
	wxButton* clBut = new wxButton(&dlg, wxID_CANCEL, _("Close"));
	szSetDefFont(clBut);

	wxBoxSizer* sizer0 = new wxBoxSizer(wxVERTICAL);

	int width = 0;
	int height = 0;
	wxClientDC dc(&dlg);
	for (size_t i = 0; i < fonts.Count() / 2; i++) {
		int w, h;
		dc.SetFont(GetFont(fonts[i*2]));
		dc.GetTextExtent(fonts[i*2+1], &w, &h);
		width = wxMax(width, w);
		height = wxMax(height, h);
	}
	int w, h, w2, h2;
	dc.SetFont(GetFont(wxEmptyString));
	dc.GetTextExtent(_("Configure"), &w, &h);
	dc.GetTextExtent(_("Reset to default"), &w2, &h2);
	for (size_t i = 0; i < fonts.Count() / 2; i++) {
		wxString name = fonts[i*2];
		wxString descr = fonts[i*2+1];
		wxBoxSizer* sizer1 = new wxBoxSizer(wxHORIZONTAL);
		wxStaticText* txt = new wxStaticText(&dlg, -1, descr, wxDefaultPosition,
				wxSize(width, height), 
				wxSUNKEN_BORDER | wxALIGN_CENTRE |
				wxST_NO_AUTORESIZE);
		txt->SetFont(GetFont(fonts[i*2]));
		//szSetFont(txt, name);
		sizer1->Add(txt, 0, wxALL, 10);
		wxButton* but = new szFontDialogBtn(this, name, &dlg, -1, _("Configure"),
				wxDefaultPosition, wxSize(w + 8, h + 8));
		but->Connect(but->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, (wxObjectEventFunction) 
				(wxEventFunction) (wxCommandEventFunction)&szFontDialogBtn::OnConfigureFont);
		szSetDefFont(but);
		sizer1->Add(but, 1, wxALL, 10);
		wxButton* but2 = new szFontDialogBtn(this, name, &dlg, -1, _("Reset to default"),
				wxDefaultPosition, wxSize(w2 + 8, h + 8));
		but2->Connect(but2->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, (wxObjectEventFunction) 
				(wxEventFunction) (wxCommandEventFunction)&szFontDialogBtn::OnResetFont);
		szSetDefFont(but2);
		sizer1->Add(but2, 0, wxALL, 10);
		sizer0->Add(sizer1, 0);
	}

	sizer0->Add(warnText, 0, wxALL | wxEXPAND, 10);
	sizer0->Add(clBut, 0, wxALL | wxCENTER, 10);
	dlg.SetSizer(sizer0);
	sizer0->SetSizeHints(&dlg);
	dlg.ShowModal();
}

void szFontProvider::ConfigureFont(wxString fontName)
{
	wxFont font = GetFont(fontName);
	font = wxGetFontFromUser(m_dlg, font);
	if (font.Ok()) {
		SetFont(font, fontName);
	}
}


/*

BEGIN_EVENT_TABLE(DateChooserWidget, wxDialog)
	EVT_BUTTON(wxOK, DateChooserWidget::onOk)
	EVT_BUTTON(wxCANCEL, DateChooserWidget::onCancel)
	EVT_SPINCTRL(ID_SpinCtrlHour, DateChooserWidget::onHourChange)
	EVT_SPINCTRL(ID_SpinCtrlMinute, DateChooserWidget::onMinuteChange)
END_EVENT_TABLE()
*/
