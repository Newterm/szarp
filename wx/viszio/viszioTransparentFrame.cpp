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
/* $Id: viszioTransparentFrame.cpp 1 2009-11-17 21:44:30Z asmyk $
 *
 * viszio program
 * SZARP
 *
 * asmyk@praterm.com.pl
 */

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif //WX_PRECOMP
#include <wx/colordlg.h>
#include "viszioTransparentFrame.h"
#include "viszioFetchFrame.h"
#include "fetchparams.h"
#include "parlist.h"
#include <wx/tokenzr.h>


enum
{
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
    PUM_FONT_SIZE_ADJUST
};

BEGIN_EVENT_TABLE( TransparentFrame, wxFrame )
    EVT_PAINT(TransparentFrame::OnPaint)
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
    EVT_MENU(PUM_FONT_SIZE_ADJUST, TransparentFrame::OnAdjustFont)
    EVT_MENU(PU_ARRANGE_RD, TransparentFrame::OnArrangeRightDown)
    EVT_MENU(PU_ARRANGE_LD, TransparentFrame::OnArrangeLeftDown)
    EVT_MENU(PU_ARRANGE_UR, TransparentFrame::OnArrangeUpRight)
    EVT_MENU(PU_ARRANGE_BR, TransparentFrame::OnChangeBottomRight)
    EVT_MENU(PUM_WITH_FRAME, TransparentFrame::OnWithFrame)
END_EVENT_TABLE()

int TransparentFrame::current_amount_of_frames = 0;
TransparentFrame** TransparentFrame::all_frames = new TransparentFrame*[TransparentFrame::max_number_of_frames];
wxSize TransparentFrame::defaultSizeWithFrame = wxSize(300, 84);
szParamFetcher *TransparentFrame::m_pfetcher;
szProbeList TransparentFrame::m_probes;
TSzarpConfig *TransparentFrame::ipk;
wxString TransparentFrame::configuration_name = _T("");
long TransparentFrame::m_fontThreshold = 19;

TransparentFrame::TransparentFrame( wxWindow* parent, bool with_frame, wxString paramName, int id, wxString title, wxPoint pos, wxSize size, int style ) :
        wxFrame((wxFrame *)NULL,
                wxID_ANY,
                wxEmptyString,
                wxPoint(0, 0),
                defaultSizeWithFrame,
                0 | wxFRAME_SHAPED | wxSIMPLE_BORDER | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP  ),
        m_color(*wxRED)
{
    m_typeOfFrame = TRANSPARENT_FRAME;
    m_fullParameterName = paramName;
    m_menu = NULL;
    //m_font = wxFont::New(15, wxSWISS,wxFONTFLAG_NOT_ANTIALIASED|wxFONTFLAG_DEFAULT,wxFONTSTYLE_NORMAL);
    m_font = wxFont::New(15, wxSWISS, wxFONTFLAG_NOT_ANTIALIASED, wxFONTSTYLE_NORMAL);
    m_fontColor = new wxColour(0, 0, 0);
    m_parameterName = new TextComponent(wxPoint(0,4), wxSize(400,35), 18, true);
    m_parameterValue = new TextComponent(wxPoint(0,43), wxSize(400,35), 20, false);
    m_parameterName->SetText(paramName);
    m_parameterName->SetFont(*m_font, *m_fontColor);
    m_parameterValue->SetFont(*m_font, *m_fontColor);
    m_color = wxColour(255, 0, 0);
    m_withFrame = with_frame;
    m_bitmap = NULL;//new wxBitmap(size.GetWidth(), size.GetHeight());
    m_memoryDC = NULL;// new wxMemoryDC();
    m_region = NULL;//new wxRegion(*m_bitmap, *wxWHITE);

    if (current_amount_of_frames == 0)
    {
        all_frames[0] = this;
        for (int i=1; i<TransparentFrame::max_number_of_frames; i++)
            all_frames[i] = NULL;
    }
    else
    {
        int i = 0;
        for (; i<TransparentFrame::max_number_of_frames; i++)
            if (all_frames[i] == NULL) break;
        all_frames[i] = this;
    }
    current_amount_of_frames++;
}

TransparentFrame::~TransparentFrame()
{
    delete m_font;
    delete m_fontColor;
    delete m_parameterName;
    delete m_parameterValue;
    delete m_bitmap;
    delete m_memoryDC;
    delete m_region;
}

void TransparentFrame::SetFrameConfiguration(wxString name, bool withFrame, long locationX, long locationY, wxColour frameColor, wxColour fontColor, int fontSize, int paramNameSizeAdjust, int desktopNumber)
{
    m_withFrame = withFrame;
    SetParameterName(name);
    Move(wxPoint(locationX, locationY));
    m_color = frameColor;
    if (fontSize<=10)
        fontSize=10;
    else if (fontSize<=15)
        fontSize=15;
    else
        fontSize=20;
    m_font->SetPointSize(fontSize);
    delete m_fontColor;
    m_fontColor = new wxColour(fontColor);
    m_parameterName->SetFont(*m_font, *m_fontColor);
    m_parameterValue->SetFont(*m_font, *m_fontColor);
    m_parameterName->SetAdjustable(paramNameSizeAdjust==1);
    szProbe *probe = new szProbe();
    probe->m_param = TransparentFrame::ipk->getParamByName((std::wstring)name);
    if (probe->m_param==NULL)
    {
        m_parameterName->SetText(_("error parameter name"));
        return;
    }
    m_probes.Append(probe);
    if (m_pfetcher->IsRunning())
    {
        m_pfetcher->Pause();
        m_pfetcher->SetSource(m_probes, TransparentFrame::ipk);
        m_pfetcher->Resume();
    }
    else
        m_pfetcher->SetSource(m_probes, TransparentFrame::ipk);
}


void TransparentFrame::DrawContent(wxDC&dc)
{
    if (m_bitmap==NULL)
        RefreshTransparentFrame();
    dc.DrawBitmap(*m_bitmap,0, 0, true);
    return;
}

bool TransparentFrame::ShouldBeTransparent(int fakeRed, int fakeGreen, int fakeBlue, int red, int green, int blue, double threshold)
{
    if (fakeRed==red && fakeGreen==green && fakeBlue==blue) 
		return true;
    double distance = (fakeRed - red) * (fakeRed - red);
    distance += (fakeGreen - green) * (fakeGreen - green);
    distance += (fakeBlue - blue) * (fakeBlue - blue);
    // it works quite fine for threshold from 17 to 22
    if (sqrt(distance) < threshold) 
		return true;
    return false;
}

void TransparentFrame::RefreshTransparentFrame()
{
    int w = 40, h = 20;
    wxSize size = GetClientSize();
    w = size.GetWidth();
    h = size.GetHeight();
    wxBitmap *bitmap = new wxBitmap(GetSize().GetWidth(), GetSize().GetHeight());
    wxMemoryDC *memoryDC = new wxMemoryDC();
    memoryDC->SelectObject(*bitmap);

    int r = m_fontColor->Red();
    int g = m_fontColor->Green();
    int b = m_fontColor->Blue();
    int nr = (r < 20?r + 20:r - 20);
    int ng = (g < 20?g + 20:g - 20);
    int nb = (b < 10?b + 20:b - 20);
    wxColor *fakeTransparentColor = new wxColor(nr, ng, nb);

    if (m_withFrame == true)
    {
        memoryDC->SetBrush(wxBrush(*wxWHITE));
        memoryDC->SetPen(*wxWHITE);
        memoryDC->DrawRectangle(0, 0, w, h);
        memoryDC->SetBrush(wxBrush(*fakeTransparentColor));
        memoryDC->SetPen(*fakeTransparentColor);
        memoryDC->DrawRectangle(15, 15, w, h-15);
        memoryDC->SetPen(wxPen(m_color));
        memoryDC->SetBrush(wxBrush(m_color));
        memoryDC->DrawRoundedRectangle(0, 0, w, h, 4);
        m_parameterName->PaintComponent(*memoryDC, *fakeTransparentColor);
        m_parameterValue->PaintComponent(*memoryDC, *fakeTransparentColor);
    }
    else
    {
        memoryDC->SetBrush(wxBrush(*fakeTransparentColor));
        memoryDC->SetPen(*fakeTransparentColor);
        memoryDC->DrawRectangle(0, 0, w, h);
        m_parameterValue->PaintComponent(*memoryDC, *fakeTransparentColor);
    }
    wxImage im = bitmap->ConvertToImage();
    unsigned char *data = im.GetData();
    unsigned char *correctData = (unsigned char *)malloc(sizeof(unsigned char)*GetSize().GetWidth()*GetSize().GetHeight()*3);
    nr = fakeTransparentColor->Red();
    ng = fakeTransparentColor->Green();
    nb = fakeTransparentColor->Blue();

    for (int i = 0; i < GetSize().GetWidth()*GetSize().GetHeight()*3; i+=3)
    {
        if (ShouldBeTransparent(nr, ng, nb, data[i], data[i+1], data[i+2], TransparentFrame::m_fontThreshold))
        {
            correctData[i] = 255;
            correctData[i+1] = 255;
            correctData[i+2] = 255;
        }
        else
        {
            correctData[i] = data[i];
            correctData[i+1] = data[i+1];
            correctData[i+2] = data[i+2];
        }
    }
    delete m_bitmap;
    delete bitmap;
    m_bitmap = new wxBitmap(wxImage(GetSize().GetWidth(), GetSize().GetHeight(), correctData));
    delete m_region;
    delete memoryDC;
    m_region = new wxRegion(*m_bitmap, *wxWHITE);
    SetShape(*m_region);
    Refresh();
}


void TransparentFrame::OnPopAdd(wxCommandEvent& evt)
{
    if (current_amount_of_frames >= max_number_of_frames) return;
    szViszioAddParam* apd = new szViszioAddParam(this->ipk, this, wxID_ANY, _("Viszio->AddParam"));
    szProbe *probe = new szProbe();
    apd->g_data.m_probe.Set(*probe);
    if ( apd->ShowModal() != wxID_OK )
        return;

    for (int j=0; j<TransparentFrame::max_number_of_frames; j++)
    {
        if (all_frames[j] != NULL && all_frames[j]->GetParameterName().Cmp(apd->g_data.m_probe.m_parname) == 0)
        {
            wxMessageDialog w(this, _("This parameter is already shown"), _("Viszio information"), wxOK);
            w.ShowModal();
            delete apd;
            return;
        }
    }
    TransparentFrame *frame = new TransparentFrame(this, m_withFrame, wxString((probe->m_param != NULL)?(probe->m_param->GetName()):L"(none)").c_str());
    frame->SetFrameConfiguration(apd->g_data.m_probe.m_parname, true, 0, 0, *wxRED, *wxBLACK, 15, 1, 1);
    frame->Show();
    delete apd;
}

void TransparentFrame::OnMenuExit(wxCommandEvent& evt)
{
    for (int i=0; i<TransparentFrame::max_number_of_frames; i++)
        if (all_frames[i] != NULL)
            all_frames[i]->WriteConfiguration();
    exit(0);
}


void TransparentFrame::OnMenuClose(wxCommandEvent& evt)
{
    int i=0;
    wxConfig::Get()->SetPath(_T("/")+TransparentFrame::configuration_name+_T("/Parameters"));
    wxConfig::Get()->DeleteEntry(m_fullParameterName);
    wxConfig::Get()->SetPath(_T("/"));
    wxConfig::Get()->Flush();
    for (; i<max_number_of_frames; i++)
        if (all_frames[i] == this)
            break;
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
        RefreshTransparentFrame();
    }
}

void TransparentFrame::OnSetFontSizeBig(wxCommandEvent& evt)
{
    wxFont* font = m_parameterValue->GetFont();
    font->SetPointSize(20);
    m_parameterValue->SetFont(*font, *m_fontColor);
    if (!m_parameterName->GetAdjustable())
        m_parameterName->SetFont(*font, *m_fontColor);
    //m_parameterName->SetAdjustable(false);
    RefreshTransparentFrame();
}

void TransparentFrame::OnSetFontSizeMiddle(wxCommandEvent& evt)
{
    wxFont* font = m_parameterValue->GetFont();
    font->SetPointSize(15);
    m_parameterValue->SetFont(*font, *m_fontColor);
    if (!m_parameterName->GetAdjustable())
        m_parameterName->SetFont(*font, *m_fontColor);
    RefreshTransparentFrame();
}

void TransparentFrame::OnSetFontSizeSmall(wxCommandEvent& evt)
{
    wxFont* font = m_parameterValue->GetFont();
    font->SetPointSize(10);
    m_parameterValue->SetFont(*font, *m_fontColor);
    if (!m_parameterName->GetAdjustable())
        m_parameterName->SetFont(*font, *m_fontColor);
    RefreshTransparentFrame();
}

void TransparentFrame::OnAdjustFont(wxCommandEvent& evt)
{
    m_parameterName->SetAdjustable(!m_parameterName->GetAdjustable());
    m_menu->Check(PUM_FONT_SIZE_ADJUST, m_parameterName->GetAdjustable());
    if (!m_parameterName->GetAdjustable())
        m_parameterName->SetFont(*m_parameterValue->GetFont(), *m_fontColor);
    RefreshTransparentFrame();
}

void TransparentFrame::OnChangeFontColor(wxCommandEvent& evt)
{
    wxColourDialog colorDialog(this);
    int result = colorDialog.ShowModal();
    if (result == wxID_OK)
    {
        wxColourData data = colorDialog.GetColourData();
        delete m_fontColor;
        m_fontColor = new wxColour(data.GetColour());
        m_parameterName->SetFont(*m_parameterName->GetFont(), *m_fontColor);
        m_parameterValue->SetFont(*m_parameterValue->GetFont(), *m_fontColor);
        RefreshTransparentFrame();
    }
}


void TransparentFrame::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
    wxPaintDC dc(this);
    DrawContent(dc);
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
    WriteConfiguration();
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
    fontsubmenu->AppendCheckItem(PUM_FONT_SIZE_ADJUST, _("Adjust name font size"));
    fontsubmenu->Check(PUM_FONT_SIZE_ADJUST, m_parameterName->GetAdjustable());
    fontsubmenu->AppendSeparator();
    fontsubmenu->AppendRadioItem(PUM_FONT_SIZE_SMALL, _("Font small"));
    fontsubmenu->AppendRadioItem(PUM_FONT_SIZE_MIDDLE, _("Font middle"));
    fontsubmenu->AppendRadioItem(PUM_FONT_SIZE_BIG, _("Font large"));

    if (m_parameterValue->GetFont()->GetPointSize()==10)
        fontsubmenu->Check(PUM_FONT_SIZE_SMALL, true);
    else if (m_parameterValue->GetFont()->GetPointSize()==15)
        fontsubmenu->Check(PUM_FONT_SIZE_MIDDLE, true);
    else if (m_parameterValue->GetFont()->GetPointSize()==20)
        fontsubmenu->Check(PUM_FONT_SIZE_BIG, true);

    menu->Append(PU_FONT_SIZE, _("Change font size"), fontsubmenu);
    menu->AppendCheckItem(PUM_WITH_FRAME, _("With frame"), _T("help"));
    menu->Check(PUM_WITH_FRAME, m_withFrame);
    wxMenu *submenu = new wxMenu;
    submenu->Append(PU_ARRANGE_RD, _("Direction -> right down"));
    submenu->Append(PU_ARRANGE_LD, _("Direction -> left down"));
    submenu->Append(PU_ARRANGE_UR, _("Direction -> up right"));
    submenu->Append(PU_ARRANGE_BR, _("Direction -> bottom right"));
    menu->Append(PU_ARRANGE, _("Arrange frames"), submenu);
    menu->AppendSeparator();
    if (m_typeOfFrame == TRANSPARENT_FRAME)
    {
        menu->Append(PUM_CLOSE,    _("Close window"));
        menu->AppendSeparator();
    }
    menu->Append(PUM_EXIT,    _("Quit"));
    return menu;
}


void TransparentFrame::OnArrangeRightDown(wxCommandEvent& evt)
{
    int x_start = 0, y_start = 0, w_max, h_max;
    wxClientDisplayRect(&x_start, &y_start, &w_max, &h_max);
    int w = 0, h = 0;
    wxSize size = GetClientSize();
    w = size.GetWidth();
    h = size.GetHeight();
    //wxSize displaySize = wxGetDisplaySize();
    //int w_max, h_max;
    //w_max = displaySize.GetWidth();
    //h_max = displaySize.GetHeight();
    int x = w_max - w;
    int y = y_start;
    // int x_corect = 0, y_correct = 0;
    for (int i = 0; i < max_number_of_frames; i++)
    {
        if (all_frames[i] == NULL) continue;
        all_frames[i]->Move(wxPoint(x, y));
        if (y + 2 * h < h_max)
        {
            y = y + h;
            continue;
        }
        y = y_start;
        x -= w;
    }
}

void TransparentFrame::OnArrangeLeftDown(wxCommandEvent& evt)
{
    int x_start = 0, y_start = 0, w_max, h_max;
    wxClientDisplayRect(&x_start, &y_start, &w_max, &h_max);
    int w = 0, h = 0;
    wxSize size = GetClientSize();
    w = size.GetWidth();
    h = size.GetHeight();
    //wxSize displaySize = wxGetDisplaySize();
    //int w_max, h_max;
    //w_max = displaySize.GetWidth();
    //h_max = displaySize.GetHeight();
    int x = x_start;
    int y = y_start;
    for (int i = 0; i < max_number_of_frames; i++)
    {
        if (all_frames[i] == NULL) continue;
        all_frames[i]->Move(wxPoint(x, y));

        if (y + 2 * h < h_max)
        {
            y = y + h;
            continue;
        }
        y = y_start;
        x += w;
    }

}

void TransparentFrame::OnArrangeUpRight(wxCommandEvent& evt)
{
    int x_start = 0, y_start = 0, w_max, h_max;
    wxClientDisplayRect(&x_start, &y_start, &w_max, &h_max);
    int w = 0, h = 0;
    wxSize size = GetClientSize();
    w = size.GetWidth();
    h = size.GetHeight();
    //wxSize displaySize = wxGetDisplaySize();
    //int w_max, h_max;
    //w_max = displaySize.GetWidth();
    //h_max = displaySize.GetHeight();
    int x = x_start;
    int y = y_start;
    for (int i = 0; i < max_number_of_frames; i++)
    {
        if (all_frames[i] == NULL) continue;
        all_frames[i]->Move(wxPoint(x, y));
        if (x + 2 * w < w_max)
        {
            x = x + w;
            continue;
        }
        y += h;
        x = x_start;
    }
}

void TransparentFrame::OnChangeBottomRight(wxCommandEvent& evt)
{
    int x_start = 0, y_start = 0, w_max, h_max;
    wxClientDisplayRect(&x_start, &y_start, &w_max, &h_max);
    int w = 0, h = 0;
    wxSize size = GetClientSize();
    w = size.GetWidth();
    h = size.GetHeight();
    //wxSize displaySize = wxGetDisplaySize();
    //int w_max, h_max;
    //w_max = displaySize.GetWidth();
    //h_max = displaySize.GetHeight();
    int x = x_start;
    int y = h_max - h;
    for (int i = 0; i < max_number_of_frames; i++)
    {
        if (all_frames[i] == NULL) continue;
        all_frames[i]->Move(wxPoint(x, y));
        if (x + 2 * w < w_max)
        {
            x = x + w;
            continue;
        }
        y -= h;
        x = x_start;
    }
}

void TransparentFrame::OnWithFrame(wxCommandEvent& evt)
{
    m_withFrame = !m_withFrame;
    m_menu->Check(PUM_WITH_FRAME, m_withFrame);
    RefreshTransparentFrame();
}

wxString TransparentFrame::GetParameterName()
{
    return m_fullParameterName;
    //return m_parameterName->GetText();
}

wxString TransparentFrame::GetParameterValue()
{
    return m_parameterValue->GetText();
}

void TransparentFrame::SetParameterName(wxString text)
{
    m_fullParameterName = text;
    m_parameterName->SetText(text);
}

void TransparentFrame::SetParameterValue(wxString text)
{
    m_parameterValue->SetText(text);
}


void TransparentFrame::WriteConfiguration()
{
    wxConfig::Get()->Flush();
    wxConfig::Get()->SetPath(_T("/") + configuration_name + _T("/Parameters"));
    int locationX = -1, locationY = -1;
    GetPosition(&locationX, &locationY);
    wxString mystring = wxString::Format(wxT("%d %d %d %d %d %d %d %d %d %d %d 1"), (m_withFrame?1:0), locationX, locationY, m_color.Red(), m_color.Green(), m_color.Blue(), m_fontColor->Red(), m_fontColor->Green(), m_fontColor->Blue(), m_parameterValue->GetFont()->GetPointSize(), m_parameterName->GetAdjustable());
    wxConfig::Get()->Write(GetParameterName(), mystring);
    wxConfig::Get()->SetPath(_T("/"));
    wxConfig::Get()->Write(_T("FontThreshold"), TransparentFrame::m_fontThreshold);
    wxConfig::Get()->Flush();
}


TextComponent::TextComponent(wxPoint anchor, wxSize  size, int font_size, bool isFontAdjustable):
        m_anchor(anchor),
        m_size(size),
        m_text(_(" "))
{
    m_font = NULL;//new wxFont(font_size, wxSWISS,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD);
    m_textcolor = new wxColour(0,0,0);
    m_forecolorPen = new wxColour(255,0,0);
    m_forecolorBrush = new wxColour(255,255,255);
    m_textDown = _T("");
    m_isFontAdjustable = isFontAdjustable;
    if (m_isFontAdjustable)
        m_isFontAdjusted = false;
    else
        m_isFontAdjusted = true;
}

TextComponent::~TextComponent()
{
    delete m_font;
    delete m_textcolor;
    delete m_forecolorPen;
    delete m_forecolorBrush;
}

void TextComponent::SetText(wxString text)
{
    m_text = text;
}

wxString TextComponent::GetText()
{
    return m_text + m_textDown;
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

void TextComponent::PaintComponent(wxDC&dc, wxColour fakeTransparentColor)
{
    //dc.SetBrush(wxBrush(*m_forecolorBrush, wxSOLID));
    dc.SetBrush(wxBrush(fakeTransparentColor, wxSOLID));
    dc.DrawRoundedRectangle(m_anchor.x, m_anchor.y, m_size.GetWidth()-1, m_size.GetHeight()-1, 4);
    dc.SetFont(*m_font);
    dc.SetTextForeground(*m_textcolor);

    if (m_isFontAdjustable && !m_isFontAdjusted)
    {
        wxString fullText = m_text+m_textDown;
        m_isFontAdjusted = true;
        wxSize text_size;
        int f_size = 25;
        do
        {
            f_size -= 5;
            m_font->SetPointSize(f_size);
            dc.SetFont(*m_font);
            text_size = dc.GetTextExtent(fullText);
        }
        while (text_size.GetWidth()>m_size.GetWidth()/2 && f_size>10);

        SetFont(*m_font, *m_textcolor);

        if (f_size == 10)
        {
            wxString tekst = _T("");
            wxStringTokenizer stt(fullText, wxT(":;  \t\n"), wxTOKEN_RET_DELIMS);
            unsigned int len = fullText.Length();
            while (tekst.Length() < len/2 && stt.HasMoreTokens())
                tekst += stt.GetNextToken();
            m_textDown = _T("");
            while (stt.HasMoreTokens())
                m_textDown += stt.GetNextToken();
            m_text = tekst;
        }
        else
            m_textDown = _T("");
    }

    if (m_isFontAdjustable)
    {
        dc.DrawText(m_text, m_anchor.x+4, m_anchor.y);
        dc.DrawText(m_textDown, m_anchor.x+4, m_anchor.y + 12);
    }
    else
        dc.DrawText(m_text+m_textDown, m_anchor.x+4, m_anchor.y);
}
