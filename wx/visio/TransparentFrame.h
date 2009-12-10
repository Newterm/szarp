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

class TextComponent;

class TransparentFrame : public wxFrame
{
    DECLARE_EVENT_TABLE()
private:
    void OnPaint(wxPaintEvent& WXUNUSED(evt));
    //void OnTimer(wxTimerEvent& event);
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

public:
    TransparentFrame( wxWindow* parent,  bool with_frame = true, int id = wxID_ANY, wxString title = wxT("SharpShower"), wxPoint pos = wxDefaultPosition, wxSize size = wxSize( 300,100 ), int style = wxFRAME_SHAPED|wxNO_BORDER |wxSTAY_ON_TOP );
    ~TransparentFrame();
    void DrawContent(wxDC&dc, wxString s, int transparent = 0);
    static const int max_number_of_frames = 100;
    static int current_amount_of_frames;
    static TransparentFrame **all_frames;
    static int last_red;
    static wxSize defaultSizeWithFrame;
    static wxSize defaultSizeWithOutFrame;
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
    TextComponent(wxPoint anchor, wxSize  size, int font_size = 10):
            m_anchor(anchor),
            m_size(size),
            m_text(_("")),
            m_font_size(font_size)//,
            //    m_text_org(_(""))
    {
        m_font = new wxFont(font_size, wxSWISS,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD);
        m_textcolor = new wxColour(0,0,0);
        m_forecolorPen = new wxColour(255,0,0);
        m_forecolorBrush = new wxColour(255,255,255);
        m_font_white = new wxFont(font_size + 1, wxSWISS,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD);
        m_fontAdjusted = false;
    }

    ~TextComponent()
    {
        delete m_font;
        delete m_textcolor;
        delete m_forecolorPen;
        delete m_forecolorBrush;
        delete m_font_white;
    }

    void SetText(wxString text)
    {
        m_text = text;
    }

    void SetFont(wxFont font, wxColour color)
    {
        delete m_textcolor;
        delete m_font;
        m_textcolor = new wxColour(color);
        m_font = new wxFont(font);
    }

    void AdjustFont(wxDC&dc)
    {
        int h=0, w=0, dummy;
        int n = m_text.length();
        wxString s1 = m_text;
        wxString s2 = _("");
        dc.SetFont(*m_font);
        s1 = m_text.Mid(0,n/2);
        s2 = m_text.Mid(n/2);
        m_text = s1;
        m_text_down = s2;
        m_fontAdjusted = true;
    }

    void PaintComponent(wxDC&dc)
    {
        dc.SetBrush(wxBrush(*m_forecolorBrush, wxSOLID));
        dc.DrawRoundedRectangle(m_anchor.x, m_anchor.y, m_size.GetWidth()-1, m_size.GetHeight()-1, 4);
        dc.SetFont(*m_font);
        dc.SetTextForeground(wxColour(220,220,220));
        wxString s2 = wxString(m_text);
        s2.Remove(0, m_text.Length()/2);
        wxString s1 = wxString(m_text);
        s1.Remove(m_text.Length()/2);
        dc.DrawText(s1, m_anchor.x+3, m_anchor.y-1); //background
        if (m_text_down != _(""));
        {
            dc.DrawText(s2 , m_anchor.x+4, m_anchor.y + m_size.GetHeight()/2 - 2);   //foreground
        }
        dc.SetFont(*m_font);
        dc.SetTextForeground(*m_textcolor);
        dc.DrawText(s1, m_anchor.x+4, m_anchor.y);   //foreground
        if (m_text_down != _(""));
        {
            dc.DrawText(s2 , m_anchor.x+4, m_anchor.y + m_size.GetHeight()/2 - 2);   //foreground
        }
    }
};

#endif //__TransparentFrame__
