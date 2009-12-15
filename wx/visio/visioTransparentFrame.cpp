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
/* $Id: visioTransparentFrame.cpp 1 2009-11-17 21:44:30Z asmyk $
 *
 * visio program
 * SZARP
 *
 * asmyko@praterm.com.pl
 */

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif //WX_PRECOMP
#include <wx/colordlg.h>
#include "visioTransparentFrame.h"
#include "visioFetchFrame.h"
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
    PUM_FONT_COLOR,
    PU_FONT_SIZE,
    PUM_FONT_SIZE_BIG,
    PUM_FONT_SIZE_MIDDLE,
    PUM_FONT_SIZE_SMALL,
};

BEGIN_EVENT_TABLE( TransparentFrame, wxFrame )
    EVT_PAINT(TransparentFrame::OnPaint)
    EVT_TIMER(TIMER_ID, TransparentFrame::OnTimerRefresh)
    EVT_LEFT_DOWN(TransparentFrame::OnLeftDown)
    EVT_RIGHT_DOWN(TransparentFrame::OnRightDown)
    EVT_LEFT_UP(TransparentFrame::OnLeftUp)
    EVT_MOTION(TransparentFrame::OnMouseMove)
    EVT_MENU(PUM_ADD, TransparentFrame::OnPopAdd)
    EVT_MENU(PUM_EXIT, TransparentFrame::OnMenuExit)
    EVT_MENU(PUM_CLOSE, TransparentFrame::OnMenuClose)
    EVT_MENU(PUM_COLOR, TransparentFrame::OnChangeColor)
    EVT_MENU(PUM_FONT_COLOR, TransparentFrame::OnChangeFontColor)
    EVT_MENU(PUM_FONT_SIZE_BIG, TransparentFrame::OnSetFontSizeBig)
    EVT_MENU(PUM_FONT_SIZE_MIDDLE, TransparentFrame::OnSetFontSizeMiddle)
    EVT_MENU(PUM_FONT_SIZE_SMALL, TransparentFrame::OnSetFontSizeSmall)
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
szVisioAddParam  *TransparentFrame::m_apd = NULL;
szParamFetcher *TransparentFrame::m_pfetcher;
szProbeList TransparentFrame::m_probes;
TSzarpConfig *TransparentFrame::ipk;

TransparentFrame::TransparentFrame( wxWindow* parent, bool with_frame, wxString paramName, int id, wxString title, wxPoint pos, wxSize size, int style ) :
        wxFrame((wxFrame *)NULL,
                wxID_ANY,
                wxEmptyString,
                wxPoint(0, 0),
                defaultSizeWithFrame,
                0 | wxFRAME_SHAPED | wxSIMPLE_BORDER | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP  ),
        m_timer(this, TIMER_ID),
        m_color(*wxRED)
{
    m_menu = NULL;
    m_font = new wxFont(15, wxSWISS,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD);
    m_font_color = new wxColour(0, 0, 0);
    m_parameter_name = new TextComponent(wxPoint(4,4), wxSize(400,35), 18);//40;
    m_parameter_value = new TextComponent(wxPoint(4,43), wxSize(400,35), 20);//60
    m_parameter_name->SetText(paramName);
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

void TransparentFrame::DrawContent(wxDC&dc, int transparent)
{
    int w = 40, h = 20;
    wxSize size = GetClientSize();
    w = size.GetWidth();
    h = size.GetHeight();

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
        m_parameter_name->PaintComponent(dc);
    }
    else
    {
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.SetPen(*wxWHITE_PEN);
        dc.DrawRectangle(0, 0, w, h);
    }
    m_parameter_value->PaintComponent(dc);
}


void TransparentFrame::OnPopAdd(wxCommandEvent& evt)
{
    int i = 0;
    if (current_amount_of_frames == max_number_of_frames) return;
    TransparentFrame::m_apd = new szVisioAddParam(this->ipk, this, wxID_ANY, _("Visio->AddParam"));
    szProbe *probe = new szProbe();
    TransparentFrame::m_apd->g_data.m_probe.Set(*probe);
    if ( TransparentFrame::m_apd->ShowModal() != wxID_OK )
        return;

    for (int j=0; j<TransparentFrame::max_number_of_frames; j++)
    {
        if (all_frames[j] != NULL && all_frames[j]->GetParameterName().Cmp(TransparentFrame::m_apd->g_data.m_probe.m_parname) == 0)
        {
            wxMessageDialog w(this, _("This parameter is already shown"), _("Visio information"), wxOK);
            w.ShowModal();
            delete TransparentFrame::m_apd;
            return;
        }
    }
    probe->Set(TransparentFrame::m_apd->g_data.m_probe);
    delete TransparentFrame::m_apd;

    m_probes.Append(probe);
    m_pfetcher->SetSource(m_probes, ipk);

    for (; i<TransparentFrame::max_number_of_frames; i++)
        if (all_frames[i] == NULL) break;

    all_frames[i] = new TransparentFrame(this, m_with_frame, wxString((probe->m_param != NULL)?(probe->m_param->GetName()):L"(none)").c_str());
    current_amount_of_frames++;
    all_frames[i]->Show();
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

void TransparentFrame::OnSetFontSizeBig(wxCommandEvent& evt)
{
    wxFont* font = m_parameter_value->GetFont();
    font->SetPointSize(20);
    font = m_parameter_name->GetFont();
    font->SetPointSize(20);
    m_parameter_value->SetFont(*font, *m_font_color);
    Refresh();
}

void TransparentFrame::OnSetFontSizeMiddle(wxCommandEvent& evt)
{
    wxFont* font = m_parameter_value->GetFont();
    font->SetPointSize(15);
    font = m_parameter_name->GetFont();
    font->SetPointSize(15);
    m_parameter_value->SetFont(*font, *m_font_color);
    Refresh();
}

void TransparentFrame::OnSetFontSizeSmall(wxCommandEvent& evt)
{
    wxFont* font = m_parameter_value->GetFont();
    font->SetPointSize(10);
    font = m_parameter_name->GetFont();
    font->SetPointSize(10);
    m_parameter_value->SetFont(*font, *m_font_color);
    Refresh();
}

void TransparentFrame::OnChangeFontColor(wxCommandEvent& evt)
{
    wxColourDialog colorDialog(this);
    int result = colorDialog.ShowModal();
    if (result == wxID_OK)
    {
        wxColourData data = colorDialog.GetColourData();
        delete m_font_color;
        wxFont* font = m_parameter_value->GetFont();
        m_font_color = new wxColour(data.GetColour());
        m_parameter_name->SetFont(*font, *m_font_color);
        m_parameter_value->SetFont(*font, *m_font_color);
    }
}


void TransparentFrame::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
    wxPaintDC dc(this);
//    wxString s;
//	  s.Printf(wxT("%d"), zwieksz);
    DrawContent(dc, 0);
}

void TransparentFrame::OnTimerRefresh(wxTimerEvent& event)
{
    int w = 0, h = 0;
    wxSize size = GetClientSize();
    w = size.GetWidth();
    h = size.GetHeight();
    wxBitmap test_bitmap(w,h);
    wxMemoryDC bdc;
    bdc.SelectObject(test_bitmap);
    DrawContent(bdc, 1);
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
    if (m_menu != NULL)
        return m_menu;
    m_menu = new wxMenu;
    wxMenu *menu = m_menu;
    menu->Append(PUM_ADD, _("Add parameter"));
    menu->Append(PUM_COLOR, _("Change frame color"));
    menu->Append(PUM_FONT_COLOR, _("Change font color"));

    wxMenu *fontsubmenu = new wxMenu;
    fontsubmenu->Append(PUM_FONT_SIZE_SMALL, _("Font small"));
    fontsubmenu->Append(PUM_FONT_SIZE_MIDDLE, _("Font middle"));
    fontsubmenu->Append(PUM_FONT_SIZE_BIG, _("Font large"));
    menu->Append(PU_FONT_SIZE, _("Change font size"), fontsubmenu);
    menu->AppendCheckItem(PUM_WITH_FRAME, _("With frame"), _T("help"));
    menu->Check(PUM_WITH_FRAME, m_with_frame);
    wxMenu *submenu = new wxMenu;
    submenu->Append(PU_ARRANGE_RD, _("Direction -> right down"));
    submenu->Append(PU_ARRANGE_LD, _("Direction -> left down"));
    submenu->Append(PU_ARRANGE_UR, _("Direction -> up right"));
    submenu->Append(PU_ARRANGE_BR, _("Direction -> bottom right"));
    menu->Append(PU_ARRANGE, _("Arrange frames"), submenu);
    menu->AppendSeparator();
    if (TransparentFrame::current_amount_of_frames > 1)
    {
        menu->Append(PUM_CLOSE,    _("Close window"));
        menu->AppendSeparator();
    }
    menu->Append(PUM_EXIT,    _("Quit"));
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

wxString TransparentFrame::GetParameterName()
{
    return m_parameter_name->GetText();
}

wxString TransparentFrame::GetParameterValue()
{
    return m_parameter_value->GetText();
}

void TransparentFrame::SetParameterName(wxString text)
{
    m_parameter_name->SetText(text);
}

void TransparentFrame::SetParameterValue(wxString text)
{
    m_parameter_value->SetText(text);
}


TextComponent::TextComponent(wxPoint anchor, wxSize  size, int font_size):
        m_anchor(anchor),
        m_size(size),
        m_text(_(" ")),
        m_font_size(font_size)
{
    m_font = new wxFont(font_size, wxSWISS,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD);
    m_textcolor = new wxColour(0,0,0);
    m_forecolorPen = new wxColour(255,0,0);
    m_forecolorBrush = new wxColour(255,255,255);
    m_font_white = new wxFont(font_size + 1, wxSWISS,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD);
    m_fontAdjusted = false;
}

TextComponent::~TextComponent()
{
    delete m_font;
    delete m_textcolor;
    delete m_forecolorPen;
    delete m_forecolorBrush;
    delete m_font_white;
}

void TextComponent::SetText(wxString text)
{
    m_text = text;
}

wxString TextComponent::GetText()
{
    return m_text;
}

void TextComponent::SetFont(wxFont font, wxColour color)
{
    if (m_font != &font)
    {
        delete m_font;
        m_font = new wxFont(font);
    }
    if (m_textcolor != &color)
    {
        delete m_textcolor;
        m_textcolor = new wxColour(color);
    }
}

wxFont* TextComponent::GetFont()
{
    return m_font;
}

void TextComponent::PaintComponent(wxDC&dc)
{
    dc.SetBrush(wxBrush(*m_forecolorBrush, wxSOLID));
    dc.DrawRoundedRectangle(m_anchor.x, m_anchor.y, m_size.GetWidth()-1, m_size.GetHeight()-1, 4);
    dc.SetFont(*m_font);
    // dc.SetTextForeground(wxColour(220,220,220));
    // dc.DrawText(m_text, m_anchor.x+3, m_anchor.y-1); //background
    dc.SetTextForeground(*m_textcolor);
    dc.DrawText(m_text, m_anchor.x+4, m_anchor.y);   //foreground
}
