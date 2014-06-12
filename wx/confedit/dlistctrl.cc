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
 * confedit - SZARP configuration editor
 * pawel@praterm.com.pl
 *
 * $Id$
 */

#include "dlistctrl.h"
#include "deditdlg.h"
#include "cconv.h"

#include <wx/imaglist.h>

DrawsListCtrl::DrawsListCtrl(wxWindow* parent, wxWindowID id, 
			const wxPoint& pos, const wxSize& size, long style) 
		: wxListCtrl(parent, id, pos, size, style | wxLC_REPORT | wxLC_SINGLE_SEL)
		, modified(false)
{  
	InsertColumn(0, _T("Name"), wxLIST_ALIGN_DEFAULT, 
			GetSize().GetWidth() - (20 + 50 + 50 + 1));
	InsertColumn(1, _T(" "), wxLIST_ALIGN_DEFAULT, 20);
	InsertColumn(2, _T("min"), wxLIST_ALIGN_DEFAULT, 50);
	InsertColumn(3, _T("max"), wxLIST_ALIGN_DEFAULT, 50);

	/* prepare 2 sets of color images, first for real colors, second with gray border
	 * for auto assigned colors */
	wxImageList* image_list = new wxImageList(16, 16);
	int dsize = (int)(sizeof DrawDefaultColors::dcolors / sizeof *DrawDefaultColors::dcolors);
	for (int i = 0; i < 2 * dsize; i++) {
		wxImage im(16, 16);
		int border = 0;
		if (i >= dsize) {
			border = 2;
			  im.SetRGB(wxRect(0, 0, 16, 16), 175, 175, 175);
		}
		im.SetRGB(wxRect(0 + border, 0 + border, 16 - 2 * border, 16 - 2 * border), 
				DrawDefaultColors::dcolors[i % dsize][0], 
				DrawDefaultColors::dcolors[i % dsize][1], 
				DrawDefaultColors::dcolors[i % dsize][2]);

		image_list->Add(im);
	}
	AssignImageList(image_list, wxIMAGE_LIST_SMALL);

	Connect(GetId(), wxEVT_COMMAND_LIST_ITEM_ACTIVATED, 
			wxListEventHandler(DrawsListCtrl::OnDrawEdit));
}

long DrawsListCtrl::InsertItem(xmlNodePtr node)
{
	assert (node != NULL);
	std::wstring s;
	xmlChar* str = xmlGetProp(node->parent, BAD_CAST "draw_name");
	if (!str) {
		str = xmlGetProp(node->parent, BAD_CAST "name");
		if (rindex((const char *)str, ':')) {
			// FIXME: may be hardcoded ISO-8859-2
			s = SC::L2S(rindex((const char*)str, ':'));
		} else {
			s = SC::U2S(str);
		}
	} else {
		s = SC::U2S(str);
	}
	xmlFree(str);
	long index = wxListCtrl::InsertItem(0, s);
	SetItemPtrData(index, (wxUIntPtr)node);
	wxListItem l;
	l.SetId(0);
	l.SetColumn(1);
	SetItem(l);

	str = xmlGetProp(node, BAD_CAST "min");
	SetItem(index, 2, SC::U2S(str));
	xmlFree(str);

	str = xmlGetProp(node, BAD_CAST "max");
	SetItem(index, 3, SC::U2S(str));
	xmlFree(str);

	return index;
}

void DrawsListCtrl::OnDrawEdit(wxListEvent &ev)
{
	  wxListItem l;
	  l.SetId(GetSelection());
	  l.SetColumn(1);
	  GetItem(l);
	  int c_ind = l.GetImage();
	  if (c_ind >= 12) {
		  c_ind = -1;
	  }
	  std::wstring s;
	  l.SetColumn(0);
	  GetItem(l);
	  xmlNodePtr node = (xmlNodePtr) l.GetData();
	  xmlChar *tmp = xmlGetProp(node, BAD_CAST "min");
	  wxString min = SC::U2S(tmp);
	  xmlFree(tmp);
	  tmp = xmlGetProp(node, BAD_CAST "max");
	  wxString max = SC::U2S(tmp);
	  xmlFree(tmp);
	 
	  DrawEditDialog dlg(this, c_ind, min, max);

	  if (dlg.ShowModal() != wxID_OK) {
		  return;
	  }

	  if (c_ind == -1) {
		  xmlUnsetProp(node, BAD_CAST "color");
	  } else {
		  xmlSetProp(node, BAD_CAST "color", BAD_CAST 
				  SC::S2U(DrawDefaultColors::AsString(c_ind)).c_str());
		  modified = true;
	  }
	  double dmin, dmax;
	  if (min.ToDouble(&dmin) and max.ToDouble(&dmax) and (dmin < dmax)) {
		  min = wxString::Format(_T("%g"), dmin);
		  max = wxString::Format(_T("%g"), dmax);
		  min.Replace(_T(","), _T("."));
		  max.Replace(_T(","), _T("."));
		  xmlSetProp(node, BAD_CAST "min", BAD_CAST SC::S2U(min).c_str());
		  xmlSetProp(node, BAD_CAST "max", BAD_CAST SC::S2U(max).c_str());
  		  SetItem(l.GetId(), 2, min);
  		  SetItem(l.GetId(), 3, max);
		  modified = true;
	  }
	  AssignColors();
}

void DrawsListCtrl::MoveItemUp()
{
	  long pos = GetSelection();
	  if (pos <= 0) {
		  return;
	  }
	  MoveItem(pos, pos - 1);
}

void DrawsListCtrl::MoveItemDown()
{
	  long pos = GetSelection();
	  if (pos == -1) {
		  return;
	  }
	  if (pos == GetItemCount() - 1) {
		  return;
	  }
	  MoveItem(pos, pos + 1);
}

void DrawsListCtrl::AssignColors()
{
	bool used[12];
	for (int i = 0; i < 12; i++) {
		used[i] = false;
	}
	long itemIndex = -1;
	for (;;) {
		itemIndex = GetNextItem(itemIndex);
		if (itemIndex == -1) break;
		xmlNodePtr node = (xmlNodePtr)(GetItemData(itemIndex));
		xmlChar* c = xmlGetProp(node, BAD_CAST "color");
		if (c != NULL) {
			std::wstring str = SC::U2S(c);
			wxColour col(str);
			/* remove incorrect colors */
			if (!col.IsOk()) {
				xmlUnsetProp(node, BAD_CAST "color");
				continue;
			}
			int index = DrawDefaultColors::FindIndex(col);
			if (index >= 0) {
				used[index] = true;
			} else {
				xmlUnsetProp(node, BAD_CAST "color");
			}
		}
	}
	itemIndex = -1;
	for (;;) {
		itemIndex = GetNextItem(itemIndex);
		if (itemIndex == -1) break;
		xmlNodePtr node = (xmlNodePtr)(GetItemData(itemIndex));
		xmlChar* c = xmlGetProp(node, BAD_CAST "color");
		int setcolor;
		if (c != NULL) {
			std::wstring str = SC::U2S(c);
			wxColour col(str);
			int index = DrawDefaultColors::FindIndex(col);
			assert (index >= 0 and index < 12);
			setcolor = index;
		} else {
			/* find first free color */
			int f;
			for (f = 0; f < 12; f++) {
				if (!used[f]) {
					break;
				}
			}
			assert (f < 12);
			used[f] = true;
			setcolor = f + 12;
		}
		wxListItem l;
		l.SetId(itemIndex);
		l.SetColumn(1);
		GetItem(l);
		l.SetImage(setcolor);
		SetItem(l);
	}
}

void DrawsListCtrl::SortItems()
{
	wxListCtrl::SortItems(sort_callback, 0);
}

long DrawsListCtrl::GetSelection()
{
	return GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}


void DrawsListCtrl::MoveItem(long from, long to)
{
	modified = true;
	assert (from >= 0);
	assert (to >= 0);
	char tmp[4];
	tmp[3] = 0;

	wxListItem l;
	l.SetId(from);
	GetItem(l);
	xmlNodePtr node = (xmlNodePtr) l.GetData();
	snprintf(tmp, 3, "%ld", to + 1);
	xmlSetProp(node, BAD_CAST "order", BAD_CAST tmp);
	if (from < to) {
		for (long i = from + 1; i <= to; i++) {
			l.SetId(i);
			GetItem(l);
			xmlNodePtr node = (xmlNodePtr) l.GetData();
			snprintf(tmp, 3, "%ld", i - 1 + 1);
			xmlSetProp(node, BAD_CAST "order", BAD_CAST tmp);
		}
	} else {
		for (long i = to; i < from; i++) {
			l.SetId(i);
			GetItem(l);
			xmlNodePtr node = (xmlNodePtr) l.GetData();
			snprintf(tmp, 3, "%ld", i + 1 + 1);
			xmlSetProp(node, BAD_CAST "order", BAD_CAST tmp);
		}
	}
	
	SortItems();
	AssignColors();
	l.SetId(to);
	GetItem(l);
	l.SetState(wxLIST_STATE_SELECTED);
	SetItem(l);
}
	
int wxCALLBACK DrawsListCtrl::sort_callback(long item1, long item2, long ignored)
{
	xmlNodePtr node1 = (xmlNodePtr) item1;
	xmlNodePtr node2 = (xmlNodePtr) item2;
	xmlChar* prop1 = xmlGetProp(node1, BAD_CAST "order");
	xmlChar* prop2 = xmlGetProp(node2, BAD_CAST "order");
	double p1 = -1;
	double p2 = -1;
	if (prop1) {
		p1 = atof((const char*)prop1);
	}
	if (prop2) {
		p2 = atof((const char*)prop2);
	}
	if (p1 * p2 > 0) {
		if (p1 < p2) {
			return -1;
		} else if (p1 > p2) {
			return 1;
		} else {
			return 0;
		}
	} else if (p1 > 0) {
		return -1;
	} else if (p1 < 0) {
		return 1;
	} else {
		return 0;
	}
}

