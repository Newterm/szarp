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
#include <algorithm>
#include <sstream>

#include <wx/dcbuffer.h>
#include <wx/splitter.h>

#include "szhlpctrl.h"

#include "cconv.h"
#include "szframe.h"

#include "ids.h"
#include "classes.h"

#include "cfgmgr.h"
#include "coobs.h"
#include "defcfg.h"
#include "drawtime.h"
#include "dbinquirer.h"
#include "database.h"
#include "dbmgr.h"
#include "draw.h"
#include "drawview.h"
#include "xydiag.h"
#include "xygraph.h"
#include "progfrm.h"
#include "timeformat.h"
#include "drawprint.h"

#include "frmmgr.h"

extern int CursorMovementSpeed[PERIOD_T_LAST];

wxString XYFormatTime(const wxDateTime& time, PeriodType pt);

XYFrame::XYFrame(wxString default_prefix, DatabaseManager *db_manager, ConfigManager *cfgmanager, TimeInfo time, DrawInfoList user_draws, FrameManager *frame_manager) :
	szFrame(NULL, XY_GRAPH_FRAME, _("X/Y graph"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS) {

	m_default_prefix = default_prefix;

	m_cfg_manager = cfgmanager;

	m_db_manager = db_manager;

	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);	
	m_panel = new XYPanel(this, db_manager, cfgmanager, frame_manager, default_prefix);
	sizer->Add(m_panel, 1, wxEXPAND | wxALL, 5);

	wxMenuBar *menu_bar = new wxMenuBar();

	wxMenu *menu = new wxMenu();
	menu->Append(XY_CHANGE_GRAPH, _("Change graph parameters"));
	menu->AppendSeparator();
	menu->Append(XY_PRINT, _("Print\tCtrl-P"));
	menu->Append(XY_PRINT_PAGE_SETUP, _("Page Settings\tCtrl+Shift+S"));
	menu->Append(XY_PRINT_PREVIEW, _("Print preview"));
	menu->AppendSeparator();
	menu->Append(XY_ZOOM_OUT, _("Zoom out\tCtrl--"));
	menu->Enable(XY_ZOOM_OUT, false);
	menu->AppendSeparator();
	menu->Append(wxID_CLOSE, _("Close"));

	menu_bar->Append(menu, _("Graph"));

	menu = new wxMenu();
	menu->Append(wxID_HELP, _("Help"));
	menu_bar->Append(menu, _("Help"));

	SetMenuBar(menu_bar);

	SetSize(800, 600);

	m_dialog = new XYDialog(this,
			m_default_prefix,
			m_cfg_manager,
			m_db_manager,
			time,
			user_draws,
			this);

	if (m_dialog->ShowModal() == wxID_OK)
		SetGraph(m_dialog->GetGraph());
	else
		Destroy();

}

void XYFrame::OnClose(wxCloseEvent &event) {

	if (event.CanVeto()) {
		int ret = wxMessageBox(_("Do you want to close this window?"), _("Window close"), wxICON_QUESTION | wxYES_NO, this);
		if (ret == wxYES)
			Destroy();
	} else {
		Destroy();
	}
}

XYFrame::~XYFrame() {
	m_dialog->Destroy();
}

int XYFrame::GetDimCount() {
	return 2;
}

void XYFrame::SetGraph(XYGraph *graph) {
	if (IsShown() == false)
		Show(true);

	m_panel->SetGraph(graph);
}

void XYFrame::OnCloseMenu(wxCommandEvent &event) {
	Destroy();
}

void XYFrame::OnGraphChange(wxCommandEvent &event) {
	if (m_dialog->ShowModal() == wxID_OK)
		SetGraph(m_dialog->GetGraph());
}

void XYFrame::OnPrint(wxCommandEvent &event) {
	m_panel->Print(false);
}

void XYFrame::OnPrintPageSetup(wxCommandEvent &event) {
	Print::PageSetup(this);	
}


void XYFrame::OnPrintPreview(wxCommandEvent &event) {
	m_panel->Print(true);
}

void XYFrame::OnZoomOut(wxCommandEvent &event) {
	m_panel->ZoomOut();	
	m_panel->SetFocus();
}

void XYFrame::OnHelp(wxCommandEvent &event) {
	SetHelpText(_T("draw3-ext-chartxy"));
	wxHelpProvider::Get()->ShowHelp(this);
	m_panel->SetFocus();
}

BEGIN_EVENT_TABLE(XYFrame, szFrame)
	EVT_MENU(XY_CHANGE_GRAPH, XYFrame::OnGraphChange)
	EVT_MENU(wxID_CLOSE, XYFrame::OnCloseMenu)
	EVT_MENU(XY_PRINT_PREVIEW, XYFrame::OnPrintPreview)
	EVT_MENU(XY_PRINT_PAGE_SETUP, XYFrame::OnPrintPageSetup)
	EVT_MENU(XY_PRINT, XYFrame::OnPrint)
	EVT_MENU(XY_ZOOM_OUT, XYFrame::OnZoomOut)
	EVT_MENU(wxID_HELP, XYFrame::OnHelp)
	EVT_CLOSE(XYFrame::OnClose)
END_EVENT_TABLE()

class XYKeyboardHandler : public wxEvtHandler {
	XYGraphWidget *m_graph;

	public:
	XYKeyboardHandler(XYGraphWidget *graph);
	void OnKeyUp(wxKeyEvent &event);
	void OnKeyDown(wxKeyEvent & event);

	DECLARE_EVENT_TABLE()
};

XYKeyboardHandler::XYKeyboardHandler(XYGraphWidget *graph) : m_graph(graph) {}

void XYKeyboardHandler::OnKeyDown(wxKeyEvent &event) {
	switch (event.GetKeyCode()) {
		case WXK_LEFT:
			m_graph->SetKeyboardAction(CURSOR_LEFT_KB);
			break;
		case WXK_RIGHT:
			m_graph->SetKeyboardAction(CURSOR_RIGHT_KB);
			break;
		case WXK_DOWN:
			m_graph->SetKeyboardAction(CURSOR_DOWN_KB);
			break;
		case WXK_UP:
			m_graph->SetKeyboardAction(CURSOR_UP_KB);
			break;
		default:
			event.Skip();
			break;
	}
	m_graph->SetFocus();
}

void XYKeyboardHandler::OnKeyUp(wxKeyEvent & event) {
	switch (event.GetKeyCode()) {
		case WXK_LEFT:
		case WXK_RIGHT:
		case WXK_DOWN:
		case WXK_UP:
			m_graph->SetKeyboardAction(NONE);
			break;
		default:
			event.Skip();
		break;
	}
}

BEGIN_EVENT_TABLE(XYKeyboardHandler, wxEvtHandler)
	EVT_KEY_DOWN(XYKeyboardHandler::OnKeyDown)
	EVT_KEY_UP(XYKeyboardHandler::OnKeyUp)
END_EVENT_TABLE()


XYPanel::XYPanel(wxWindow *parent, DatabaseManager *database_manager, ConfigManager *config_manager, FrameManager *frame_manager, wxString default_prefix) 
		: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize) {
	m_default_prefix = default_prefix;
	m_database_manager = database_manager;
	m_config_manager = config_manager;

	wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);	

	wxSplitterWindow *splitter = new wxSplitterWindow(this, wxID_ANY);
	m_point_info = new XYPointInfo(splitter, m_config_manager, frame_manager, default_prefix);
	m_graph = new XYGraphWidget(splitter, m_point_info);

	splitter->SplitVertically(m_graph, m_point_info, -300);
	splitter->SetSashGravity(1);
	splitter->SetMinimumPaneSize(120);

// 	sizer->Add(m_graph, 1, wxEXPAND | wxALL, 5);
// 	sizer->Add(m_point_info, 0, wxALIGN_LEFT | wxALL, 5);

	sizer->Add(splitter, 1, wxEXPAND | wxALL, 5);

	XYKeyboardHandler *eh;  
 	eh = new XYKeyboardHandler(m_graph);
 	m_point_info->PushEventHandler(eh);
	eh  = new XYKeyboardHandler(m_graph);
	m_graph->PushEventHandler(eh);
	eh  = new XYKeyboardHandler(m_graph);
	parent->PushEventHandler(eh);
	eh  = new XYKeyboardHandler(m_graph);
	PushEventHandler(eh);

	SetSizer(sizer);
}

#if 0
bool XYPanel::StartNewGraph() {
	XYDialog * dialog = new XYDialog(this, m_default_prefix, m_config_manager);
	if (dialog->ShowModal() != wxID_OK) {
		dialog->Destroy();
		return false;
	}

	m_graph->StopDrawing();
	assert(m_mangler == NULL);
	m_mangler = new DataMangler(m_database_manager,
			dialog->GetXDraw(),
			dialog->GetYDraw(),
			dialog->GetStartTime(),
			dialog->GetEndTime(),
			dialog->GetPeriod(),
			this);

	dialog->Destroy();

	m_mangler->Go();

	return true;
}
#endif

void XYPanel::SetGraph(XYGraph *graph) {
	m_graph->SetGraph(graph);
	m_point_info->SetGraphInfo(graph);
	m_graph->StartDrawing();
}

void XYPanel::Print(bool preview) {
	m_graph->Print(preview);
}

void XYPanel::ZoomOut() {
	m_graph->ZoomOut();
}

const int XYGraphWidget::left_margin = 64;
const int XYGraphWidget::right_margin = 64;
const int XYGraphWidget::top_margin = 24;
const int XYGraphWidget::bottom_margin = 24;

XYGraphWidget::XYGraphWidget(wxWindow *parent, XYPointInfo *point_info) : 
	wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE),
	m_painter(left_margin, right_margin, top_margin, bottom_margin)
{
	m_cursor_index = -1;
	m_drawing = false;
	m_point_info = point_info;
	m_graph = NULL;
	m_kdtree = NULL;

	m_bmp = NULL;
	m_keyboard_timer = new wxTimer(this, wxID_ANY);

	m_current_rect.x = -1;

	m_current_size = wxSize(-1, -1);

	SetSizeHints(400, 300);

#ifdef __WXGTK__
	wxFont f = GetFont();
	f.SetPointSize(f.GetPointSize() - 2);
	SetFont(f);
#endif


}

void XYGraphWidget::UpdateInfoWidget() {
	m_point_info->SetPointInfo(m_graph, m_cursor_index);
}

void XYGraphWidget::OnKeyboardTimer(wxTimerEvent &event) {
	switch (m_keyboard_action) {
	case CURSOR_LEFT:
	case CURSOR_LEFT_KB:
		MoveCursorLeft();
		break;
	case CURSOR_RIGHT:
	case CURSOR_RIGHT_KB:
		MoveCursorRight();
		break;
	case CURSOR_UP:
	case CURSOR_UP_KB:
		MoveCursorUp();
		break;
	case CURSOR_DOWN:
	case CURSOR_DOWN_KB:
		MoveCursorDown();
		break;
	default:
		break;
	}

	m_keyboard_timer->Start(CursorMovementSpeed[PERIOD_T_MONTH], wxTIMER_ONE_SHOT);
}

void XYGraphWidget::SetKeyboardAction(ActionKeyboardType type) {
	switch (type) {
		case NONE:
			m_keyboard_action = NONE;
			m_keyboard_timer->Stop();
			break;
		default:
			m_keyboard_action = type;
			m_keyboard_timer->Start(CursorMovementSpeed[PERIOD_T_DAY], wxTIMER_ONE_SHOT);
			break;
	}
}

void XYGraphWidget::MoveCursor(MoveDirection dir) {
	if (m_cursor_index == -1)
		return;

	int w,h;
	GetSize(&w, &h);
	w -= left_margin + right_margin;
	h -= bottom_margin + top_margin;
		

	int x, y;
	m_painter.GetPointPosition(m_cursor_index, &x, &y);

	x -= left_margin;
	y -= top_margin;

	KDTree::Range range;

	switch (dir) {
		case UP:
			range = KDTree::Range(wxPoint(0, 0), wxPoint(w, y - 1));
			break;
		case DOWN:
			range = KDTree::Range(wxPoint(0, y + 1), wxPoint(w, h));
			break;
		case LEFT:
			range = KDTree::Range(wxPoint(0, 0), wxPoint(x - 1, h));
			break;
		case RIGHT:
			range = KDTree::Range(wxPoint(x + 1, 0), wxPoint(w, h));
			break;
	}

	KDTree::KDTreeEntry result;
	double ret = m_kdtree->GetNearest(wxPoint(x, y), result, range, INFINITY);
	if (!std::isfinite(ret))
		return;
	
	SetCursorIndex(result.i);

	UpdateInfoWidget();
}

void XYGraphWidget::SetCursorIndex(int index) {
	if (m_cursor_index == index)
		return;

	int w, h;
	GetSize(&w, &h);

	int x, y;
	if (m_cursor_index != -1) {
		m_painter.GetPointPosition(m_cursor_index, &x, &y);
		RefreshRect(wxRect(x - 1, 0, 3, h));
		RefreshRect(wxRect(0, y - 1, w, 3));
	}

	if (index != -1) {
		m_painter.GetPointPosition(index, &x, &y);
		RefreshRect(wxRect(x - 1, 0, 3, h));
		RefreshRect(wxRect(0, y - 1, w, 3));
	}

	m_cursor_index = index;
}

void XYGraphWidget::MoveCursorRight() {
	MoveCursor(RIGHT);
}

void XYGraphWidget::MoveCursorLeft() {
	MoveCursor(LEFT);
}

void XYGraphWidget::MoveCursorUp() {
	MoveCursor(UP);
}

void XYGraphWidget::MoveCursorDown() {
	MoveCursor(DOWN);
}

void XYGraphWidget::StartDrawing() {
	assert(m_graph);

	if (m_graph->m_visible_points.size()) {
		m_cursor_index = 0;
		UpdateInfoWidget();
	}

	m_drawing = true;

	int w, h;
	GetSize(&w, &h);
	if (w < 0 || h < 0)
		return;

	RepaintBitmap();

	UpdateTree();

	UpdateStats();

	Refresh();
}

void XYGraphWidget::StopDrawing() {
	m_drawing = false;
	RepaintBitmap();
	Refresh();
}

void XYGraphWidget::UpdateTree() {
	if (m_graph == NULL)
		return;

	delete m_kdtree;

	std::deque<KDTree::KDTreeEntry*> points;

	for (size_t i = 0; i < m_graph->m_visible_points.size(); ++i) {
		int x, y;
		m_painter.GetPointPosition(i, &x, &y);
		x -= left_margin;
		y -= top_margin;

		KDTree::KDTreeEntry *e = new KDTree::KDTreeEntry();

		e->p = wxPoint(x, y);
		e->i = i;

		points.push_back(e);
	}

	int w,h;
	GetSize(&w, &h);
	w -= left_margin + right_margin;
	h -= bottom_margin + top_margin;
	
	m_kdtree = KDTree::Construct(points, 0, w, 0, h);

	for (size_t i = 0; i < points.size(); ++i) 
		delete points[i];

}


void XYGraphWidget::SetGraph(XYGraph *graph) {
	delete m_graph;

	m_graph = graph;

	std::deque<XYPoint>& vals = m_graph->m_points_values;

	std::sort(vals.begin(), vals.end());

	for (size_t i = 0; i < vals.size(); i++)
		graph->m_visible_points.push_back(i);

	m_painter.SetGraph(m_graph);

	EnableZoomOut(false);

	SetFocus();
}

bool XYGraphWidget::UpdateStats() {
	int w, h;
	GetSize(&w, &h);

	w -= left_margin + right_margin;
	h -= top_margin + bottom_margin;

	KDTree::Range r;

	r = KDTree::Range(wxPoint(0,0), wxPoint(w, h));

	StatsCollector col(m_graph);

	m_kdtree->VisitInRange(r, col);

	bool ret = false;
	if (col.GetDataCount()) {
		col.UpdateGraphStats();
		m_point_info->SetGraphInfo(m_graph);

		ret = true;
	}

	return ret;

}

void XYGraphWidget::DrawRectangle(wxDC *dc) {
	if (m_current_rect.x == -1)
		return;

	int x = m_current_rect.x;
	int width = m_current_rect.width;
	if (width < 0) {
		x += width;
		width *= -1;
	}

	int y = m_current_rect.y;
	int height = m_current_rect.height;
	if (height < 0) {
		y += height;
		height *= -1;
	}

	dc->SetBrush(*wxTRANSPARENT_BRUSH);
	dc->SetPen(wxPen(*wxWHITE));
	dc->DrawRectangle(x, y, width, height);

}

void XYGraphWidget::OnPaint(wxPaintEvent &event) {
	wxBufferedPaintDC odc(this);
	wxDC* dc = &odc;

	dc->SetFont(GetFont());

	dc->SetBackground(*wxBLACK_BRUSH);
	dc->Clear();

	if (m_drawing == false)
		return;

	wxRegion region = GetUpdateRegion();

	wxRegionIterator i(region);

	while (i) {
		wxLogInfo(_T("Repainting x:%d y:%d w:%d h:%d"), i.GetX(), i.GetY(), i.GetW(), i.GetH());
		dc->Blit(i.GetX(), i.GetY(), i.GetW(), i.GetH(), &m_bdc, i.GetX(), i.GetY(), wxCOPY);
		++i;
	}

	DrawRectangle(dc);

	DrawCursor(dc);

}

void XYGraphWidget::DrawCursor(wxDC *dc) {
	if (m_cursor_index < 0)
		return;

	dc->SetPen(*wxWHITE_PEN);
	
	int x,y;
	m_painter.GetPointPosition(m_cursor_index, &x, &y);
	dc->CrossHair(x, y);
}

void XYGraphWidget::DrawAxes(wxDC *dc) {

	dc->SetPen(*wxWHITE_PEN);
	dc->SetTextForeground(*wxWHITE);

	m_painter.DrawXAxis(dc, 7, 5, 2);
	m_painter.DrawYAxis(dc, 7, 5, 2);
	m_painter.DrawXUnit(dc, 6, 1);
	m_painter.DrawYUnit(dc, 1, 8);

}

void XYGraphWidget::RepaintBitmap() {
	int bw = -1, bh = -1; 
	int w, h;

	GetSize(&w, &h);

	if (m_bmp) {
		bw = m_bmp->GetWidth();
		bh = m_bmp->GetHeight();
	}

	if (bw != w || bh != h) {
		m_bdc.SelectObject(wxNullBitmap);
		
		delete m_bmp;
		m_bmp = new wxBitmap(w, h);

		m_bdc.SelectObject(*m_bmp);

		m_bdc.SetFont(GetFont());

		m_painter.SetSize(w, h);
	}

	m_bdc.SetBackground(*wxBLACK_BRUSH);
	m_bdc.Clear();
	if (m_drawing == false)
		return;

	m_bdc.SetBrush(wxBrush(wxColour(64, 64, 64)));
	m_bdc.SetPen(*wxTRANSPARENT_PEN);
	m_bdc.SetBrush(*wxTRANSPARENT_BRUSH);

	DrawAxes(&m_bdc);

	m_painter.DrawGraph(&m_bdc);

	m_painter.DrawDrawsInfo(&m_bdc, 10, 2);

}

void XYGraphWidget::OnSize(wxSizeEvent &event) {
	if (m_drawing == false)
		return;

	int w,h;
	GetSize(&w, &h);

	RepaintBitmap();

	UpdateTree();

	if (m_current_size.IsFullySpecified() == false) 
		UpdateStats();

	m_current_size = wxSize(w, h);

	Refresh();

}

void XYGraphWidget::OnLeftMouseDown(wxMouseEvent &event) {
	int x,y,w,h;	
	event.GetPosition(&x, &y);
	GetSize(&w, &h);
	w -= left_margin + right_margin;
	h -= top_margin + bottom_margin;

	x -= left_margin;
	y -= top_margin;

	KDTree::KDTreeEntry result;
	KDTree::Range range(wxPoint(0, 0), wxPoint(w, h));

	double ret = m_kdtree->GetNearest(wxPoint(x, y), result, range, INFINITY);
	if (!std::isfinite(ret))
		return;
	
	SetCursorIndex(result.i);

	UpdateInfoWidget();
	SetFocus();

}

void XYGraphWidget::OnRightMouseDown(wxMouseEvent &event) {
	int x, y;

	event.GetPosition(&x, &y);
	m_current_rect.x = x;
	m_current_rect.y = y;
	m_current_rect.width = 0;
	m_current_rect.height = 0;
	SetFocus();

}

void XYGraphWidget::OnMouseMove(wxMouseEvent &event) {
	if (m_current_rect.x == -1)
		return;
#define REFRESH_RECT(rect) { \
	int x1 = rect.x;				\
	int y1 = rect.y;				\
	int x2 = x1 + rect.width;			\
	int y2 = y1 + rect.height;			\
	if (x1 > x2) {					\
		int t = x2;				\
		x2 = x1;				\
		x1 = t;					\
	}						\
							\
	if (y1 > y2) {					\
		int t = y2;				\
		y2 = y1;				\
		y1 = t;					\
	}						\
							\
	RefreshRect(wxRect(x1, y1, 1, y2 - y1 + 1));	\
	RefreshRect(wxRect(x1, y1, x2 - x1 + 1, 1));	\
	RefreshRect(wxRect(x1, y2 - 1, x2 - x1, 1));	\
	RefreshRect(wxRect(x2 - 1, y1, 1, y2 - y1));	\
}


	if (m_current_rect.width) 
		REFRESH_RECT(m_current_rect);

	int x, y;
	event.GetPosition(&x, &y);

	m_current_rect.width = x - m_current_rect.x;
	m_current_rect.height = y - m_current_rect.y;

	if (m_current_rect.width) 
		REFRESH_RECT(m_current_rect);

	SetFocus();
}


void XYGraphWidget::OnRightMouseUp(wxMouseEvent &event) {
	double nxmin, nxmax, nymin, nymax;

	wxRect nrec;
	if (m_current_rect.width == 0) {
		m_current_rect.x = -1;
		ZoomOut();
		return;
	}

	if (m_current_rect.width < 0) {
		nrec.x = m_current_rect.x + m_current_rect.width;
		nrec.width = - m_current_rect.width;
	} else {
		nrec.x = m_current_rect.x;
		nrec.width = m_current_rect.width;
	}

	if (m_current_rect.height < 0) {
		nrec.y = m_current_rect.y + m_current_rect.height;
		nrec.height = - m_current_rect.height;
	} else {
		nrec.y = m_current_rect.y;
		nrec.height = m_current_rect.height;
	}

	int ww, wh;
	GetSize(&ww, &wh);

	ww -= left_margin + right_margin;
	wh -= top_margin + bottom_margin;

	nxmin = (nrec.x - left_margin) * (m_graph->m_dmax[0] - m_graph->m_dmin[0]) / ww + m_graph->m_dmin[0];
	nxmax = (nrec.x + nrec.width - left_margin) * (m_graph->m_dmax[0] - m_graph->m_dmin[0]) / ww + m_graph->m_dmin[0];

	nymin = (wh - (nrec.y + nrec.height - top_margin)) * (m_graph->m_dmax[1] - m_graph->m_dmin[1]) / wh + m_graph->m_dmin[1];
	nymax = (wh - (nrec.y - top_margin)) * (m_graph->m_dmax[1] - m_graph->m_dmin[1]) / wh + m_graph->m_dmin[1];

	std::deque<size_t> points;
	for (size_t i = 0; i < m_graph->m_points_values.size(); i++) {
		XYPoint &p = m_graph->m_points_values[i];
		double& x = p.first[0];
		double& y = p.first[1];
		if (x >= nxmin && x <= nxmax && y >= nymin && y <= nymax)
			points.push_back(i);
	}

	if (points.size()) {
		m_cursor_index = 0;
		m_graph->m_visible_points = points;
		m_graph->m_zoom_history.push_back(std::make_pair(wxRealPoint(m_graph->m_dmin[0], m_graph->m_dmin[1]), wxRealPoint(m_graph->m_dmax[0], m_graph->m_dmax[1]))); 

		m_graph->m_dmin[0] = nxmin;
		m_graph->m_dmin[1] = nymin;
		m_graph->m_dmax[0] = nxmax;
		m_graph->m_dmax[1] = nymax;
		UpdateTree();
		if (UpdateStats()) 
			RepaintBitmap();
		EnableZoomOut(true);
	}

	m_current_rect.x = -1;
	Refresh();
	SetFocus();
}

void XYGraphWidget::ZoomOut() {
	if (m_graph->m_zoom_history.size() == 0)
		return;

	std::pair<wxRealPoint, wxRealPoint>& pp = m_graph->m_zoom_history.back();
	m_graph->m_dmin[0] = pp.first.x;
	m_graph->m_dmin[1] = pp.first.y;
	m_graph->m_dmax[0] = pp.second.x;
	m_graph->m_dmax[1] = pp.second.y;
	m_graph->m_zoom_history.pop_back();

	EnableZoomOut(m_graph->m_zoom_history.size() > 0);

	std::deque<size_t> points;
	for (size_t i = 0; i < m_graph->m_points_values.size(); i++) {
		XYPoint &p = m_graph->m_points_values[i];
		double& x = p.first[0];
		double& y = p.first[1];
		if (x >= m_graph->m_dmin[0] && x <= m_graph->m_dmax[0] && y >= m_graph->m_dmin[1] && y <= m_graph->m_dmax[1])
			points.push_back(i);
	}

	m_cursor_index = 0;
	m_graph->m_visible_points = points;

	UpdateTree();
	UpdateStats();
	RepaintBitmap();
	Refresh();

	SetFocus();
}

void XYGraphWidget::EnableZoomOut(bool enable) {
	wxWindow *parent = GetParent();
	while (parent && !parent->IsTopLevel())
		parent = parent->GetParent();

	wxFrame *frame = dynamic_cast<wxFrame*>(parent);
	if (!parent)
		return;

	frame->GetMenuBar()->Enable(XY_ZOOM_OUT, enable);
}


void XYGraphWidget::Print(bool preview) {
	if (m_graph == NULL)
		return;

	if (preview) 
		Print::DoXYPrintPreview(m_graph);
	else
		Print::DoXYPrint(this, m_graph);
}

void XYGraphWidget::OnEraseBackground(wxEraseEvent & WXUNUSED(event))
{
}


XYGraphWidget::~XYGraphWidget() {
	delete m_graph;
	delete m_kdtree;
}

StatsCollector::StatsCollector(XYGraph *graph)
	: m_graph(graph)
{}

int StatsCollector::GetDataCount() {
	return xvals.size();
}

void StatsCollector::Visit(const KDTree::KDTreeEntry &e) {
	double x = m_graph->ViewPoint(e.i).first[0];
	double y = m_graph->ViewPoint(e.i).first[1];

	//Bacouse points of the same values are compressed to one with many dates
	int how_many = m_graph->ViewPoint(e.i).second.size();

	assert(how_many > 0);

	while(how_many--) {
		xvals.push_back(x);
		yvals.push_back(y);
	}
}

double StatsCollector::ComputeCorrelation(double xavg, double yavg, std::vector<double> &xvals, std::vector<double> &yvals, double &x_standard_deviation, double &y_standard_deviation) {
	
	x_standard_deviation = 0;
	y_standard_deviation = 0;

	int n = xvals.size();
	double mulsum = 0;

	if(n > 1) {
		for(int i = 0; i < n; i++) {
			x_standard_deviation += pow(xvals[i] - xavg, 2);
			y_standard_deviation += pow(yvals[i] - yavg, 2);
			mulsum += xvals[i] * yvals[i];
		}
	
		x_standard_deviation /= n - 1;
		y_standard_deviation /= n - 1;
		x_standard_deviation = sqrt(x_standard_deviation);
		y_standard_deviation = sqrt(y_standard_deviation);

		if (m_graph->m_standard_deviation[0] == 0 || m_graph->m_standard_deviation[1] == 0)
			return 0;	
		else
			return (mulsum - n * xavg * yavg) / ( (n - 1) * x_standard_deviation * y_standard_deviation);
	} else
		return 0;
	
}

void StatsCollector::UpdateGraphStats() {

	assert(xvals.size() == yvals.size());
	assert(xvals.size() > 0);
	
	std::vector<double> tmp_x_vals = xvals;
	std::vector<double> tmp_y_vals = yvals;
	
	std::sort(tmp_x_vals.begin(), tmp_x_vals.end());
	std::sort(tmp_y_vals.begin(), tmp_y_vals.end());

	m_graph->m_min[0] = tmp_x_vals[0];
	m_graph->m_max[0] = tmp_x_vals[tmp_x_vals.size()-1];

	m_graph->m_min[1] = tmp_y_vals[0];
	m_graph->m_max[1] = tmp_y_vals[tmp_y_vals.size()-1];

	std::map<double, std::pair<int, int> > xrankmap;
	std::map<double, std::pair<int, int> > yrankmap;

	double xsum = 0;
	double ysum = 0;

	std::vector<double>::iterator x_iter = tmp_x_vals.begin();
	std::vector<double>::iterator y_iter = tmp_y_vals.begin();

	int idx = 0;
	while (x_iter != tmp_x_vals.end()) {
		xsum += *x_iter;
		ysum += *y_iter;

		xrankmap[*x_iter].first += idx;
		xrankmap[*x_iter].second++;
		
		yrankmap[*y_iter].first += idx;
		yrankmap[*y_iter].second++;

		x_iter++;
		y_iter++;
		idx++;
	}
	
	m_graph->m_avg[0] = xsum / tmp_x_vals.size();
	m_graph->m_avg[1] = ysum / tmp_y_vals.size();

	m_graph->m_xy_correlation = ComputeCorrelation(m_graph->m_avg[0], m_graph->m_avg[1], xvals, yvals, m_graph->m_standard_deviation[0], m_graph->m_standard_deviation[1]);

	xsum = 0;
	ysum = 0;
	for(unsigned int i = 0; i < xvals.size(); i++) {
		tmp_x_vals[i] = (double) xrankmap[xvals[i]].first / (double) xrankmap[xvals[i]].second;
		tmp_y_vals[i] = (double) yrankmap[yvals[i]].first / (double) yrankmap[yvals[i]].second;
		xsum += tmp_x_vals[i];
		ysum += tmp_y_vals[i];
	}
	double a, b;
	m_graph->m_xy_rank_correlation = ComputeCorrelation(xsum / xvals.size(), ysum / yvals.size(), tmp_x_vals, tmp_y_vals, a, b);

}

BEGIN_EVENT_TABLE(XYGraphWidget, wxWindow)
	EVT_ERASE_BACKGROUND(XYGraphWidget::OnEraseBackground)
	EVT_PAINT(XYGraphWidget::OnPaint)
	EVT_SIZE(XYGraphWidget::OnSize)
	EVT_TIMER(wxID_ANY, XYGraphWidget::OnKeyboardTimer)
	EVT_LEFT_DOWN(XYGraphWidget::OnLeftMouseDown)
	EVT_RIGHT_DOWN(XYGraphWidget::OnRightMouseDown)
	EVT_RIGHT_UP(XYGraphWidget::OnRightMouseUp)
	EVT_MOTION(XYGraphWidget::OnMouseMove)
END_EVENT_TABLE()

enum GridInfo{ Xval=0, Yval, Xmin, Ymin, Xavg, Yavg, Xmax, Ymax, Xsigma, Ysigma, XYcorrelation, XYrankCorrelation, XYstart, XYend, GridInfoGuard };
	
XYPointInfo::XYPointInfo(wxWindow *parent, ConfigManager *cfg_manager, FrameManager *frame_manager, wxString default_prefix) : 
		wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS), m_config_manager(cfg_manager), m_frame_manager(frame_manager), m_default_prefix(default_prefix)
{

	m_dx = m_dy = NULL;

	wxBoxSizer *point_date_sizer_v = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer *point_date_sizer_h = new wxBoxSizer(wxHORIZONTAL);

	point_date_sizer_h->Add(new wxStaticText(this, wxID_ANY, _("Points:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT), 0);
	m_points_no = new wxStaticText(this, wxID_ANY, _T("(0)"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
	point_date_sizer_h->Add(m_points_no, 0, wxALIGN_CENTER_VERTICAL | wxALL, 0);

	point_date_sizer_v->Add(point_date_sizer_h, 0, wxALIGN_LEFT | wxALL, 2);

	m_point_dates_choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(120, -1), 0);
	point_date_sizer_v->Add(m_point_dates_choice, 1, wxEXPAND | wxALL, 2);

	m_goto_button = new wxButton(this, XY_GOTO_GRAPH_BUTTON, _("Go to graph"), wxDefaultPosition, wxSize(120, -1));
	point_date_sizer_v->Add(m_goto_button, 1, wxEXPAND | wxALL, 2);

	m_info_grid = new wxGrid(this, wxID_ANY);

	m_info_grid->SetColLabelSize(0);
	m_info_grid->SetRowLabelSize(150);
	m_info_grid->SetRowLabelAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
	
	m_info_grid->CreateGrid(GridInfoGuard, 1);
	m_info_grid->EnableDragGridSize(false);
	m_info_grid->EnableEditing(false);
	m_info_grid->EnableGridLines(false);
	m_info_grid->SetCellBackgroundColour(m_info_grid->GetLabelBackgroundColour());

	m_info_grid->SetRowLabelValue(Xval, _("X value"));
	m_info_grid->SetRowLabelValue(Yval, _("Y value"));
	m_info_grid->SetRowLabelValue(Xmin, _("X min"));
	m_info_grid->SetRowLabelValue(Ymin, _("Y min"));
	m_info_grid->SetRowLabelValue(Xavg, _("X avg"));
	m_info_grid->SetRowLabelValue(Yavg, _("Y avg"));
	m_info_grid->SetRowLabelValue(Xmax, _("X max"));
	m_info_grid->SetRowLabelValue(Ymax, _("Y max"));
	m_info_grid->SetRowLabelValue(Xsigma, _("X sd"));
	m_info_grid->SetRowLabelValue(Ysigma, _("Y sd"));
	m_info_grid->SetRowLabelValue(XYcorrelation, _("X/Y Correlation"));
	m_info_grid->SetRowLabelValue(XYrankCorrelation, _("X/Y Rank correl."));
	m_info_grid->SetRowLabelValue(XYstart, _("From"));
	m_info_grid->SetRowLabelValue(XYend, _("To"));

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	sizer->Add(m_info_grid, 1, wxEXPAND | wxALL, 0);
	sizer->Add(point_date_sizer_v, 0, wxEXPAND | wxALL, 0);

	SetSizer(sizer);

	sizer->SetSizeHints(this);

}

void XYPointInfo::SetGraphInfo(XYGraph *graph) {

	m_dx = graph->m_di[0];
	m_dy = graph->m_di[1];

	m_px = m_dx->GetPrec();
	m_py = m_dy->GetPrec();

	m_info_grid->SetCellValue(Xmin, 0, m_dx->GetValueStr(graph->m_min[0], _T("")));
	m_info_grid->SetCellValue(Xavg, 0, m_dx->GetValueStr(graph->m_avg[0], _T("")));
	m_info_grid->SetCellValue(Xmax, 0, m_dx->GetValueStr(graph->m_max[0], _T("")));
	m_info_grid->SetCellValue(Xsigma, 0, m_dx->GetValueStr(graph->m_standard_deviation[0]));
	m_info_grid->SetCellValue(Ymin, 0, m_dy->GetValueStr(graph->m_min[1], _T("")));
	m_info_grid->SetCellValue(Yavg, 0, m_dy->GetValueStr(graph->m_avg[1], _T("")));
	m_info_grid->SetCellValue(Ymax, 0, m_dy->GetValueStr(graph->m_max[1], _T("")));
	m_info_grid->SetCellValue(Ysigma, 0, m_dy->GetValueStr(graph->m_standard_deviation[1]));

	m_info_grid->SetCellValue(XYstart, 0, XYFormatTime(graph->m_start, graph->m_period));
	m_info_grid->SetCellValue(XYend, 0, XYFormatTime(graph->m_end, graph->m_period));
	
	
	m_period = graph->m_period;

	std::wstringstream wss;
	wss.precision(2);
	wss << graph->m_xy_correlation;
	m_info_grid->SetCellValue(XYcorrelation, 0, wss.str());

	wss.str(std::wstring());
	wss << graph->m_xy_rank_correlation;
	m_info_grid->SetCellValue(XYrankCorrelation, 0, wss.str());

	m_info_grid->SetCellBackgroundColour(Xval, 0, m_dx->GetDrawColor());
	m_info_grid->SetCellBackgroundColour(Yval, 0, m_dy->GetDrawColor());
	m_info_grid->AutoSize();

}


void XYPointInfo::SetPointInfo(XYGraph *graph, int point_index) {

	XYPoint &point = graph->ViewPoint(point_index);

	double vx = point.first[0];
	double vy = point.first[1];

	wxString str1 = m_dx->GetValueStr(vx, _T(" -- "));
	str1 +=	wxString(_T(" ")) + m_dx->GetUnit();

	m_info_grid->SetCellValue(Xval, 0, str1);

	wxString str2 = m_dy->GetValueStr(vy, _T(" -- "));

	str2 +=	wxString(_T(" ")) + m_dy->GetUnit();

	m_info_grid->SetCellValue(Yval, 0, str2);

	m_info_grid->AutoSize();

	m_point_dates_choice->Freeze();
	m_point_dates_choice->Clear();

	if(graph->point2group.find(point_index) != graph->point2group.end()) {
		int group = graph->point2group[point_index];

		std::list<int>::iterator iter;
		for(iter = graph->points_groups[group].begin(); iter != graph->points_groups[group].end(); iter++) {

			XYPoint &same_pixel_point = graph->ViewPoint(*iter);
			std::vector<DTime>::iterator tmp;
			for (tmp = same_pixel_point.second.begin(); tmp != same_pixel_point.second.end(); tmp++) {
				m_point_dates.push_back(*tmp);
			}

		}

		std::sort(m_point_dates.begin(), m_point_dates.end());

	} else
		m_point_dates = point.second;

	for(unsigned int i = 0; i < m_point_dates.size(); i++)
		m_point_dates_choice->Append( XYFormatTime(m_point_dates[i].GetTime() , m_period) );

	if(m_point_dates.size() > 0)
		m_point_dates_choice->Select(0);

	m_points_no->SetLabel(wxString::Format(_T("(%d)"), m_point_dates.size()));

	m_point_dates_choice->Thaw();

}

void XYPointInfo::OnGoToGraph(wxCommandEvent &event) {

	DefinedDrawSet *dset = new DefinedDrawSet(m_config_manager->GetDefinedDrawsSets());
	dset->SetTemporary(true);
	dset->SetName(wxString(_T("X/Y")));
	dset->Add(std::vector<DrawInfo*>(1, m_dx));
	dset->Add(std::vector<DrawInfo*>(1, m_dy));

	m_config_manager->GetDefinedDrawsSets()->AddSet(dset);

	time_t t = m_point_dates[m_point_dates_choice->GetSelection()].GetTime().GetTicks();

	m_frame_manager->CreateFrame(m_dx->GetBasePrefix(), dset->GetName(), m_period, t, wxDefaultSize, wxDefaultPosition);
}

BEGIN_EVENT_TABLE(XYPointInfo, wxPanel)
	EVT_BUTTON(XY_GOTO_GRAPH_BUTTON, XYPointInfo::OnGoToGraph)
END_EVENT_TABLE()

KDTree::KDTree(const KDTreeEntry &entry, KDTree *left, KDTree *right, const Range &range, Pivot pivot) :
	m_entry(entry), m_left(left), m_right(right), m_range(range), m_pivot(pivot)
{ }

bool KDTree::XCoordCmp(const KDTreeEntry* p1, const KDTreeEntry* p2) {
	return p1->p.x < p2->p.x;
}

bool KDTree::YCoordCmp(const KDTreeEntry* p1, const KDTreeEntry* p2) {
	return p1->p.y < p2->p.y;
}

KDTree* KDTree::Construct(std::deque<KDTreeEntry*> vals, const Range& range, Pivot pivot) {
	if (vals.size() == 0)
		return NULL;
	if (vals.size() == 1)
		return new KDTree(*vals[0], NULL, NULL, range, pivot);

	Pivot np;

	Range lr = range;
	Range rr = range;

	switch (pivot) {
		case XPIVOT:
			std::sort(vals.begin(), vals.end(), XCoordCmp);

			np = YPIVOT;
			break;
		case YPIVOT:
			std::sort(vals.begin(), vals.end(), YCoordCmp);

			np = XPIVOT;
			break;
		default:
			assert(false);
	}

	size_t m = vals.size() / 2;
	const KDTreeEntry& ip = *vals[m];

	switch (pivot) {
		case XPIVOT:
			lr.second.x = ip.p.x;
			rr.first.x = ip.p.x;

			break;
		case YPIVOT:
			lr.second.y = ip.p.y;
			rr.first.y = ip.p.y;
			break;
	}

	std::deque<KDTreeEntry*> lv;
	for (size_t i = 0; i < m; ++i) 
		lv.push_back(vals[i]);

	std::deque<KDTreeEntry*> rv;
	for (size_t i = m + 1; i < vals.size(); ++i) 
		rv.push_back(vals[i]);

	KDTree* lt = Construct(lv, lr, np);
	KDTree* rt = Construct(rv, rr, np);
	
	return new KDTree(ip, lt, rt, range, pivot);
}


double KDTree::GetNearest(const wxPoint &p, KDTreeEntry &r, const KDTree::Range &ran, const double& max) {

	Range range = m_range.Intersect(ran);
	if (range.IsEmpty())
		return INFINITY;
	
	double d = range.Distance(p);
	if (d >= max)
		return INFINITY;

	KDTree* _near = NULL;
	KDTree* _far = NULL;

	Range nran;
	Range fran;

	switch (m_pivot) {
		case XPIVOT:
			if (p.x <= m_entry.p.x) {
				_near = m_left;
				_far = m_right;
				range.SplitV(m_entry.p.x, nran, fran);
			} else {
				_far = m_left;
				_near = m_right;
				range.SplitV(m_entry.p.x, fran, nran);

			}
			break;
		case YPIVOT:
			if (p.y <= m_entry.p.y) {
				_near = m_left;
				_far = m_right;
				range.SplitH(m_entry.p.y, nran, fran);
			} else {
				_far = m_left;
				_near = m_right;
				range.SplitH(m_entry.p.y, fran, nran);

			}
			break;
	}

	double ds = INFINITY;

	if (range.IsContained(m_entry.p)) {
		ds = (m_entry.p.x - p.x) * (m_entry.p.x - p.x) +
				(m_entry.p.y - p.y) * (m_entry.p.y - p.y);

		r = m_entry;
	}

	if (_near) {
		double nds;
		KDTreeEntry np;
		nds = _near->GetNearest(p, np, nran, ds);
		if (nds < ds) {
			r = np;
			ds = nds;
		}
	}

	if (_far && !fran.IsEmpty()) {
		
		double pd = fran.Distance(p);

		if (pd < ds) {
			KDTreeEntry fr;
			double dd = _far->GetNearest(p, fr, fran, ds);

			if (dd < ds) {
				r = fr;
				ds = dd;
			}
		}
	}

	return ds;

}

void KDTree::VisitInRange(const Range &ran, Visitor &visitor) {
	Range range = m_range.Intersect(ran);
	if (range.IsEmpty())
		return;

	Range lran;
	Range rran;

	switch (m_pivot) {
		case XPIVOT:
			range.SplitV(m_entry.p.x, lran, rran);
			break;
		case YPIVOT:
			range.SplitH(m_entry.p.y, lran, rran);
			break;
	}

	if (range.IsContained(m_entry.p)) 
		visitor.Visit(m_entry);

	if (m_left) 
		m_left->VisitInRange(lran, visitor);

	if (m_right) 
		m_right->VisitInRange(rran, visitor);
		
}

KDTree* KDTree::Construct(std::deque<KDTreeEntry*>& vals, int xmin, int xmax, int ymin, int ymax) {
	Range r(wxPoint(xmin,ymin), wxPoint(xmax, ymax));

	KDTree *ret = Construct(vals, r, XPIVOT);

	return ret;
}

KDTree::~KDTree() {
	delete m_left;
	delete m_right;
}

KDTree::Visitor::~Visitor() {}

KDTree::Range::Range() : first(0,0), second(-1, -1) {}

KDTree::Range::Range(const wxPoint &s, const wxPoint &e) : first(s), second(e) {}

KDTree::Range KDTree::Range::Intersect(const Range &range) const {
	return Range(wxPoint(wxMax(first.x, range.first.x), 
				wxMax(first.y, range.first.y)),
			wxPoint(wxMin(second.x, range.second.x), 
				wxMin(second.y, range.second.y)));
}

void KDTree::Range::SplitH(int y, Range &up, Range &down) const {
	if (y < first.y) {
		up = Range();
		down = *this;
		return;
	}

	if (y > second.y) {
		up = *this;
		down = Range();
		return;
	}

	up = Range(wxPoint(first.x, first.y), wxPoint(second.x, y));
	down = Range(wxPoint(first.x, y), wxPoint(second.x, second.y));
}

void KDTree::Range::SplitV(int x, Range &left, Range &right) const {
	if (x < first.x) {
		left = Range();
		right = *this;
		return;
	}

	if (x > second.x) {
		left = *this;
		right = Range();
		return;
	}

	left = Range(wxPoint(first.x, first.y), wxPoint(x, second.y));
	right = Range(wxPoint(x, first.y), wxPoint(second.x, second.y));
}

	
bool KDTree::Range::IsEmpty() const {
	return first.x > second.x || first.y > second.y;
}

double KDTree::Range::Distance(const wxPoint &p) const {
	if (IsEmpty())
		return nan("");

	int rxs = first.x;
	int rys = first.y;

	int rxe = second.x;
	int rye = second.y;

	double d;

	if (p.x < rxs) 
		d = (p.x - rxs) * (p.x - rxs);
	else if (p.x > rxe)
		d = (p.x - rxe) * (p.x - rxe);
	else
		d = 0;

	if (p.y < rys) 
		d += (p.y - rys) * (p.y - rys);
	else if (p.y > rye)
		d += (p.y - rye) * (p.y - rye);

	return d;
}

bool KDTree::Range::IsContained(const wxPoint &point) const {
	return first.x <= point.x && point.x <= second.x 
		&& first.y <= point.y && point.y <= second.y ;
}

XYGraphPainter::XYGraphPainter(int left_margin, int right_margin, int top_margin, int bottom_margin) :
		m_left_margin(left_margin), 
		m_right_margin(right_margin), 
		m_top_margin(top_margin), 
		m_bottom_margin(bottom_margin) {

	m_width = -1;
	m_height = -1;

	m_graph = NULL;
};

int XYGraphPainter::FindNumberOfSlices(double max, double min, DrawInfo *di) {

	int prec = di->GetPrec();
	for (int i = 0; i < prec; ++i) {
		max *= 10;
		min *= 10;
	}

	double dif = max - min;

	if (dif == 0) 
		return 2;

	while (dif > 100) 
		dif /= 10;
	while (dif < 10)
		dif *= 10;

	int ret = 0;

	for (ret = 20; ret > 10; ret--) {
		int d = int(dif / ret) % 10;
		if ((d == 0) || (d == 5))
			break;
	}

	return ret;
}

void XYGraphPainter::SetSize(int w, int h) {
	m_width = w;
	m_height = h;
}

void XYGraphPainter::SetGraph(XYGraph *graph) {
	m_graph = graph;
}

void XYGraphPainter::GetPointPosition(int i, int *x, int *y)  {
	int w = m_width, h = m_height;

	w -= m_left_margin + m_right_margin;
	h -= m_top_margin + m_bottom_margin;

	double xmax = m_graph->m_dmax[0];
	double xmin = m_graph->m_dmin[0];
	double xdif = xmax - xmin;
	double& vx = m_graph->ViewPoint(i).first[0];
	*x = int(m_left_margin + (vx - xmin) * w / xdif);

	double ymax = m_graph->m_dmax[1];
	double ymin = m_graph->m_dmin[1];
	double ydif = ymax - ymin;
	double& vy = m_graph->ViewPoint(i).first[1];
	*y = int(m_top_margin + (ymax - vy) * h / ydif);

}

void XYGraphPainter::DrawXAxis(wxDC *dc, int arrow_width, int arrow_height, int mark_height) {

	int w = m_width, h = m_height;

	dc->DrawLine(m_left_margin, h - m_bottom_margin, w, h - m_bottom_margin);
	dc->DrawLine(w - arrow_width, h - m_bottom_margin - arrow_height, w, h - m_bottom_margin);
	dc->DrawLine(w - arrow_width, h - m_bottom_margin + arrow_height, w, h - m_bottom_margin);

	int rw = w - (m_left_margin + m_right_margin);

	DrawInfo *dx = m_graph->m_di[0];

	double dif = m_graph->m_dmax[0] - m_graph->m_dmin[0];

	int slices = FindNumberOfSlices(m_graph->m_dmax[0],
			m_graph->m_dmin[0],
			m_graph->m_di[0]);

	assert(slices > 1);

	for (int i = 0; i <= slices; ++i) {
		double val = m_graph->m_dmin[0] + dif * i / slices;

		int x = int(m_left_margin + (val - m_graph->m_dmin[0]) * rw / dif);

		dc->DrawLine(x, h - m_bottom_margin + mark_height, x, h - m_bottom_margin);

		wxString _val = dx->GetValueStr(val, _T(""));

		int tw, th;
		dc->GetTextExtent(_val, &tw, &th);
		dc->DrawText(_val, x, h - m_bottom_margin + dc->GetPen().GetWidth());

	}

}

void XYGraphPainter::DrawYAxis(wxDC *dc, int arrow_height, int arrow_width, int mark_width) {
	int h = m_height;

	dc->DrawLine(m_left_margin, 0, m_left_margin, h - m_bottom_margin);
	dc->DrawLine(m_left_margin - arrow_width, arrow_height, m_left_margin, 0);
	dc->DrawLine(m_left_margin + arrow_width, arrow_height, m_left_margin, 0);

	int rh = h - (m_top_margin + m_bottom_margin);

	DrawInfo *dy = m_graph->m_di[1];

	double dif = m_graph->m_dmax[1] - m_graph->m_dmin[1];

	int slices = FindNumberOfSlices(m_graph->m_dmax[1],
			m_graph->m_dmin[1],
			m_graph->m_di[1]);

	assert(slices > 0);

	for (int i = 0; i <= slices; ++i) {
		double val = m_graph->m_dmin[1] + dif * i / slices;

		int y = int(m_top_margin + (m_graph->m_dmax[1] - val) * rh / dif);

		dc->DrawLine(m_left_margin - mark_width, y, m_left_margin, y);

		wxString _val = dy->GetValueStr(val, _T(""));

		int tw, th;
		dc->GetTextExtent(_val, &tw, &th);
		dc->DrawText(_val, m_left_margin - tw - dc->GetPen().GetWidth(), y + dc->GetPen().GetWidth());

	}

}

void XYGraphPainter::DrawYUnit(wxDC *dc, int xdisp, int ydisp) {

	int tw, th;
	dc->GetTextExtent(m_graph->m_di[1]->GetUnit(), &tw, &th);

	dc->DrawText(m_graph->m_di[1]->GetUnit(), m_left_margin - tw - xdisp, ydisp);
}

void XYGraphPainter::DrawXUnit(wxDC *dc, int xdisp, int ydisp) {
	int w = m_width, h = m_height;

	int tw, th;
	dc->GetTextExtent(m_graph->m_di[0]->GetUnit(), &tw, &th);

	dc->DrawText(m_graph->m_di[0]->GetUnit(), 
			w - tw - xdisp, 
			h - m_bottom_margin + ydisp);
}

void XYGraphPainter::DrawGraph(wxDC *dc) {

	m_graph->point2group.clear();
	m_graph->points_groups.clear();

	std::map< int, std::map<int, std::list<int> > > points;

	wxPen pen = dc->GetPen();
	pen.SetColour(m_graph->m_di[1]->GetDrawColor());
	dc->SetPen(pen);

	for (size_t i = 0; i < m_graph->m_visible_points.size(); ++i) {
		int x,y;
		GetPointPosition(i, &x, &y);

		points[x][y].push_front(i);

		dc->DrawPoint(x, y);
		if (m_graph->m_averaged) 
			dc->DrawCircle(x, y, 2);
	}

	int groups = 0;
	std::map< int, std::map<int, std::list<int> > >::iterator i;
	std::map<int, std::list<int> >::iterator j;
	std::list<int>::iterator iter;

	for(i = points.begin(); i != points.end(); i++) {
		for(j = i->second.begin(); j != i->second.end(); j++) {

			if(j->second.size() > 1) {

				m_graph->points_groups.push_back(j->second);

				for(iter = m_graph->points_groups[groups].begin(); iter != m_graph->points_groups[groups].end(); iter++) {
					m_graph->point2group[*iter] = groups;
				}
				groups++;
			}

		}

	}

}

void XYGraphPainter::DrawDrawsInfo(wxDC *dc, int xdisp, int y) {
	int tw, th;

	int x = m_left_margin + xdisp;

	wxString text;

	text = _("Graph of ");
	dc->GetTextExtent(text, &tw, &th);
	dc->DrawText(text, x, y);
	x += tw;

	wxColour col = dc->GetTextForeground();

	text = m_graph->m_di[1]->GetName();
	dc->SetTextForeground(m_graph->m_di[1]->GetDrawColor());
	dc->GetTextExtent(text, &tw, &th);
	dc->DrawText(text, x, y);
	x += tw;

	text = _(" in terms of ");
	dc->SetTextForeground(col);
	dc->GetTextExtent(text, &tw, &th);
	dc->DrawText(text, x, y);
	x += tw;

	text = m_graph->m_di[0]->GetName();
	dc->SetTextForeground(m_graph->m_di[0]->GetDrawColor());
	dc->GetTextExtent(text, &tw, &th);
	dc->DrawText(text, x, y);
	x += tw;

	dc->SetTextForeground(col);

}

wxString XYFormatTime(const wxDateTime& time, PeriodType period) {
	wxString ret(_T(""));

	if (!time.IsValid()) {
		return ret;
	}

	assert(period != PERIOD_T_SEASON);

	int minute = time.GetMinute();
	if (period != PERIOD_T_30MINUTE)
		minute = minute / 10 * 10;

	switch (period) {
		case PERIOD_T_30MINUTE:
			ret = wxString::Format(_T(":%02d"), time.GetSecond() / 10 * 10);
		case PERIOD_T_WEEK:
		case PERIOD_T_DAY :
		case PERIOD_T_OTHER :
			ret = wxString(time.Format(_T(" %H:")) + wxString::Format(_T("%02d"), minute)) + ret;
		case PERIOD_T_MONTH :
			ret = wxString(_T("-")) 
#ifdef MINGW32
				+ time.Format(_T("%d ")) 
#else
				+ time.Format(_T("%e ")) 
#endif
				+ ret;
		case PERIOD_T_YEAR :
			ret = wxString(_T("-")) + time.Format(_T("%m")) + ret;
		case PERIOD_T_DECADE:
			ret = time.Format(_T("%Y")) + ret;
			break;
		default:
			break;
		}
		
	return ret;
}

