#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif //WX_PRECOMP
#include <wx/colordlg.h>
#include "TransparentFrame.h"
#include "TaskbarExampleMain.h"
#include "fetchparams.h"

enum
{
    TIMER_ID = 10000,
    PUM_ADD,
    PUM_EXIT,
    PUM_COLOR,
    PUM_CLOSE,
    PU_ARRANGE_RD,
    PU_ARRANGE_LD,
    PU_ARRANGE_UR,
    PU_ARRANGE_BR,
    PU_ARRANGE,
    PUM_WITH_FRAME,
    PUM_FONT_COLOR
};

BEGIN_EVENT_TABLE( TransparentFrame, wxFrame )
    //   EVT_CLOSE( TransparentFrame::_wxFB_OnClose )
    EVT_PAINT(TransparentFrame::OnPaint)
    EVT_SZ_FETCH(TransparentFrame::OnTimer)
    //    EVT_TIMER(TIMER_ID, TransparentFrame::OnTimer)
    EVT_LEFT_DOWN(TransparentFrame::OnLeftDown)
    EVT_RIGHT_DOWN(TransparentFrame::OnRightDown)
    EVT_LEFT_UP(TransparentFrame::OnLeftUp)
    EVT_MOTION(TransparentFrame::OnMouseMove)
    EVT_MENU(PUM_ADD, TransparentFrame::OnPopAdd)
    EVT_MENU(PUM_EXIT, TransparentFrame::OnMenuExit)
    EVT_MENU(PUM_CLOSE, TransparentFrame::OnMenuClose)
    EVT_MENU(PUM_COLOR, TransparentFrame::OnChangeColor)
    EVT_MENU(PUM_FONT_COLOR, TransparentFrame::OnChangeFontColor)
    EVT_MENU(PU_ARRANGE_RD, TransparentFrame::OnArrangeRightDown)
    EVT_MENU(PU_ARRANGE_LD, TransparentFrame::OnArrangeLeftDown)
    EVT_MENU(PU_ARRANGE_UR, TransparentFrame::OnArrangeUpRight)
    EVT_MENU(PU_ARRANGE_BR, TransparentFrame::OnChangeBottomRight)
    EVT_MENU(PUM_WITH_FRAME, TransparentFrame::OnWithFrame)
END_EVENT_TABLE()

int TransparentFrame::current_amount_of_frames = 0;
TransparentFrame** TransparentFrame::all_frames = new TransparentFrame*[TransparentFrame::max_number_of_frames];
int TransparentFrame::last_red = 255;
wxSize TransparentFrame::defaultSizeWithFrame = wxSize(300, 84);

TransparentFrame::TransparentFrame( wxWindow* parent, bool with_frame, int id, wxString title, wxPoint pos, wxSize size, int style ) :
        wxFrame((wxFrame *)NULL,
                wxID_ANY,
                wxEmptyString,
                wxPoint(0, 0),
                defaultSizeWithFrame,
                0 | wxFRAME_SHAPED | wxSIMPLE_BORDER | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP  ),
        m_timer(this, TIMER_ID),
        m_color(*wxRED)
{
    m_font = new wxFont(10, wxSWISS,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD);
    m_font_color = new wxColour(0, 0, 0);
    m_parameter_name = new TextComponent(wxPoint(4,4), wxSize(400,35), 18);//40;
    m_parameter_value = new TextComponent(wxPoint(4,43), wxSize(400,35), 20);//60
//    m_parameter_name->SetText((_("Moc bardzo duzego kotla [kW]")));
    m_parameter_name->SetText((_("12345678901234567890123456789")));

    m_parameter_name->SetFont(*m_font, *m_font_color);
    m_parameter_value->SetFont(*m_font, *m_font_color);

    zwieksz=0;
    m_timer.Start(1000);
    m_color = wxColour(last_red, 0, 0);
    m_with_frame = with_frame;
    last_red -=20;
    if (last_red<50)last_red=255;
    if (current_amount_of_frames == 0)
    {
        all_frames[0] = this;
        for (int i=1; i<max_number_of_frames; i++)
            all_frames[i] = NULL;
        current_amount_of_frames++;
    }
}

TransparentFrame::~TransparentFrame()
{
    delete m_font;
    delete m_font_color;
    delete m_parameter_name;
    delete m_parameter_value;
}

void TransparentFrame::DrawContent(wxDC&dc, wxString s, int transparent)
{
    int w = 40, h = 20;
    wxSize size = GetClientSize();
    w = size.GetWidth();
    h = size.GetHeight();
//   dc.SetFont(*m_font);
//   dc.SetTextForeground(*m_font_color);

    if (m_with_frame == true)
    {
        if (!transparent)
        {
            dc.SetBrush(wxBrush(m_color));
            dc.SetPen(wxPen(m_color));
            dc.DrawRoundedRectangle(0, 0, w, h, 4);
        }
        else
        {
            dc.SetBrush(wxBrush(m_color));
            dc.SetPen(*wxWHITE_PEN);
            dc.DrawRectangle(0, 0, w, h);
            dc.SetPen(wxPen(m_color));
            dc.DrawRoundedRectangle(0, 0, w, h,4);
        }
        //   m_parameter_name->SetText(paramName);
        m_parameter_name->PaintComponent(dc);
    }
    else
    {
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.SetPen(*wxWHITE_PEN);
        dc.DrawRectangle(0, 0, w, h);
    }
    m_parameter_value->SetText(s);
    m_parameter_value->PaintComponent(dc);
}


void TransparentFrame::OnPopAdd(wxCommandEvent& evt)
{
    int ii=0;
    while (ii<25)
        {}
    if (current_amount_of_frames == max_number_of_frames) return;
    int i=0;
    for (; i<max_number_of_frames; i++)
        if (all_frames[i] == NULL) break;
    all_frames[i] = new TransparentFrame(this, m_with_frame);
    current_amount_of_frames++;
    all_frames[i]->Show();
    ii++;
    {}
}

void TransparentFrame::OnMenuExit(wxCommandEvent& evt)
{
    exit(0);
}


void TransparentFrame::OnMenuClose(wxCommandEvent& evt)
{
    int i=0;
    for (; i<max_number_of_frames; i++)
        if (all_frames[i] == this) break;
    //delete all_frames[i];
    all_frames[i] = NULL;
    current_amount_of_frames--;
    Close();
}

void TransparentFrame::OnChangeColor(wxCommandEvent& evt)
{
    wxColourDialog colorDialog(this);
    int result = colorDialog.ShowModal();
    if (result == wxID_OK)
    {
        wxColourData data = colorDialog.GetColourData();
        m_color = data.GetColour();
    }
}

void TransparentFrame::OnChangeFontColor(wxCommandEvent& evt)
{
    wxColourDialog colorDialog(this);
    int result = colorDialog.ShowModal();
    if (result == wxID_OK)
    {
        wxColourData data = colorDialog.GetColourData();
        delete m_font_color;
        m_font_color = new wxColour(data.GetColour());
        m_parameter_name->SetFont(*m_font, *m_font_color);
        m_parameter_value->SetFont(*m_font, *m_font_color);
    }
}


void TransparentFrame::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
    wxPaintDC dc(this);
    wxString s;
    s.Printf(wxT("%d"), zwieksz);
    DrawContent(dc, s);
}


//void TransparentFrame::OnTimer(wxTimerEvent& event)
void TransparentFrame::OnTimer(wxCommandEvent& event)
{
    int w = 0, h = 0;
    wxSize size = GetClientSize();
    w = size.GetWidth();
    h = size.GetHeight();

    // TaskbarExampleFrame::m_pfetcher->Fetch();
//    m_apd->g_data.m_probe

    wxBitmap test_bitmap(w,h);
    wxMemoryDC bdc;
    wxString s;
    zwieksz++;
    s.Printf(wxT("%d"), zwieksz);
    bdc.SelectObject(test_bitmap);

    DrawContent(bdc, s, 1);
    wxRegion region(test_bitmap, *wxWHITE);
    SetShape(region);
    Refresh();
}

void TransparentFrame::OnRightDown(wxMouseEvent& evt)
{
    PopupMenu(CreatePopupMenu());
}

void TransparentFrame::OnLeftDown(wxMouseEvent& evt)
{
    CaptureMouse();
    wxPoint pos = ClientToScreen(evt.GetPosition());
    wxPoint origin = GetPosition();
    int dx =  pos.x - origin.x;
    int dy = pos.y - origin.y;
    m_delta = wxPoint(dx, dy);
}

void TransparentFrame::OnLeftUp(wxMouseEvent& WXUNUSED(evt))
{
    if (HasCapture())
        ReleaseMouse();
}

void TransparentFrame::OnMouseMove(wxMouseEvent& evt)
{
    wxPoint pt = evt.GetPosition();
    if (evt.Dragging() && evt.LeftIsDown())
    {
        wxPoint pos = ClientToScreen(pt);
        Move(wxPoint(pos.x - m_delta.x, pos.y - m_delta.y));
    }
}


wxMenu *TransparentFrame::CreatePopupMenu()
{
    m_menu = new wxMenu;
    wxMenu *menu = m_menu;
    menu->Append(PUM_ADD, _T("&Add new parameter"));
    menu->Append(PUM_COLOR, _T("&Change frame color"));
    menu->Append(PUM_FONT_COLOR, _T("&Change font color"));
    menu->AppendCheckItem(PUM_WITH_FRAME, _T("&With frame"), _T("HELPIK"));
    menu->Check(PUM_WITH_FRAME, m_with_frame);
    wxMenu *submenu = new wxMenu;
    submenu->Append(PU_ARRANGE_RD, _T("Direction -> right down"));
    submenu->Append(PU_ARRANGE_LD, _T("Direction -> left down"));
    submenu->Append(PU_ARRANGE_UR, _T("Direction -> up right"));
    submenu->Append(PU_ARRANGE_BR, _T("Direction -> bottom right"));
    menu->Append(PU_ARRANGE, _T("Arrange frames"), submenu);
    menu->AppendSeparator();
    menu->Append(PUM_CLOSE,    _T("C&lose"));
    menu->AppendSeparator();
    menu->Append(PUM_EXIT,    _T("E&xit"));
    return menu;
}

void TransparentFrame::OnArrangeRightDown(wxCommandEvent& evt)
{
    int w = 0, h = 0;
    wxSize size = GetClientSize();
    w = size.GetWidth();
    h = size.GetHeight();
    wxSize displaySize = wxGetDisplaySize();
    int w_max, h_max;
    w_max = displaySize.GetWidth();
    h_max = displaySize.GetHeight();
    int x = w_max - w;
    int y = 0;
    for (int i=0; i<max_number_of_frames; i++)
    {
        if (all_frames[i] == NULL) continue;
        all_frames[i]->Move(wxPoint(x, y));
        if (y+2*h < h_max)
        {
            y = y+h;
            continue;
        }
        y = 0;
        x -= w;
    }
}

void TransparentFrame::OnArrangeLeftDown(wxCommandEvent& evt)
{
    int w = 0, h = 0;
    wxSize size = GetClientSize();
    w = size.GetWidth();
    h = size.GetHeight();
    wxSize displaySize = wxGetDisplaySize();
    int w_max, h_max;
    w_max = displaySize.GetWidth();
    h_max = displaySize.GetHeight();
    int x = 0;
    int y = 0;
    for (int i=0; i<max_number_of_frames; i++)
    {
        if (all_frames[i] == NULL) continue;
        all_frames[i]->Move(wxPoint(x, y));

        if (y+2*h < h_max)
        {
            y = y+h;
            continue;
        }
        y = 0;
        x += w;
    }

}

void TransparentFrame::OnArrangeUpRight(wxCommandEvent& evt)
{
    int w = 0, h = 0;
    wxSize size = GetClientSize();
    w = size.GetWidth();
    h = size.GetHeight();
    wxSize displaySize = wxGetDisplaySize();
    int w_max, h_max;
    w_max = displaySize.GetWidth();
    h_max = displaySize.GetHeight();
    int x = 0;
    int y = 0;
    for (int i=0; i<max_number_of_frames; i++)
    {
        if (all_frames[i] == NULL) continue;
        all_frames[i]->Move(wxPoint(x, y));
        if (x+2*w < w_max)
        {
            x = x+w;
            continue;
        }
        y += h;
        x = 0;
    }
}

void TransparentFrame::OnChangeBottomRight(wxCommandEvent& evt)
{
    int w = 0, h = 0;
    wxSize size = GetClientSize();
    w = size.GetWidth();
    h = size.GetHeight();
    wxSize displaySize = wxGetDisplaySize();
    int w_max, h_max;
    w_max = displaySize.GetWidth();
    h_max = displaySize.GetHeight();
    int x = 0;
    int y = h_max-h;
    for (int i=0; i<max_number_of_frames; i++)
    {
        if (all_frames[i] == NULL) continue;
        all_frames[i]->Move(wxPoint(x, y));
        if (x+2*w < w_max)
        {
            x = x+w;
            continue;
        }
        y -= h;
        x = 0;
    }
}

void TransparentFrame::OnWithFrame(wxCommandEvent& evt)
{
    m_with_frame = !m_with_frame;
    m_menu->Check(PUM_WITH_FRAME, m_with_frame);
}
