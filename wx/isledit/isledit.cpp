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
 
  Pawe³ Pa³ucha pawel@praterm.com.pl
  $Id$

  ISL Editor - Inkscape plugin for editing isl namespace properties in ISL schemas.

*/

#include "cconv.h"
#include "isledit.h"
#include "szapp.h"
#include "szarp_config.h"
#include "szbase/szbname.h"
#include "xmlutils.h"
#include <wx/cmdline.h>
#include <libxml/xpath.h>

#ifdef MINGW32
#include <wx/filename.h>
#endif

#define szID_OK		wxID_HIGHEST
#define szID_CANCEL	wxID_HIGHEST+1
#define szID_RADIO	wxID_HIGHEST+2
#define szID_HELP	wxID_HIGHEST+3
#define szID_CHOOSE	wxID_HIGHEST+4
#define szID_CHECK	wxID_HIGHEST+5

#define ISL_URI "http://www.praterm.com.pl/ISL/params"

/**
  Parameter filter - selects parameters that can be read from paramd.
 */
int par_filter(TParam * p)
{
	if ((p->GetType() == TParam::P_REAL)
	    || (p->GetType() == TParam::P_COMBINED))
		return 0;
	return 1;
}

ISLFrame::ISLFrame(wxWindow * parent, int id, const wxString & title, TSzarpConfig * ipk, xmlNodePtr _element, const wxPoint & pos, const wxSize & size, long style):
wxFrame(parent, id, title, pos, size,
	wxCAPTION | wxCLOSE_BOX | wxTAB_TRAVERSAL), element(_element)
{
#ifdef MINGW32
       	SetBackgroundColour(wxColour(200,200,200));
#endif
	parameter_txtctrl =
	    new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
			   wxDefaultSize, wxTE_READONLY | wxTAB_TRAVERSAL);
	parameter_bt = new wxButton(this, szID_CHOOSE, _("Choose parameter"));
	const wxString content_rb_choices[] = {
		_("Replace element text content"),
		_("Replace element attribute")
	};
	content_rb =
	    new wxRadioBox(this, szID_RADIO, _("Content disposition"),
			   wxDefaultPosition, wxDefaultSize, 2,
			   content_rb_choices, 2,
			   wxRA_SPECIFY_ROWS | wxTAB_TRAVERSAL);
	v_u_cb = new wxCheckBox(this, szID_CHECK, _("Show parameter unit"));
	attribute_lb = new wxStaticText(this, wxID_ANY, _("Target attribute"));
	const wxString combo_box_1_choices[] = {

	};
	combo_box_1 =
	    new wxComboBox(this, wxID_ANY, wxT(""), wxDefaultPosition,
			   wxDefaultSize, 0, combo_box_1_choices,
			   wxCB_DROPDOWN | wxCB_READONLY | wxCB_SORT |
			   wxTAB_TRAVERSAL);
	scale_lb = new wxStaticText(this, wxID_ANY, _("Scale by"));
	scale_ctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	shift_lb = new wxStaticText(this, wxID_ANY, _("Shift by"));
	shift_ctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	ok_bt = new wxButton(this, szID_OK, _("Ok"));
	cancel_bt = new wxButton(this, szID_CANCEL, _("Cancel"));
	help_bt = new wxButton(this, szID_HELP, _("Help"));

	set_properties();
	do_layout();

	parsel_wdg = new szParSelect(ipk, NULL, -1, _("Select parameter"),
				     FALSE /* addStyle */ ,
				     FALSE /* multiple */ ,
				     TRUE /* show short */ ,
				     TRUE /* show draw */ ,
				     FALSE /* _single */ ,
				     par_filter);

	assert(element->type == XML_ELEMENT_NODE);
	xmlChar *prop =
	    xmlGetNsProp(element, (xmlChar *) "uri", (xmlChar *) ISL_URI);
	if (prop) {
		wxString parameter = wxString(SC::U2S(prop));
		xmlFree(prop);
		if (parameter.AfterFirst('@') == _T("v_u")) {
			v_u_cb->SetValue(TRUE);
		} else {
			parameter = parameter.BeforeFirst('@') + _T("@value");
		}
		parameter_txtctrl->SetValue(parameter);
	}

	for (xmlAttrPtr i = element->properties; i; i = i->next) {
		if (i->ns || !strcmp((const char *)i->name, "id"))
			continue;
		combo_box_1->Append(wxString(SC::U2S(i->name)));
	}

	prop = xmlGetNsProp(element, (xmlChar *) "target", (xmlChar *) ISL_URI);
	if (prop) {
		int found = combo_box_1->FindString(wxString(SC::U2S(prop)));
		if (found != wxNOT_FOUND) {
			combo_box_1->SetSelection(found);
		} else if (combo_box_1->GetCount() > 0) {
			combo_box_1->SetSelection(0);
		}
		if (!v_u_cb->IsChecked()) {
			content_rb->SetSelection(1);
		}
	}
	wxString dec = wxString::Format(_T("%.1f"), 0.5).Mid(1, 1);
	prop = xmlGetNsProp(element, (xmlChar *) "scale", (xmlChar *) ISL_URI);
	if (prop) {
		wxString scale = wxString(SC::U2S(prop));
		scale.Replace(_T("."), dec);
		double val;
		if (scale.ToDouble(&val)) {
			scale_ctrl->ChangeValue(scale);
		}
	}
	prop = xmlGetNsProp(element, (xmlChar *) "shift", (xmlChar *) ISL_URI);
	if (prop) {
		wxString shift = wxString(SC::U2S(prop));
		double val;
		if (shift.ToDouble(&val)) {
			shift_ctrl->ChangeValue(shift);
		}
	}

	content_rb_selection = content_rb->GetSelection();
	do_disable();
}

BEGIN_EVENT_TABLE(ISLFrame, wxFrame)
    EVT_BUTTON(szID_CHOOSE, ISLFrame::doChoose)
    EVT_RADIOBOX(szID_RADIO, ISLFrame::doRadio)
    EVT_CHECKBOX(szID_CHECK, ISLFrame::doCheck)
    EVT_BUTTON(szID_OK, ISLFrame::doOk)
    EVT_BUTTON(szID_CANCEL, ISLFrame::doCancel)
    EVT_BUTTON(szID_HELP, ISLFrame::doHelp)
    EVT_CLOSE(ISLFrame::doExit)
    END_EVENT_TABLE();

void ISLFrame::doChoose(wxCommandEvent & event)
{
	event.Skip();
	if (parsel_wdg->ShowModal() != wxID_OK) {
		return;
	}
	if (parsel_wdg->g_data.m_param) {
		wxString name = wxString(parsel_wdg->g_data.m_param->GetName());
		if (name.IsEmpty()) return;
		name = wxString(_T("http://localhost:8081/")) + wxString(wchar2szb(std::wstring(name)));
		if (v_u_cb->IsChecked()) {
			name += _T("@v_u");
		} else {
			name += _T("@value");
		}

		parameter_txtctrl->SetValue(name);
	}
}

void ISLFrame::doRadio(wxCommandEvent & event)
{
	content_rb_selection = event.GetSelection();
	do_disable();
}

void ISLFrame::doCheck(wxCommandEvent & event)
{
	wxString parameter = parameter_txtctrl->GetValue().BeforeFirst('@');
	if (event.IsChecked()) {
		parameter_txtctrl->SetValue(parameter + _T("@v_u"));
	} else {
		parameter_txtctrl->SetValue(parameter + _T("@value"));
	}
	do_disable();
}

void ISLFrame::doOk(wxCommandEvent & event)
{
	event.Skip();
	if (parameter_txtctrl->GetValue().length() > 0) {
		xmlNsPtr ns =
		    xmlSearchNsByHref(element->doc, element,
				      (xmlChar *) ISL_URI);
		if (!ns) {
			xmlNodePtr n = element->doc->children;
			while (n && (n->type != XML_ELEMENT_NODE)) {
				n = n->next;
			}
			assert(n != NULL);
			ns = xmlNewNs(n, (xmlChar *) ISL_URI,
				      (xmlChar *) "isl");
			if (!ns) {
				fprintf(stderr, "%s\n",	SC::S2A(wxString(_("Error in XML document: ISL namespace is incorrectly defined"))).c_str());

				exit(1);
			}
		}
		std::wstring uri = std::wstring(parameter_txtctrl->GetValue());
		//uri = wchar2szb(uri);
		xmlSetNsProp(element, ns, (xmlChar *) "uri", SC::S2U(uri).c_str());
		if (content_rb->GetSelection() == 1) {
			xmlSetNsProp(element, ns, (xmlChar *) "target",
				     (SC::S2U(combo_box_1->GetValue())).
				     c_str());
			xmlSetNsProp(element, ns, (xmlChar *) "scale",
				     (SC::S2U(scale_ctrl->GetValue())).c_str());
			xmlSetNsProp(element, ns, (xmlChar *) "shift",
				     (SC::S2U(shift_ctrl->GetValue())).c_str());

		} else {
			xmlUnsetNsProp(element, ns, (xmlChar *) "target");
		}
	}
	xmlDocDump(stdout, element->doc);
	exit(0);
}

void ISLFrame::doCancel(wxCommandEvent & event)
{
	event.Skip();
	fprintf(stderr, "%s\n",
		SC::S2A(wxString(_("Operation canceled by user"))).c_str());
	exit(1);
}

void ISLFrame::doExit(wxCloseEvent & event)
{
	event.Skip();
	fprintf(stderr, "%s\n",
		SC::S2A(wxString(_("Operation canceled by user"))).c_str());
	exit(1);
}

void ISLFrame::doHelp(wxCommandEvent & event)
{
	event.Skip();
	printf("%s\n", "Event handler (ISLFrame::doHelp) not implemented yet");
}

void ISLFrame::set_properties()
{
	SetSizeHints(470, 280);
	parameter_txtctrl->SetMinSize(wxSize(300, 23));
	content_rb->SetSelection(0);
	ok_bt->SetFocus();
	ok_bt->SetDefault();
}

void ISLFrame::do_layout()
{
	wxBoxSizer *sizer_5 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *sizer_8 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *sizer_9 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *sizer_7 = new wxBoxSizer(wxHORIZONTAL);
	wxGridSizer *grid_sizer_1 = new wxGridSizer(3, 2, 0, 0);
	wxBoxSizer *sizer_6 = new wxBoxSizer(wxHORIZONTAL);
	sizer_5->Add(parameter_txtctrl, 0, wxALL | wxEXPAND, 10);
	sizer_6->Add(v_u_cb, 1, wxLEFT | wxRIGHT | wxBOTTOM, 10);
	sizer_6->Add(parameter_bt, 0,
		     wxRIGHT | wxBOTTOM | wxADJUST_MINSIZE | wxALIGN_RIGHT, 10);
	sizer_5->Add(sizer_6, 0, wxEXPAND, 0);
	sizer_5->Add(content_rb, 0,
		     wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND | wxADJUST_MINSIZE,
		     10);
	grid_sizer_1->Add(attribute_lb, 0,
			  wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL |
			  wxADJUST_MINSIZE, 0);
	grid_sizer_1->Add(combo_box_1, 0, wxALL | wxEXPAND | wxADJUST_MINSIZE,
			  0);
	grid_sizer_1->Add(scale_lb, 0,
			  wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL |
			  wxADJUST_MINSIZE, 0);
	grid_sizer_1->Add(scale_ctrl, 0, wxADJUST_MINSIZE, 0);
	grid_sizer_1->Add(shift_lb, 0,
			  wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL |
			  wxADJUST_MINSIZE, 0);
	grid_sizer_1->Add(shift_ctrl, 0, wxADJUST_MINSIZE, 0);
	sizer_7->Add(grid_sizer_1, 1, wxLEFT | wxRIGHT | wxEXPAND, 10);
	sizer_5->Add(sizer_7, 1, wxEXPAND, 0);
	sizer_9->Add(ok_bt, 1,
		     wxLEFT | wxRIGHT | wxALIGN_CENTER_HORIZONTAL |
		     wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 10);
	sizer_9->Add(cancel_bt, 1,
		     wxLEFT | wxRIGHT | wxALIGN_CENTER_HORIZONTAL |
		     wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 10);
	sizer_9->Add(help_bt, 1,
		     wxLEFT | wxRIGHT | wxALIGN_CENTER_HORIZONTAL |
		     wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 10);
	sizer_8->Add(sizer_9, 1, wxALL | wxEXPAND, 10);
	sizer_5->Add(sizer_8, 1, wxEXPAND, 0);
	SetSizer(sizer_5);
	sizer_5->Fit(this);
	Layout();
}

void ISLFrame::do_disable()
{
	if (v_u_cb->IsChecked()) {
		content_rb_selection = content_rb->GetSelection();
		content_rb->SetSelection(0);
		content_rb->Disable();
		combo_box_1->Disable();
		scale_ctrl->Disable();
		shift_ctrl->Disable();
		attribute_lb->Disable();
		scale_lb->Disable();
		shift_lb->Disable();
	} else {
		content_rb->Enable();
		content_rb->SetSelection(content_rb_selection);
		if (content_rb_selection == 0) {
			combo_box_1->Disable();
			scale_ctrl->Disable();
			shift_ctrl->Disable();
			attribute_lb->Disable();
			scale_lb->Disable();
			shift_lb->Disable();
		} else {
			combo_box_1->Enable();
			scale_ctrl->Enable();
			shift_ctrl->Enable();
			attribute_lb->Enable();
			scale_lb->Enable();
			shift_lb->Enable();
		}
	}
}

class ISLEditor:public szApp {
 private:
	wxLocale locale;	/**< locale object */
	wxString m_config;	/**< path to file with SZARP configuration */
	wxString m_input;	/**< path to file to edit */
	wxString m_id;		/**< id of element to edit */
 protected:
	virtual bool OnCmdLineError(wxCmdLineParser & parser) {
		return true;
	} virtual bool OnCmdLineHelp(wxCmdLineParser & parser) {
		parser.Usage();
		return false;
	}

	virtual bool OnCmdLineParsed(wxCmdLineParser & parser) {
		if (parser.GetParamCount() != 1) {
			parser.Usage();
			return false;
		}
		m_input = parser.GetParam(0);
#ifdef MINGW32
		/* on Windows, Inkscape calls plugin with DOS-like path format, and libxml2
		 * does not like this... */
		wxFilePath path = m_input;
		if (!path.IsOk() or !path.FileExists()) {
			fprintf(stderr, "%s\n",
					SC::S2A(wxString(_("Invalid path to SVG file."))).c_str());
			return false;
		}
		m_input = path.GetLongPath();
#endif

		parser.Found(_T("f"), &m_config);
		if (!parser.Found(_T("i"), &m_id)) {
			fprintf(stderr, "%s\n",
				SC::S2A(wxString(_("You must select some element"))).c_str());
			return false;
		}
		return true;
	}

	virtual void OnInitCmdLine(wxCmdLineParser & parser) {
		szApp::OnInitCmdLine(parser);
		parser.SetLogo(_("ISL Editor - plugin for Inkscape"));
		parser.AddSwitch(_T("h"), _T("help"), _("print usage info"),
				 wxCMD_LINE_OPTION_HELP);
		parser.AddOption(_T("f"), _T("file"), _("IPK file to use"),
				 wxCMD_LINE_VAL_STRING,
				 wxCMD_LINE_OPTION_MANDATORY);
		parser.AddOption(_T("i"), _T("id"), _("id of element to edit"));
		parser.AddParam(_T("config"));
	}
	virtual bool OnInit();
};

IMPLEMENT_APP(ISLEditor)

bool ISLEditor::OnInit()
{
	if (szApp::OnInit() == false) {
		return false;
	}
	SetAppName(_T("ISL Editor"));

	wxLog *logger = new wxLogStderr();
	wxLog::SetActiveTarget(logger);

#if wxUSE_LIBPNG
	wxImage::AddHandler(new wxPNGHandler());
#endif  //wxUSE_LIBPNG

	//SET LOCALE
	wxArrayString catalogs;
	this->InitializeLocale(_T("isledit"), locale);

	TSzarpConfig *ipk;
	ipk = new TSzarpConfig();
	if (ipk->loadXML(std::wstring(m_config))) {
		fprintf(stderr, "%s",
			SC::S2A(wxString(_("Error loading configration file "))).c_str());
		fprintf(stderr, " %s\n", SC::S2A(m_config).c_str());
		exit(1);
	}

	xmlDocPtr input = xmlParseFile(SC::S2A(m_input).c_str());
	if (!input) {
		fprintf(stderr, "%s\n",
			SC::S2A(wxString(_("Error parsing input XML file"))).c_str());
		exit(1);
	}

	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(input);
	wxString id = _T("//*[@id='") + m_id + _T("']");

	xmlNodePtr element =
	    uxmlXPathGetNode((xmlChar *) SC::S2A(id).c_str(), xpath_ctx);
	if (!element) {
		fprintf(stderr, "%s\n",
			SC::S2A(wxString(_("Selected element not found in XML file"))).
			c_str());
		exit(1);
	}
	/* inkscape adds tspan elements inside text elements */
	if (element->children && (element->children->type == XML_ELEMENT_NODE)) {
		element = element->children;
	}
	wxString title =
	    wxString(_("ISL Editor: editing <")) +
	    wxString(SC::U2S(element->name)) + _T(">, id ") + m_id;

	wxInitAllImageHandlers();
	ISLFrame *isl_frame = new ISLFrame(NULL, wxID_ANY, title, ipk, element);
	SetTopWindow(isl_frame);
	isl_frame->Show();
	return true;
}

