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
/* $Id: viszioTransparent.h 1 2009-11-17 21:44:30Z asmyk $
 *
 * viszio program
 * SZARP
 *
 * asmyk@praterm.com.pl
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
#include "viszioParamadd.h"
#include "parlist.h"
#include "cconv.h"
#include "authdiag.h"

enum typeOfFrame{
		FETCH_TRANSPARENT_FRAME = 8888,
		TRANSPARENT_FRAME
};

class TextComponent;

class TransparentFrame : public wxFrame
{
    DECLARE_EVENT_TABLE()
private:
    void OnPaint(wxPaintEvent& WXUNUSED(evt));
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnRightDown(wxMouseEvent& evt);
    wxMenu *CreatePopupMenu();
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
	void OnSetThresholdBig(wxCommandEvent& evt);
    void OnSetThresholdMiddle(wxCommandEvent& evt);
    void OnSetThresholdSmall(wxCommandEvent& evt);
    void OnSetThresholdVerySmall(wxCommandEvent& evt);
	void OnAdjustFont(wxCommandEvent& evt);
	void WriteConfiguration();
	bool ShouldBeTransparent(int, int, int, int, int, int, double);
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
	TextComponent *m_parameterName;
    TextComponent *m_parameterValue;
    wxString m_fullParameterName;
    wxPoint m_delta;
    wxMenu *m_menu;
    wxFont *m_font;
	wxColour m_color;
    wxColour *m_fontColor;
    bool m_withFrame;
	int m_typeOfFrame;
	wxBitmap *m_bitmap;
	wxMemoryDC *m_memoryDC;
	wxRegion *m_region;					
public:
    //TransparentFrame(wxWindow* parent,  bool with_frame = true, wxString paramName = wxT("no param"), int id = wxID_ANY, wxString title = wxT("SharpShower"), wxPoint pos = wxDefaultPosition, wxSize size = wxSize( 300,100 ), int style = wxFRAME_SHAPED|wxNO_BORDER |wxSTAY_ON_TOP );
    TransparentFrame(wxWindow* parent,  bool with_frame = true, wxString paramName = wxT("no param"), int id = wxID_ANY, wxString title = wxT("SharpShower"), wxPoint pos = wxDefaultPosition, wxSize size = wxSize( 300,100 ), int style = 0 );
    ~TransparentFrame();
    void DrawContent(wxDC&dc);
    void SetFrameConfiguration(wxString, bool, long, long, wxColour, wxColour, int, int, int);
    wxString GetParameterName();
    wxString GetParameterValue();
    void RefreshTransparentFrame();	
    void SetParameterName(wxString text);
    void SetParameterValue(wxString text);
    static const int max_number_of_frames = 100;
    static int current_amount_of_frames;
    static TransparentFrame **all_frames;
    static wxSize defaultSizeWithFrame;
    static wxSize defaultSizeWithOutFrame;
    static szParamFetcher *m_pfetcher;
    static szProbeList m_probes;
    static TSzarpConfig *ipk;
    static wxString configuration_name;    
    static long m_fontThreshold;
};


class TextComponent
{
    wxPoint m_anchor;
    wxSize  m_size;
    wxString m_text;
    wxString m_textDown;
    wxFont   *m_font;
    wxColour *m_textcolor;
    wxColour *m_forecolorPen;
    wxColour *m_forecolorBrush;
    bool 	 m_isFontAdjustable;
    bool     m_isFontAdjusted;
public:
    TextComponent(wxPoint anchor, wxSize size, int font_size = 10, bool isFontAdjustable = false);
    ~TextComponent();
    void SetAdjustable(bool value) { m_isFontAdjustable = value;m_isFontAdjusted=false;}
    void SetText(wxString text);
    void SetFont(wxFont font, wxColour color);
    bool GetAdjustable() { return m_isFontAdjustable;}
    wxString GetText();
    wxFont* GetFont();    
    void PaintComponent(wxDC&dc, wxColour fakeTransparentColor=wxColour(255,255,255));
};

#endif //__TransparentFrame__
