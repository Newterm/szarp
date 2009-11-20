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
 * confedit - WX widgets for SZARP configuration edition
 * SZARP, 2002 Pawe³ Pa³ucha
 *
 * $Id$
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "confapp.h"
#include "cframe.h"
#include "szarp_config.h"
#include "htmlview.h"
#include "cconv.h"

#include <sstream>
#include <libxml/valid.h>
#include <wx/listimpl.cpp>

#if defined(__WXGTK__) || defined(__WXMOTIF__)
#include "up.xpm"
#include "down.xpm"
#include "load.xpm"
#include "reload.xpm"
#include "save.xpm"
#include "greenexit.xpm"
#include "help.xpm"
#endif

#define xmlSetRemoveProp(a, b, c) \
	if (((c) == NULL)) \
		xmlRemoveProp(xmlHasProp(a, b)); \
	else \
		xmlSetProp(a, b, c);

template<typename T, typename F> double convert_to(const F& f) {
	T ret;

	std::wstringstream ss;
	ss.imbue(std::locale("C"));
	ss << f; 
	ss >> ret;

	return ret;
}

enum {
	ID_Open = wxID_HIGHEST,
        ID_tbOpen,
        ID_Reload,
        ID_tbReload,
        ID_Save,
        ID_tbSave,
        ID_SaveAs,
        ID_Exit,
        ID_tbExit,
        ID_tbHelp,
        ID_tbUp,
        ID_tbDown,
        ID_RapList,
        ID_DrawList,
	ID_DrawItems,
	ID_MenuDown,
	ID_MenuUp,
	ID_Clear,
	ID_Help,
	ID_About
};

BEGIN_EVENT_TABLE(ConfFrame, szFrame)
	EVT_CLOSE(ConfFrame::OnClose)
        EVT_MENU(ID_Exit, ConfFrame::OnExit)
        EVT_TOOL(ID_tbExit, ConfFrame::OnExit)
        EVT_MENU(ID_Open, ConfFrame::OnOpen)
        EVT_TOOL(ID_tbOpen, ConfFrame::OnOpen)
        EVT_MENU(ID_Reload, ConfFrame::OnReload)
        EVT_TOOL(ID_tbReload, ConfFrame::OnReload)
        EVT_MENU(ID_Save, ConfFrame::OnSave)
        EVT_MENU(ID_tbSave, ConfFrame::OnSave)
        EVT_MENU(ID_SaveAs, ConfFrame::OnSaveAs)
        EVT_MENU(ID_tbHelp, ConfFrame::OnHelp)
        EVT_TOOL(ID_tbUp, ConfFrame::UpPressed)
        EVT_TOOL(ID_MenuUp, ConfFrame::UpPressed)
        EVT_MENU(ID_tbDown, ConfFrame::DownPressed)
        EVT_MENU(ID_MenuDown, ConfFrame::DownPressed)
	EVT_MENU(ID_Clear, ConfFrame::OnClear)
	EVT_MENU(ID_Help, ConfFrame::OnHelp)
	EVT_MENU(ID_About, ConfFrame::OnAbout)
        EVT_LISTBOX(ID_RapList, ConfFrame::RaportSelected)
END_EVENT_TABLE()

WX_DEFINE_LIST(xmlNodeList);

ConfFrame::ConfFrame(wxString _filename, const wxPoint& pos, const wxSize& size) 
	: szFrame((wxFrame *)NULL, -1, _("SZARP config editor"), 
		pos, size), filename(_filename), params(NULL)
{
	modified = FALSE;
        /* Minimal program window size */
	SetSizeHints(300, 200);

        /* Load IPK DTD */
        if (LoadSchema() != 0)
                Close();
        /* Menu */
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_Open, _("&Open\tCtrl-O"),
		_("Load configuration from file."));
	menuFile->Append(ID_Open, _("&Reload\tCtrl-R"),
		_("Reload last opened file."));
	menuFile->Append(ID_Save, _("&Save\tCtrl-S"),
		_("Save changes to file."));	
	menuFile->Append(ID_SaveAs, _("Save &As\tCtrl-A"),
		_("Save configuration with different file name."));
	menuFile->AppendSeparator();
	menuFile->Append(ID_Exit, _("E&xit\tAlt-X"),
		_("Exit from program."));

	wxMenu *menuEdit = new wxMenu;
	menuEdit->Append(ID_MenuUp, _("&Up\tCtrl-U"),
		_("Move selected object up."));
	menuEdit->Append(ID_MenuDown, _("&Down\tCtrl-D"),
		_("Move selected object down."));
	menuEdit->AppendSeparator();
	menuEdit->Append(ID_Clear, _("&Clear all"),
		_("Clears all the order attributes."));
	
	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(ID_Help, _("&Help\tF1"), _("Open program documentation"));
	menuHelp->Append(ID_About, _("&About"), _("Info about program and authors"));
	
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, _("&File"));
	menuBar->Append(menuEdit, _("&Edit"));
	menuBar->Append(menuHelp, _("&Help"));

	SetMenuBar(menuBar);

        /* Status bar */
	CreateStatusBar();
        /* Tool bar */
	CreateToolBar();
	wxToolBar* toolBar = GetToolBar();
	toolBar->AddTool(ID_tbOpen, wxICON(load), _("Load file"), 
		_("Load configuration from disk file"));
	toolBar->AddTool(ID_tbReload, wxICON(reload), _("Reload file"), 
		_("Reload last opened file"));
	toolBar->AddTool(ID_tbSave, wxICON(save), _("Save file"), 
		_("Save configuration to file"));
	toolBar->AddSeparator();
	toolBar->AddTool(ID_tbUp, wxICON(up), _("Move up"), 
		_("Move object up in the list"));
	toolBar->AddTool(ID_tbDown, wxICON(down), _("Move down "), 
		_("Move object down on the list"));
	toolBar->AddSeparator();
	toolBar->AddTool(ID_tbExit, wxICON(greenexit), _("Exit"), 
		_("Exit from program"));
	toolBar->AddSeparator();
	toolBar->AddTool(ID_tbHelp, wxICON(help), _("Help"), 
		_("Display program help"));
        SetToolBar(toolBar);

        /* Widgets layout */
        wxBoxSizer* top_sizer = new wxBoxSizer(wxVERTICAL);

        notebook = new wxNotebook(this, -1);
        
        wxPanel* raptab = new wxPanel(notebook, -1);
        notebook->AddPage(raptab, _("Raports"));
        wxBoxSizer *raptab_sizer = new wxBoxSizer(wxHORIZONTAL);
        
        raplist = new wxListBox(raptab, ID_RapList, wxDefaultPosition,
                        wxSize(150, 300));
        raptab_sizer->Add(raplist, 2, wxEXPAND | wxALL, 10);

        ritemslist = new wxListBox(raptab, -1, wxDefaultPosition,
                        wxSize(150, 300));
        raptab_sizer->Add(ritemslist, 3, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT,
                        10);
        
        raptab->SetAutoLayout(TRUE);
        raptab->SetSizer(raptab_sizer);

        wxPanel* drawtab = new wxPanel(notebook, -1);
        notebook->AddPage(drawtab, _("Draws"));
        wxBoxSizer *drawtab_sizer = new wxBoxSizer(wxHORIZONTAL);

        drawlist = new wxListBox(drawtab, ID_DrawList, wxDefaultPosition,
                        wxSize(150, 300));
        drawtab_sizer->Add(drawlist, 1, wxEXPAND | wxALL, 10);

        ditemslist = new DrawsListCtrl(drawtab, ID_DrawItems, wxDefaultPosition,
                        wxSize(300, 300));
        drawtab_sizer->Add(ditemslist, 1, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT,
                        10);
        drawtab->SetAutoLayout(TRUE);
        drawtab->SetSizer(drawtab_sizer);

        notebook->SetAutoLayout(TRUE);
        
        top_sizer->Add(notebook, 1, wxEXPAND | wxALL, 10);
        
        SetSizer(top_sizer);
        SetAutoLayout(TRUE);
        top_sizer->SetSizeHints(this);

        if (filename != wxEmptyString)
                LoadFile(filename);

        GetStatusBar()->SetStatusText(_("SZARP IPK editor"));

	Connect(ID_DrawList, 
			wxEVT_COMMAND_LISTBOX_SELECTED, 
			wxCommandEventHandler(ConfFrame::DrawSelected));
}

void ConfFrame::OnExit(wxCommandEvent& WXUNUSED(event))
{
	Close(TRUE);
}

void ConfFrame::OnHelp(wxCommandEvent& WXUNUSED(event))
{
	HtmlViewerFrame* f = new HtmlViewerFrame(
		wxString(SC::A2S(INSTALL_PREFIX"/resources/documentation/new/ipk/html/ipkedit.html")),
		this, _("IPK editor help"),
		wxDefaultPosition, wxSize(600,600));
	f->Show();
}

void ConfFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxGetApp().ShowAbout();
}

void ConfFrame::OnClose(wxCloseEvent& WXUNUSED(event))
{
	if (AskForSave())
		return;
	Destroy();
}

void ConfFrame::OnOpen(wxCommandEvent& WXUNUSED(event))
{
	if (AskForSave())
		return;
	wxFileDialog *dialog = new wxFileDialog(
                this,                           //parent windows
                _("Choose a file to load"),     // message
                _T(""),                             // default dir (current)
                _T(""),                             // no default file
                _("XML files (*.xml)|*.xml|All files|*.*"),
                                                // type of files
                wxOPEN |                        // open file dialog
                wxCHANGE_DIR);                  // change directory to last
        if (dialog->ShowModal() == wxID_CANCEL)
                return;
        LoadFile(dialog->GetPath());
}

void ConfFrame::OnReload(wxCommandEvent& WXUNUSED(event))
{
	if (params == NULL)
		return;
	if (AskForSave())
		return;
	LoadFile(filename);
}

void ConfFrame::LoadFile(wxString path)
{
        xmlDocPtr newdoc = xmlParseFile((const char*)path.mb_str());
        if (newdoc == NULL) {
                wxLogError(_("XML document not well-formed"));
                return ;
        }
       
	/* validate file */
	xmlRelaxNGValidCtxtPtr ctxt;
	int ret;
	
	ctxt = xmlRelaxNGNewValidCtxt(ipk_rng);
	ret = xmlRelaxNGValidateDoc(ctxt, newdoc);
	xmlRelaxNGFreeValidCtxt(ctxt);	
	
        if (ret < 0) {
		wxLogError(_("Failed to compile IPK schema!"));
		return;
	} else if (ret > 0) {
                wxLogError(_("XML document doesn't validate against IPK schema!"));
                return;
        }
        if (params != NULL)
                xmlFreeDoc(params);
        params = newdoc;
        ReloadParams();
	ResetAll();
	SetTitle(wxString(_("SZARP ")) + path);
	modified = FALSE;
	filename = path;
}

void ConfFrame::OnSave(wxCommandEvent& event)
{
	if (params)
		SaveFile(filename);
}

void ConfFrame::OnSaveAs(wxCommandEvent& event)
{
	if (!params)
		return;
	wxFileDialog *dialog = new wxFileDialog(
                this,                           //parent windows
                _("Save as"), 		    	// message
                _T(""),                             // default dir (current)
                _T(""),                             // no default file
                _("XML files (*.xml)|*.xml|All files|*.*"),
                                                // type of files
                wxSAVE |                        // save file dialog
                wxCHANGE_DIR);                  // change directory to last
        if (dialog->ShowModal() == wxID_CANCEL)
                return;
	filename = dialog->GetPath();
	if (SaveFile(filename) == 0)
		SetTitle(wxString(_("SZARP ")) + filename);
}

int ConfFrame::SaveFile(wxString path)
{
 	int ret = xmlSaveFormatFileEnc((const char*)path.mb_str(), params, "ISO-8859-2", 1);
 	if (ret < 0) {
		wxLogError(_("There was error saving file %s."), path.c_str());
	} else {
		modified = FALSE;
		GetStatusBar()->SetStatusText(_("File successfully saved."));
	}
	return (ret < 0);
}

int ConfFrame::AskForSave(void)
{
	if (!modified)
		return 0;
	int ret = wxMessageBox(_("File was modified. Save changes ?"),
			_("Save changes ?"), wxYES_NO | wxCANCEL | 
			wxCENTRE | wxICON_QUESTION);
	if (ret == wxCANCEL)
		return 1;
	if (ret == wxNO)
		return 0;
	return SaveFile(filename);
}

int ConfFrame::LoadSchema(void)
{
	xmlRelaxNGParserCtxtPtr ctxt;
	
	ctxt = xmlRelaxNGNewParserCtxt(
                "file:///"INSTALL_PREFIX"/resources/dtd/ipk-params.rng");
	ipk_rng = xmlRelaxNGParse(ctxt);
        if (ipk_rng == NULL) {
                wxLogError(_("Could not load IPK RelaxNG schema from file %s"),
                        INSTALL_PREFIX"/resources/dtd/ipk-params.rng");
                return 1;
        }
        return 0;
}

void ConfFrame::ReloadParams(void)
{
        while (raplist->GetCount() > 0) {
                delete (xmlNodeList *)raplist->GetClientData(0);
		raplist->SetClientData(0, NULL);
                raplist->Delete(0);
        }
        ritemslist->Clear();
        while (drawlist->GetCount() > 0) {
                delete (xmlNodeList *)drawlist->GetClientData(0);
		drawlist->SetClientData(0, NULL);
                drawlist->Delete(0);
        }
        ditemslist->DeleteAllItems();
      
        FindElements(params->children);
}


void ConfFrame::AddRaport(xmlNodePtr node)
{
        /* get raport title */
        wxString raport = wxString(SC::U2S(xmlGetProp(node, (xmlChar*)"title")));
        /* check if raport laready exists */
        int pos = raplist->FindString(raport);
        xmlNodeList* list;
        if (pos < 0) {
                /* new raport - just add it */
                list = new xmlNodeList();
                list->Append(node);
                raplist->Append(raport, (void *)list);
                return;
        }
        /* get list of raport items */
        list = (xmlNodeList*)raplist->GetClientData(pos);
        /* get order of inserted element */
        char* str = (char*)xmlGetProp(node, (xmlChar*)"order");
        if (str == NULL) {
                list->Append(node);
                return;
        }
        double order = atof(str);
        /* search for position to insert current element */
        xmlNodeList::Node* tmp;
        for (tmp = list->GetFirst(); tmp; tmp = tmp->GetNext())
        {
                xmlNodePtr n = tmp->GetData();
                str = (char *)xmlGetProp(n, (xmlChar*)"order");
                if ((str == NULL) || (atof(str) > order)) {
                        list->Insert(tmp, node);
                        break;
                }
        }
        /* append element at end */
        if (tmp == NULL)
                list->Append(node);
}

#define prior_greater(a, b) \
        (((a * b) > 0) ? (a < b) : (a > 0))

void ConfFrame::AddDraw(xmlNodePtr node)
{
        /* get window title */
        wxString title = wxString(SC::U2S(xmlGetProp(node, (xmlChar*)"title")));
        /* check if draw already exists */
        int pos = drawlist->FindString(title);
        bool isnew = (pos < 0);
        double prior, old_prior;
        std::wstring str;
        xmlNodeList* list;

        /* Get the prior of window */
        xmlChar *tmp_prop= xmlGetProp(node, (xmlChar*)"prior");
        /* Prior of newly inserted element. */
	if (!tmp_prop) {
		prior = -1;
	} else {
		str = SC::U2S(tmp_prop); 
		if(str.empty()) {
			prior = -1;
		} else {
			prior = convert_to<double>(str);
		}
	}

        if (!isnew) {
               /* Old prior */
               list = (xmlNodeList*) (drawlist->GetClientData(pos));
               xmlNodePtr tmp = list->GetFirst()->GetData();
               tmp_prop = xmlGetProp(tmp, (xmlChar*)"prior");
		if (!tmp_prop) {
			 old_prior = -1;
		} else {
			str = SC::U2S(tmp_prop);
			if(str.empty()) {
				old_prior = -1;
			} else {
				old_prior = convert_to<double>(str);
			}
		}
		if (!prior_greater(prior, old_prior))
			prior = old_prior;
        } else {
                /* Append new window */
                list = new xmlNodeList();
                list->Append(node);
                drawlist->Append(title, (void*)list);
                pos = drawlist->GetCount() - 1;
        }
        /* Now get the window at 'pos' and sort it up. */
        while (pos > 0) {
                xmlNodePtr prev = (xmlNodePtr)
                        (((xmlNodeList*)drawlist->GetClientData(pos - 1))
                                ->GetFirst()->GetData());
		tmp_prop = xmlGetProp(prev, (xmlChar*)"prior");
		if (!tmp_prop) {
			 old_prior = -1;
		} else {
			str = SC::U2S(tmp_prop);
			if(str.empty()) {
				old_prior = -1;
			} else {
				old_prior = convert_to<double>(str);
			}
		}
                if (prior_greater(prior, old_prior)) {
                        /* Swap windows */
                        void* data = drawlist->GetClientData(pos - 1);
                        wxString entry = drawlist->GetString(pos - 1);
                        drawlist->Delete(pos - 1);
                        drawlist->InsertItems(1, &entry, pos);
                        drawlist->SetClientData(pos, data);
                        pos--;
                } else
                        break;
        }
        if (isnew)
                return;
        /* Now set the correct position of draw in window (according
          to 'order' attribute'. */
        /* 'list' is the list of draws. First reset prior of first element. */
        xmlNodePtr tmp = list->GetFirst()->GetData();
        xmlUnsetProp(tmp, (xmlChar*)"prior");
        /* remember the order of inserted element */
        double order;
	tmp_prop = xmlGetProp(node, (xmlChar*)"order");
        if (tmp_prop == NULL)
                list->Append(node);
        else {
		str = SC::U2S(tmp_prop);
		if(str.empty()) {
			order = -1;
		} else {
			order = convert_to<double>(str);
		}

                xmlNodeList::Node* t;
                /* search for position to insert current element */
                for (t = list->GetFirst(); t; t = t->GetNext()) {
                        xmlNodePtr n = t->GetData();
                        tmp_prop = xmlGetProp(n, (xmlChar*)"order");
                        if ((tmp_prop == NULL) || (convert_to<double>(SC::U2S(tmp_prop)) > order)) {
                                list->Insert(t, node);
                                break;
                        }
                }
                /* append element at end */
                if (t == NULL)
                        list->Append(node);
        }
        /* Now set the correct prior for the first element. */
        if (prior > 0) {
                tmp = list->GetFirst()->GetData();
                title.Printf(_T("%g"), prior);
                xmlSetRemoveProp(tmp, (xmlChar*)"prior", SC::S2U(title).c_str());
        }
}

void ConfFrame::FindElements(xmlNodePtr node)
{
        if (node == NULL)
                return;
        if ((node->type == XML_ELEMENT_NODE) && node->ns) {
                if (!strcmp((char *)node->name, "draw")
                        && !strcmp((char *)node->ns->href, 
                                (char *)SC::S2U(std::wstring(IPK_NAMESPACE_STRING)).c_str()))
                        AddDraw(node);
                else if (!strcmp((char *)node->name, "raport")
                        && !strcmp((char *)node->ns->href, 
                                (char *)SC::S2U(std::wstring(IPK_NAMESPACE_STRING)).c_str()))
                        AddRaport(node);
        }
        for (xmlNodePtr n = node->children; n; n = n->next)
                FindElements(n);
}
                
void ConfFrame::RaportSelected(wxCommandEvent& event)
{
        ritemslist->Clear();
        xmlNodeList::Node* node;
	if(event.GetClientData() != NULL) {
		for (node = ((xmlNodeList*)event.GetClientData())->GetFirst();
				node; node = node->GetNext()) {
			xmlChar* str = xmlGetProp(node->GetData(),
					(xmlChar*)"description");
			if (!str) {
				str = xmlGetProp(node->GetData()->parent,
						(xmlChar*)"name");
			}
			std::wstring s = SC::U2S(str);
			size_t last = s.find_last_of(':');
			if (last != std::wstring::npos) {
				s = s.substr(last + 1);
			}
			ritemslist->Append(wxString(s));
		}
	}
}

void ConfFrame::DrawSelected(wxCommandEvent& event)
{
        ditemslist->DeleteAllItems();
        xmlNodeList::Node* node;
	if(event.GetClientData() != NULL) {
		int i = 0;
		for (node = ((xmlNodeList*)event.GetClientData())->GetFirst();
				node; node = node->GetNext(), i++) {
			ditemslist->InsertItem(node->GetData());
		}
	}
	ditemslist->SortItems();
	ditemslist->AssignColors();
}

void ConfFrame::UpPressed(wxCommandEvent& event)
{
        wxString page_title = notebook->GetPageText(notebook->GetSelection());
        if (!page_title.Cmp(_("Raports")) && 
                (ritemslist->GetSelection() > 0)) 
                MoveRapItemUp();
        else if (!page_title.Cmp(_("Draws"))) {
		if (ditemslist->GetSelection() >= 0) {
			MoveDrawItemUp();
		} else if (drawlist->GetSelection() > 0) {
                        MoveDrawUp();
		}
        }
}

void ConfFrame::DownPressed(wxCommandEvent& event)
{
        wxString page_title = notebook->GetPageText(notebook->GetSelection());
        if (!page_title.Cmp(_("Raports")) && 
                (ritemslist->GetSelection() > 0) &&
                (ritemslist->GetSelection() < (static_cast<int>(ritemslist->GetCount()) - 1)) )
                MoveRapItemDown();
        else if (!page_title.Cmp(_("Draws"))) {
		if (ditemslist->GetSelection() >= 0) {
			MoveDrawItemDown();
		} else if (drawlist->GetSelection() > 0) {
                        MoveDrawDown();
		}
        }
}

void ConfFrame::MoveRapItemUp(void) 
{
	modified = TRUE;
        /* Get objects to swap */
        int pos = ritemslist->GetSelection();
        xmlNodeList* list = (xmlNodeList*) raplist->GetClientData(
                raplist->GetSelection());
        xmlNodePtr node = list->Item(pos)->GetData();
        xmlNodePtr prev_node = list->Item(pos-1)->GetData();
        /* Make sure that objects have order attributes. */
        xmlChar *porder = xmlGetProp(prev_node, (xmlChar*)"order");
        if (porder == NULL)
                AssignItemsPriors(list, pos - 1);
        /* Swap attribute values. */
        xmlChar* order = xmlGetProp(node, (xmlChar*)"order");
        porder = xmlGetProp(prev_node, (xmlChar*)"order");
        xmlSetRemoveProp(node, (xmlChar*)"order", porder);
        xmlSetRemoveProp(prev_node, (xmlChar*)"order", order);
        /* Swap list elements. */
        list->DeleteObject(prev_node);
        list->Insert(pos, prev_node);
        /* Swap listbox entries. */
        wxString entry = ritemslist->GetString(pos-1);
        ritemslist->Delete(pos - 1);
        ritemslist->InsertItems(1, &entry, pos);
        /* Make sure selection is in correct position */
        ritemslist->SetSelection(pos-1);
}

void ConfFrame::MoveRapItemDown(void)
{
	modified = TRUE;
        /* Get objects to swap */
        int pos = ritemslist->GetSelection();
        xmlNodeList* list = (xmlNodeList*) raplist->GetClientData(
                raplist->GetSelection());
        xmlNodePtr node = list->Item(pos)->GetData();
        xmlNodePtr next_node = list->Item(pos+1)->GetData();
        /* Make sure that objects have order attributes. */
        xmlChar *order = xmlGetProp(node, (xmlChar*)"order");
        if (order == NULL)
                AssignItemsPriors(list, pos);
        /* Swap attribute values. */
        xmlChar* norder = xmlGetProp(next_node, (xmlChar*)"order");
        order = xmlGetProp(node, (xmlChar*)"order");
        xmlSetRemoveProp(node, (xmlChar*)"order", norder);
        xmlSetRemoveProp(next_node, (xmlChar*)"order", order);
        /* Swap list elements. */
        list->DeleteObject(next_node);
        list->Insert(pos, next_node);
        /* Swap listbox entries. */
        wxString entry = ritemslist->GetString(pos + 1);
        ritemslist->Delete(pos + 1);
        ritemslist->InsertItems(1, &entry, pos);
        /* Make sure selection is in correct position */
        ritemslist->SetSelection(pos+1);
}

void ConfFrame::MoveDrawItemUp(void)
{
	modified = TRUE;
	ditemslist->MoveItemUp();
}

void ConfFrame::MoveDrawItemDown(void)
{
	modified = TRUE;
	ditemslist->MoveItemDown();
}

void ConfFrame::MoveDrawUp(void)
{
	modified = TRUE;
        /* Get objects to swap */
        int pos = drawlist->GetSelection();
        xmlNodeList* list = (xmlNodeList*) drawlist->GetClientData(
                pos);
	xmlNodeList* prevlist = (xmlNodeList*) drawlist->GetClientData(
		pos - 1);
        /* Make sure that previous has order attributes. */
        xmlChar *pprior = xmlGetProp(prevlist->GetFirst()->GetData(), 
			(xmlChar*)"prior");
	if (pprior && (pprior[0] == 0))
		pprior = NULL;
        if (pprior == NULL)
                AssignWindowPriors(pos - 1);
        /* Swap attribute values. */
        xmlChar* prior = xmlGetProp(list->GetFirst()->GetData(), 
			(xmlChar*)"prior");
        xmlSetRemoveProp(list->GetFirst()->GetData(), (xmlChar*)"prior", pprior);
        xmlSetRemoveProp(prevlist->GetFirst()->GetData(), (xmlChar*)"prior", prior);
        /* Swap listbox entries. */
        wxString entry = drawlist->GetString(pos-1);
        drawlist->Delete(pos - 1);
        drawlist->InsertItems(1, &entry, pos);
	drawlist->SetClientData(pos, (void*)prevlist);
        /* Make sure selection is in correct position */
        drawlist->SetSelection(pos-1);
}

void ConfFrame::MoveDrawDown(void)
{
	modified = TRUE;
        /* Get objects to swap */
        int pos = drawlist->GetSelection();
        xmlNodeList* list = (xmlNodeList*) drawlist->GetClientData(
                pos);
	xmlNodeList* nextlist = (xmlNodeList*) drawlist->GetClientData(
		pos + 1);
        /* Make sure that element have order attributes. */
        xmlChar* prior = xmlGetProp(list->GetFirst()->GetData(), 
			(xmlChar*)"prior");
        if (prior == NULL)
                AssignWindowPriors(pos);
        xmlChar *nprior = xmlGetProp(nextlist->GetFirst()->GetData(), 
			(xmlChar*)"prior");
        /* Swap attribute values. */
        xmlSetRemoveProp(list->GetFirst()->GetData(), (xmlChar*)"prior", nprior);
        xmlSetRemoveProp(nextlist->GetFirst()->GetData(), (xmlChar*)"prior", prior);
        /* Swap listbox entries. */

	Disconnect(ID_DrawList, wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(ConfFrame::DrawSelected));

        wxString entry = drawlist->GetString(pos);
        drawlist->Delete(pos);
        drawlist->InsertItems(1, &entry, pos+1);
	drawlist->SetClientData(pos+1, (void*)list);
        /* Make sure selection is in correct position */

	Connect(ID_DrawList, wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(ConfFrame::DrawSelected));

        drawlist->SetSelection(pos+1);

}

void ConfFrame::AssignItemsPriors(xmlNodeList* list, int pos) 
{
        char* str;
        wxString wxstr;
        double pr = 1;
        xmlNodeList::Node* el;

        for (el = list->GetFirst(); el && (pos >= 0); el = el->GetNext(),
                pos--) {
                xmlNodePtr node = el->GetData();
                str = (char*)xmlGetProp(node, (xmlChar*)"order");
                if (str == NULL) {
                        wxstr.Printf(_T("%g"), pr);
                        pr = pr + 1;
                        xmlSetRemoveProp(node, (xmlChar*)"order",
                                SC::S2U(wxstr).c_str());
                } else
                        pr = atof(str);
        }
}

void ConfFrame::AssignWindowPriors(int pos)
{
        char* str;
        wxString wxstr;
        double pr = 1;

        for (int i = 0; i <= pos; i++) {
                xmlNodePtr node = ((xmlNodeList*)drawlist->GetClientData(i))
			->GetFirst()->GetData();
                str = (char*)xmlGetProp(node, (xmlChar*)"prior");
                if (str == NULL) {
                        wxstr.Printf(_T("%g"), pr);
                        pr = pr + 1;
                        xmlSetRemoveProp(node, (xmlChar*)"prior",
                                SC::S2U(wxstr).c_str());
                } else
                        pr = atof(str);
        }
}

void ConfFrame::ClearElements(xmlNodePtr node)
{
        if (node == NULL)
                return;
        if ((node->type == XML_ELEMENT_NODE) && 
			node->ns &&
			( !strcmp((char *)node->name, "draw") ||
			  !strcmp((char *)node->name, "raport") ) &&
			!strcmp((char *)node->ns->href, (char *)SC::S2U(std::wstring(IPK_NAMESPACE_STRING)).c_str())) 
	{
		xmlRemoveProp(xmlHasProp(node, (xmlChar*)"prior")); 
		xmlRemoveProp(xmlHasProp(node, (xmlChar*)"order")); 
	}
        for (xmlNodePtr n = node->children; n; n = n->next)
                ClearElements(n);
}

void ConfFrame::OnClear(wxCommandEvent& WXUNUSED(event))
{
	ClearAll();
}

void ConfFrame::ClearAll()
{
	if (params == NULL)
		return;
	modified = TRUE;
	ClearElements(params->children);
	ReloadParams();
	ResetAll();
}

void ConfFrame::ResetAll()
{
	if (params == NULL)
		return;
	modified = TRUE;
	ClearElements(params->children);
	
	for (unsigned int i = 0; i < raplist->GetCount(); i++) {
		xmlNodeList * l = (xmlNodeList *)raplist->GetClientData(i);
		int j = 1;
		wxString str;
        	for (xmlNodeList::Node* lel = l->GetFirst(); 
				lel; lel = lel->GetNext(), j ++) {
			xmlNodePtr n = lel->GetData();
			str.Printf(_T("%d"), j);
			xmlSetProp(n, (xmlChar*)"order", 
					SC::S2U(str).c_str());
                }
	}

	for (unsigned int i = 0; i < drawlist->GetCount(); i++) {
		xmlNodeList * l = (xmlNodeList *)drawlist->GetClientData(i);
		int j = 1;
		wxString str;
		xmlNodePtr n;
		if (l) {
			n = (xmlNodePtr) l->GetFirst()->GetData();
			str.Printf(_T("%d"), i + 1);
			xmlSetProp(n, (xmlChar*)"prior", 
					SC::S2U(str).c_str());
		}
        	for (xmlNodeList::Node* lel = l->GetFirst(); 
				lel; lel = lel->GetNext(), j ++) {
			n = (xmlNodePtr) lel->GetData();
			str.Printf(_T("%d"), j);
			xmlSetProp(n, (xmlChar*)"order", 
					SC::S2U(str).c_str());
                }
	}
}


