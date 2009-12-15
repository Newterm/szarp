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
/* $Id: visioTransparent.h 1 2009-11-17 21:44:30Z asmyk $
 *
 * visio program
 * SZARP
 *
 * asmyko@praterm.com.pl
 */
 
#ifndef __TransparentFrame__
#define __TransparentFrame__

#ifdef WX_GCH
#include <wx_pch.h>
#else
#include <wx/wx.h>
#endif

#include <wx/menu.h>

#define idMenuQuit 1000
#define idMenuAbout 1001
#include "fetchparams.h"
#include "serverdlg.h"
#include "visioParamadd.h"
#include "parlist.h"

#include "cconv.h"
#include "authdiag.h"

class TextComponent;

class TransparentFrame : public wxFrame
{
    DECLARE_EVENT_TABLE()
private:
    void OnPaint(wxPaintEvent& WXUNUSED(evt));
    void OnTimerRefresh(wxTimerEvent& event);
    void OnTimer(wxCommandEvent& event);
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnRightDown(wxMouseEvent& evt);
    virtual wxMenu *CreatePopupMenu();
    void OnPopAdd(wxCommandEvent& evt);
    void OnMenuExit(wxCommandEvent& evt);
    void OnChangeColor(wxCommandEvent& evt);
    void OnChangeFontColor(wxCommandEvent& evt);
    void OnMenuClose(wxCommandEvent& evt);
    void OnArrangeRightDown(wxCommandEvent& evt);
    void OnArrangeLeftDown(wxCommandEvent& evt);
    void OnArrangeUpRight(wxCommandEvent& evt);
    void OnChangeBottomRight(wxCommandEvent& evt);
    void OnWithFrame(wxCommandEvent& evt);
	void OnSetFontSizeBig(wxCommandEvent& evt);
	void OnSetFontSizeMiddle(wxCommandEvent& evt);
	void OnSetFontSizeSmall(wxCommandEvent& evt);
protected:

    virtual void OnClose( wxCloseEvent& event )
    {
        event.Skip();
    }
    virtual void OnQuit( wxCommandEvent& event )
    {
        event.Skip();
    }
    virtual void OnAbout( wxCommandEvent& event )
    {
        event.Skip();
    }

    int zwieksz;
    wxPoint m_delta;
    wxColour m_color;
    bool m_with_frame;
    wxTimer m_timer;
    wxMenu *m_menu;
    wxFont *m_font;
    wxColour *m_font_color;
    TextComponent *m_parameter_name;
    TextComponent *m_parameter_value;
    static szVisioAddParam  *m_apd;
public:
    TransparentFrame( wxWindow* parent,  bool with_frame = true, wxString paramName = wxT("no param"), int id = wxID_ANY, wxString title = wxT("SharpShower"), wxPoint pos = wxDefaultPosition, wxSize size = wxSize( 300,100 ), int style = wxFRAME_SHAPED|wxNO_BORDER |wxSTAY_ON_TOP );
    ~TransparentFrame();
    void DrawContent(wxDC&dc, int transparent = 0);
    wxString GetParameterName();
    wxString GetParameterValue();
    void SetParameterName(wxString text);
    void SetParameterValue(wxString text);
    static const int max_number_of_frames = 100;
    static int current_amount_of_frames;
    static TransparentFrame **all_frames;
    static int last_red;
    static wxSize defaultSizeWithFrame;
    static wxSize defaultSizeWithOutFrame;
    static szParamFetcher *m_pfetcher;
    static szProbeList m_probes;
    static TSzarpConfig *ipk;
};


class TextComponent
{
    wxPoint m_anchor;
    wxSize  m_size;
    wxString m_text;
    wxString m_text_down; //if needed text s divided into two lines
    wxFont   *m_font;
    wxColour *m_textcolor;
    wxFont   *m_font_white;
    wxColour *m_forecolorPen;
    wxColour *m_forecolorBrush;
    bool     m_fontAdjusted;
    int      m_font_size;
public:
    TextComponent(wxPoint anchor, wxSize size, int font_size = 10);
    ~TextComponent();
    void SetText(wxString text);
    wxString GetText();
    void SetFont(wxFont font, wxColour color);
    wxFont* GetFont();
    void PaintComponent(wxDC&dc);
};

#endif //__TransparentFrame__
