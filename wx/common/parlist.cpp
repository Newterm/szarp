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
 * Parameters list implementation.
 * 

 * pawel@praterm.com.pl
 * 
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <wx/print.h>

#include <wx/wx.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "parlist.h"
#include "fonts.h"
#include "szapp.h"
#include "conversion.h"

namespace SC {
	std::wstring W2S(const wxString& c){
		return (std::wstring(c.ToStdWstring()));
	}
}

szParList::szParList()
{
	xml = NULL;
	last_path = _T("");
	LoadSchema();
	is_modified = FALSE;
}

szParList::szParList(const szParList& list)
{
	if (list.xml != NULL) {
		xml = xmlCopyDoc(list.xml, 1);
	} else {
		xml = NULL;
	}
	last_path = list.last_path;
	is_modified = list.is_modified;
	ipks = list.ipks;
	LoadSchema();

	for (size_t i = 0; i < list.Count(); i++) {
		xmlNodePtr onode = list.GetXMLNode(i);
		xmlNodePtr nnode = GetXMLNode(i);

		nnode->_private = onode->_private;
	}
}

szParList::szParList(const xmlDocPtr doc)
{
	assert (doc);
	last_path = _T("");
	LoadSchema();
	is_modified = false;
	xml = NULL;
	LoadXML(doc);
}

szParList& szParList::operator=(const szParList& list)
{
	if (this != &list) {
		if (xml) {
			xmlFreeDoc(xml);
		}
		if (list.xml) {
			xml = xmlCopyDoc(list.xml, 1);
		} else {
			xml = NULL;
		}
		last_path = list.last_path;
		is_modified = list.is_modified;
		ipks = list.ipks;

		for (size_t i = 0; i < list.Count(); i++) {
			xmlNodePtr onode = list.GetXMLNode(i);
			xmlNodePtr nnode = GetXMLNode(i);

			nnode->_private = onode->_private;
		}
	}
	return *this;
}

szParList::~szParList()
{
	if (xml)
		xmlFreeDoc(xml);
}

int szParList::LoadXML(const xmlDocPtr doc)
{
	if (xml) {
		xmlFreeDoc(xml);
	}
	xml = (xmlDocPtr) doc;

	BindAll();

	return Count();
}

int szParList::LoadFile(wxString path, bool showErrors)
{
	if (IsModified() && !IsEmpty()) {
		if (wxMessageBox(_("Current parameters list is not saved.\nContinue anyway?"), _("Parameters list modified"), wxICON_QUESTION | wxYES_NO) == wxNO)
			return 0;
		
	}
	if (path == wxEmptyString) {
		wxFileDialog dlg(NULL, _("Choose a file"), _T(""), _T(""), 
				_("SZARP parameters list (*.xpl)|*.xpl|All files (*.*)|*.*"));
		szSetDefFont(&dlg);
		if (dlg.ShowModal() != wxID_OK)
			return 0;
		path = dlg.GetPath();
	}

	xmlDocPtr doc = xmlParseFile(SC::S2A(SC::W2S(wxString::FromUTF8(path.c_str()))).c_str());
	if (doc == NULL) {
		if (!showErrors)
			return 0;
		if (wxSysErrorCode() != 0 ) {
			wxMessageBox(wxString::Format(_("Cannot open file %s\nError %d (%s)"),
						path.c_str(), wxSysErrorCode(), 
						wxSysErrorMsg()),
					_("Error"), wxICON_ERROR | wxOK);
		} else {
			wxMessageBox(wxString::Format(_("Document %s not well-formed"),
						path.c_str()),
					_("Error"), wxICON_ERROR | wxOK);
		}
		return 0;
	}
#if 0 
#error adjust schema so that control raport validates
	xmlRelaxNGValidCtxtPtr ctxt;
        int ret;

        ctxt = xmlRelaxNGNewValidCtxt(list_rng);
        ret = xmlRelaxNGValidateDoc(ctxt, doc);
        xmlRelaxNGFreeValidCtxt(ctxt);

        if (ret < 0) {
		if (showErrors)
			wxLogError(_("Failed to compile IPK schema!"));
                return 0;
        } else if (ret > 0) {
	if (showErrors)
			wxLogError(_("XML document doesn't validate against IPK schema!"));
               return 0;
        }
#endif

	Clear();
	if (xml) {
		xmlFreeDoc(xml);
	}
	xml = doc;
	is_modified = FALSE;
	BindAll();

	return Count();
}

bool szParList::SaveFile(wxString path, bool showErrors)
{
	if (xml == NULL)
		return FALSE;
	if (path == wxEmptyString) {
		if (last_path != _T(""))
			path = last_path;
		else  {
			wxFileDialog dlg(NULL, _("Choose a file"), _T(""), _T(""),
					_("SZARP parameters list (*.xpl)|*.xpl|All files (*.*)|*.*"),
					wxFD_SAVE | wxFD_CHANGE_DIR);
			szSetDefFont(&dlg);
			if (dlg.ShowModal() != wxID_OK)
				return FALSE;
			path = dlg.GetPath();
			if (path.Right(4).Find('.') < 0) {
				path += _T(".xpl");
			}
		}
	}

	int ret = xmlSaveFormatFileEnc(SC::S2A(SC::W2S(wxString::FromUTF8(path.c_str()))).c_str(), xml, "UTF-8", 1);
	if (ret == -1) {
		if (showErrors) {
			wxLogError(wxString::Format(_("Error saving document %s\nError %d: %s"), 
						path.c_str(), wxSysErrorCode(), 
						wxSysErrorMsg()));
		}
		return FALSE;
	}
	is_modified = FALSE;
	return TRUE;
}

bool szParList::IsModified()
{
	return is_modified;
}

void szParList::Clear()
{
	if (xml) {
		xmlFreeDoc(xml);
		xml = NULL;
		is_modified = TRUE;
	}
}

size_t szParList::Count() const
{
	int c = 0;
	if (xml == NULL)
		return 0;
	xmlNodePtr n = xml->children;
	assert (n != NULL);
	n = n->children;
	while (n) {
		if (n->type == XML_ELEMENT_NODE)
			c++;
		n = n->next;
	}

	return c;
}

wxString szParList::GetName(size_t index)
{
	wxString ret;
	xmlNodePtr n = GetXMLNode(index);
	if (n == NULL)
		return wxEmptyString;
	xmlChar *name = xmlGetProp(n, BAD_CAST "name");
	if (name == NULL)
		return wxEmptyString;
	ret = SC::U2S(name).c_str();
	xmlFree(name);
	return ret;
}
	
xmlNodePtr szParList::GetXMLNode(size_t index) const
{
	size_t i = 0;
	if (xml == NULL)
		return NULL;
	xmlNodePtr n = xml->children;
	if (n == NULL)
		return NULL;
	n = n->children;
	while (n) {
		while (n && (n->type != XML_ELEMENT_NODE))
			n = n->next;
		if (i == index)
			return n;
		if (n)
			n = n->next;
		i++;
	}
	return n;
}

xmlNodePtr szParList::GetXMLRootNode() {
  return xmlDocGetRootElement(xml);
}

TParam* szParList::GetParam(size_t index)
{
	xmlNodePtr n = GetXMLNode(index);
	if (n == NULL)
		return NULL;
	return (TParam*)n->_private;
}

size_t szParList::Append(TParam *param)
{
	xmlNodePtr node;
	
	TSzarpConfig *conf = param->GetSzarpConfig();
	CreateEmpty();
	if (!IsRegisteredIPK(conf))
		RegisterIPK(conf);
	
	node = xmlNewChild(xml->children, NULL,  BAD_CAST "param", NULL);
	xmlSetProp(node, BAD_CAST "name", SC::S2U(param->GetName()).c_str());
	is_modified = TRUE;

	node->_private = param;

	return Count() - 1;
}

size_t szParList::Insert(TParam *param, size_t index)
{
	xmlNodePtr node;
	
	TSzarpConfig *conf = param->GetSzarpConfig();
	CreateEmpty();
	if (!IsRegisteredIPK(conf)) {
		RegisterIPK(conf);
	}
	if (index >= Count())
		return Append(param);
	node = xmlNewNode(NULL, BAD_CAST "param");
	xmlAddPrevSibling(GetXMLNode(index), node);
	is_modified = TRUE;
	return index;
}

void szParList::Remove(size_t index)
{
	xmlNodePtr node;
	node = GetXMLNode(index);
	if (node == NULL)
		return;
	xmlUnlinkNode(node);
	xmlFreeNode(node);
	is_modified = TRUE;
}

void szParList::RegisterIPK(TSzarpConfig *ipk)
{
	if (ipks.Count() == 0) {
		CreateEmpty();
		xmlSetProp(xml->children, BAD_CAST "source", SC::S2U(ipk->GetTitle()).c_str() );
	}
	ipks.Add(ipk);
}

bool szParList::IsRegisteredIPK(TSzarpConfig *ipk)
{
	for (size_t i = 0; i < ipks.Count(); i++)
		if (ipk->GetTitle() == ipks.Item(i)->GetTitle())
			return true;
	return false;
}

size_t szParList::Unbinded()
{
	if (xml == NULL)
		return 0;
	int c = 0;
	xmlNodePtr n = xml->children;
	assert (n != NULL);
	n = n->children;
	while (n) {
		if ((n->type == XML_ELEMENT_NODE) && (n->_private == NULL))
			c++;
		n = n->next;
	}
	return c;
}

void szParList::RemoveUnbinded()
{
	if (xml == NULL)
		return;
	xmlNodePtr nn, n = xml->children;
	assert (n != NULL);
	n = n->children;
	while (n) {
		nn = n->next;
		if ((n->type == XML_ELEMENT_NODE) && (n->_private == NULL)) {
			xmlUnlinkNode(n);
			xmlFreeNode(n);
			is_modified = TRUE;
		}
		n = nn;
	}
}

wxString szParList::GetConfigName()
{
	if (ipks.Count() <= 0) {
		xmlChar* def = xmlGetProp(xml->children, BAD_CAST "source");
		if (def == NULL) {
			return _T("");
		}
		return wxString(SC::U2S(def));
	}
	return ipks.Item(0)->GetTitle();
}

bool szParList::IsEmpty()
{
	return (Count() == 0);
}

void szParList::Bind(TSzarpConfig *ipk)
{
	xmlNodePtr n;
	xmlChar* def;
	if (xml == NULL)
		return;
	def = xmlGetProp(xml->children, BAD_CAST"source");
	for (n = xml->children->children; n; n = n->next) {
		if (n->_private || n->type != XML_ELEMENT_NODE)
			continue;
		xmlChar *cur = xmlGetProp(n, BAD_CAST"source");
		if (cur == NULL)
			cur = xmlStrdup(def);
		if (ipk->GetTitle() == SC::U2S(cur)) {
			xmlChar *name = xmlGetProp(n, BAD_CAST"name");
			assert(name);
			n->_private = ipk->getParamByName(SC::U2S(name));
			xmlFree(name);
		}
		xmlFree(cur);
	}
	xmlFree(def);
	
}

void szParList::BindAll()
{
	if (xml == NULL)
		return;
	for (unsigned int i = 0; i < ipks.Count(); i++) {
		Bind(ipks.Item(i));
	}
}

xmlRelaxNGPtr szParList::list_rng = NULL;

void szParList::LoadSchema()
{
	if (list_rng) {
		return;
	}
	xmlRelaxNGParserCtxtPtr ctxt;

	wxString dir = dynamic_cast<szAppConfig*>(wxTheApp)->GetSzarpDir();

#ifndef MINGW32
		dir += _T("/resources/dtd/params-list.rng");
#else
		dir += _T("\\resources\\dtd\\params-list.rng");
#endif

        ctxt = xmlRelaxNGNewParserCtxt(SC::S2A(SC::W2S(wxString::FromUTF8(dir.c_str()))).c_str());

        list_rng = xmlRelaxNGParse(ctxt);
	xmlRelaxNGFreeParserCtxt(ctxt);
        if (list_rng == NULL) {
                wxLogWarning(_("Could not load RelaxNG schema from file %s"), dir.c_str());
        }

}

void szParList::Cleanup()
{
	if (list_rng == NULL) {
		return;
	}
	xmlRelaxNGFree(list_rng);
	list_rng = NULL;
}

void szParList::CreateEmpty()
{
	if (xml != NULL)
		return;
	xml = xmlNewDoc(BAD_CAST "1.0");
	assert (xml != NULL);
	xmlNodePtr node = xmlNewNode(NULL, BAD_CAST "parameters");
	assert (node != NULL);
	xmlDocSetRootElement(xml, node);
	xmlNewNs(node, BAD_CAST SZPARLIST_NS_HREF, NULL);
}

wxListBox* szParList::AsListBox(wxWindow* parent, wxWindowID id, const wxPoint& pos, 
		const wxSize& size, long style)
{
	wxListBox *lbox = new wxListBox(parent, id, pos, size, 0, NULL, style);
	FillListBox(lbox);
	return lbox;
}

void szParList::FillListBox(wxListBox* lbox)
{
	assert (lbox != NULL);
	if (xml == NULL)
		return;
	lbox->Clear();
	BindAll();
	for (xmlNodePtr node = xml->children->children; node; node = node->next) {
		xmlChar *s = xmlGetProp(node, BAD_CAST "name");
		if (s == NULL)
			continue;
		lbox->Append(SC::U2S(s).c_str(), node->_private);
		xmlFree(s);
	}
}

void szParList::FillCheckListBox(wxCheckListBox* lbox)
{
	assert (lbox != NULL);
	if (xml == NULL)
		return;
	lbox->Clear();
	BindAll();
	for (xmlNodePtr node = xml->children->children; node; node = node->next) {
		xmlChar *s = xmlGetProp(node, BAD_CAST "name");
		xmlAttrPtr b = xmlHasProp(node, BAD_CAST "value");
		bool byvalue = b != NULL;
		if (s == NULL)
			continue;
		int pos = lbox->Append(SC::U2S(s).c_str(), node->_private);
		lbox->Check( pos , byvalue );
		xmlFree(s);
	}
}

void szParList::Check( unsigned int pos , bool check )
{
	unsigned int i=0;
	xmlNodePtr node;
	// find param at position pos
	for (node = xml->children->children; node; node = node->next) {
		if( xmlHasProp(node, BAD_CAST "name") != NULL ) ++i;
		if( i == pos+1 ) break;
	}
	// check if its not to far
	if (!node) return;
	// set or unset attribute value
	if (check) 
		xmlSetProp(node,BAD_CAST "value",BAD_CAST "");
	else
		xmlUnsetProp(node,BAD_CAST "value");
}

class szParListPrintout : public wxPrintout {
	public:
	szParListPrintout(szParList* parlist, wxString title);
	bool OnPrintPage(int pageNum);
	void GetPageInfo(int *minPage, int *maxPage, 
			int *pageFrom, int *pageTo);
	bool HasPage(int pageNum);
	protected:
	szParList *parlist;
	int pages;
};

szParListPrintout::szParListPrintout(szParList* parlist, wxString title)
	: wxPrintout(title)
{
	this->parlist = parlist;
	pages = parlist->Count() / 20 + 1;
}
	
void szParListPrintout::GetPageInfo(int *minPage, int *maxPage, 
			int *pageFrom, int *pageTo)
{
	*minPage = *pageFrom = 1;
	*maxPage = *pageTo = pages;
}

bool szParListPrintout::HasPage(int pageNum)
{
	return ((pageNum >= 1) && (pageNum <= pages));
}

bool szParListPrintout::OnPrintPage(int pageNum)
{
	int params_per_page = 20;
	int margin_percent = 5;
	int w, h;
	float xscale = 1.0;
	float yscale = 1.0;
	wxDC *dc = GetDC();

	dc->GetSize(&w, &h);
	
	dc->SetUserScale(xscale, yscale);
	
	int maxw = 0;
	int maxh = 0;
	for (int i = (pageNum -1) * params_per_page; 
			(i < (int)parlist->Count()) && (i < pageNum * params_per_page) ; 
			i++) {
		int curw, curh;
		dc->GetTextExtent(parlist->GetName(i), &curw, &curh);
		maxw = wxMax(curw, maxw);
		maxh = wxMin(curh, maxh);
	}
	int text_w = (w - 2 * w * margin_percent / 100);
	if (maxw > text_w)
		xscale = (float)text_w / (float)maxw;
	int text_h = (h - (2 * h * margin_percent / 100)) / params_per_page;
	if (maxh > text_h)
		yscale = (float)text_h / (float)maxh;
	dc->SetUserScale(1/xscale, 1/yscale);
	dc->SetDeviceOrigin(0, 0);
	dc->SetBackground(*wxWHITE_BRUSH);
	dc->Clear();
	
	dc->SetBackgroundMode(wxTRANSPARENT);
	
	wxFont parFont(14, //size
			wxROMAN, // family
			wxNORMAL, // style
			wxNORMAL, // weight
			FALSE, // no underline
			_T(""), // default facetype
#ifdef MING32
			wxFONTENCODING_CP1250 // encoding for Windows
#else
			wxFONTENCODING_ISO8859_2 // encoding for Linux
#endif
		      );
	dc->SetFont(parFont);
	for (int i = (pageNum -1) * params_per_page; 
			(i < (int)parlist->Count()) && (i < pageNum * params_per_page) ; 
			i++) {
		dc->DrawText(parlist->GetName(i), (int)(w * margin_percent / xscale / 100), 
				(int)((h * (i % params_per_page)  / (params_per_page + 1) + h * margin_percent / 100) / yscale));
	}
	return TRUE;
}

void szParList::Print(wxWindow *parent)
{
	wxPrinter printer;
	szParListPrintout printout(this, GetConfigName());
	
	printer.Print(parent, &printout, TRUE);
}

void szParList::PrintPreview(wxFrame *parent)
{
	/* Doesn't work under linux/gtk wxwidgets 2.4 version - bad font scalling */
	wxPrintPreview *preview = new wxPrintPreview(
			new szParListPrintout(this, GetConfigName()), NULL);
	wxPreviewFrame *frame = new wxPreviewFrame(preview, parent, 
			_("Print preview"), wxDefaultPosition, wxSize(650, 600));
	frame->Centre(wxBOTH);
	frame->Initialize();
	frame->Show(TRUE);
}

wxString szParList::GetExtraProp(size_t index, const wxString &ns, 
		const wxString &attrib) 
{
	assert (xml);
	xmlNodePtr n = GetXMLNode(index);
	if (n == NULL) {
		return wxEmptyString;
	}
	xmlChar *p = xmlGetNsProp(n, SC::S2U(SC::W2S(wxString::FromUTF8(attrib.c_str()))).c_str(), SC::S2U(SC::W2S(wxString::FromUTF8(ns.c_str()))).c_str());
	if ( p == NULL) {
		return wxEmptyString;
	}
	wxString p_wx = SC::U2S(p).c_str();
	xmlFree(p);
	return p_wx;
}

void szParList::SetExtraProp(size_t index, const wxString& ns,
		const wxString &attrib, const wxString &value) 
{
	assert (xml);
	xmlNodePtr n = GetXMLNode(index);
	if (n == NULL) {
		return;
	}
	xmlNsPtr np = xmlSearchNsByHref(xml, n, SC::S2U(SC::W2S(wxString::FromUTF8(ns.c_str()))).c_str());
	if (np == NULL) {
		return;
	}
	xmlSetNsProp(n, np, SC::S2U(SC::W2S(wxString::FromUTF8(attrib.c_str()))).c_str(), SC::S2U(SC::W2S(wxString::FromUTF8(value.c_str()))).c_str());
}

wxString szParList::GetExtraRootProp(const wxString &ns, const wxString &attrib) 
{
	assert (xml);
	xmlNodePtr n = GetXMLRootNode();
	if (n == NULL) {
		return wxEmptyString;
	}
	xmlChar *p = xmlGetNsProp(n, SC::S2U(SC::W2S(wxString::FromUTF8(attrib.c_str()))).c_str(), SC::S2U(SC::W2S(wxString::FromUTF8(ns.c_str()))).c_str());
	if (p == NULL) {
		return wxEmptyString;
	}
	wxString p_wx = SC::U2S(p).c_str();
	xmlFree(p);
	return p_wx;
}

void szParList::SetExtraRootProp(const wxString &ns, const wxString &attrib, 
		const wxString &value) 
{
	assert (xml);
	xmlNodePtr n = GetXMLRootNode();
	if ( n == NULL ) {
		return;
	}
	xmlNsPtr np = xmlSearchNsByHref(xml, n, SC::S2U(SC::W2S(wxString::FromUTF8(ns.c_str()))).c_str());
	if (np == NULL) {
		return;
	}
	xmlSetNsProp(n, np, SC::S2U(SC::W2S(wxString::FromUTF8(attrib.c_str()))).c_str(), SC::S2U(SC::W2S(wxString::FromUTF8(value.c_str()))).c_str());
}

void szParList::AddNs(const wxString &prefix, const wxString &href) 
{
	assert (xml);
	xmlNewNs(xmlDocGetRootElement(xml), 
			SC::S2U(SC::W2S(wxString::FromUTF8(href.c_str()))).c_str(), 
			SC::S2U(SC::W2S(wxString::FromUTF8(prefix.c_str()))).c_str());
}

