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
 
#ifndef __VISZIOTRANSPARENTFRAME_H__
#define __VISZIOTRANSPARENTFRAME_H__

#ifdef WX_GCH
#include <wx_pch.h>
#else
#include <wx/wx.h>
#endif

#include <wx/menu.h>
#include "fetchparams.h"
#include "serverdlg.h"
#include "viszioParamadd.h"
#include "viszioConfiguration.h"
#include "parlist.h"
#include "cconv.h"
#include "authdiag.h"

enum typeOfFrame{
		FETCH_TRANSPARENT_FRAME = 8888,
		TRANSPARENT_FRAME
};

class TextComponent;

/**
 * Viszio transparent frame 
 */
class TransparentFrame : public wxFrame
{
    DECLARE_EVENT_TABLE()
private:
	/**
	* Invoked on paint event
	*/
    void OnPaint(wxPaintEvent& WXUNUSED(evt));
    /**
	* Invoked when left mouse button has been pressed down
	*/
    void OnLeftDown(wxMouseEvent& evt);
    /**
	* Invoked when left mouse button has been released
	*/
    void OnLeftUp(wxMouseEvent& evt);
    /**
	* Invoked when mouse has been moved
	*/    
    void OnMouseMove(wxMouseEvent& evt);
    /**
	* Invoked when right mouse button has been pressed down
	*/
    void OnRightDown(wxMouseEvent& evt);
	/**
	* Creates popup menu for this frame
	*/
    wxMenu *CreatePopupMenu();
    /**
	* Invoked when menu 'add parameter' has been chosen
	*/
    void OnPopAdd(wxCommandEvent& evt);
	/**
	* Invoked when menu 'exit' has been chosen
	*/
    void OnMenuExit(wxCommandEvent& evt);
	/**
	* Invoked when menu 'change frame color' has been chosen
	*/    
    void OnChangeColor(wxCommandEvent& evt);
	/**
	* Invoked when menu 'change font color' has been chosen
	*/     
    void OnChangeFontColor(wxCommandEvent& evt);
	/**
	* Invoked when menu 'close window' has been chosen
	*/     
    void OnMenuClose(wxCommandEvent& evt);
    /**
	* Invoked when menu 'arrange frames (right down style)' has been chosen 
	*/ 
    void OnArrangeRightDown(wxCommandEvent& evt);
    /**
	* Invoked when menu 'arrange frames (left down style)' has been chosen 
	*/ 
    void OnArrangeLeftDown(wxCommandEvent& evt);
    /**
	* Invoked when menu 'arrange frames (top right style)' has been chosen 	
	*/ 
    void OnArrangeTopRight(wxCommandEvent& evt);
	/**
	* Executed on menu arrange frames (bottom right style)
	*/ 
    void OnChangeBottomRight(wxCommandEvent& evt);
	/**
	* Executed on menu with frame
	*/ 
    void OnWithFrame(wxCommandEvent& evt);
	/**
	* Executed on menu font size big
	*/ 
	void OnSetFontSizeBig(wxCommandEvent& evt);
	/**
	* Executed on menu font size middle
	*/ 
	void OnSetFontSizeMiddle(wxCommandEvent& evt);
	/**
	* Executed on menu font size small
	*/ 	
	void OnSetFontSizeSmall(wxCommandEvent& evt);
#ifndef MINGW32
	/**
	* Executed on menu antyaliasing threshold big
	*/ 	
	void OnSetThresholdBig(wxCommandEvent& evt);
	/**
	* Executed on menu antyaliasing threshold middle
	*/ 	
    void OnSetThresholdMiddle(wxCommandEvent& evt);
	/**
	* Executed on menu antyaliasing threshold small
	*/ 	    
    void OnSetThresholdSmall(wxCommandEvent& evt);
	/**
	* Executed on menu antyaliasing threshold very small
	*/ 	    
    void OnSetThresholdVerySmall(wxCommandEvent& evt);
	/**
	* Executed on menu move to another desktop
	*/ 	    
    void OnMoveToDesktop(wxCommandEvent& evt);
#endif
	/**
	* Executed on menu adjust font
	*/ 	    
	void OnAdjustFont(wxCommandEvent& evt);
	/**
	* Writes configuration to file (Linux) or to register (Windows)
	*/ 	    	
	void WriteConfiguration();
	/**
	* Returns true if this color (represents by red, green, blue components) is close enough to given fake transparent color (represents by fakeRed, fakeGreen, fakeBlue components). Closness depends on the threshold value.
	* @param fakeRed red component of fake color
	* @param fakeGreen green component of fake color
	* @param fakeBlue blue component of fake color
	* @param red red component of this color
	* @param green green component of this color
	* @param blue blue component of this color
	* @param threshold threshold of
	* @return true if this color should be transparent
	*/ 	    		
	bool ShouldBeTransparent(int, int, int, int, int, int, double);
protected:
    virtual void OnClose( wxCloseEvent& event ) { event.Skip();}
    virtual void OnQuit( wxCommandEvent& event ) { event.Skip();}
    virtual void OnAbout( wxCommandEvent& event ) { event.Skip();}
	TextComponent *m_parameterName;		/**< Text component with name of parameter */
    TextComponent *m_parameterValue;	/**< Text component with value of parameter */
    wxString m_fullParameterName;		/**< Complete parameter name */
    wxPoint m_delta;					/**< Object used for frame moving operation */
    wxMenu *m_menu;						/**< Pointer to popmenu */
#ifndef MINGW32	
    wxMenu *m_desktop_menu;				/**< Pointer to submenu with desktop switching */
    int m_desktop;						/**< Desktop identifier */
#endif
    wxFont *m_font;						/**< Pointer to style of font */
	wxColour m_color;					/**< Color of frame */
    wxColour *m_fontColor;				/**< Pointer to color of font */
    bool m_withFrame;					/**< True if frame is painted with frame */
	int m_typeOfFrame;					/**< Type of frame */
	wxBitmap *m_bitmap;					/**< Handle to bitmap that describes shape of frame */
	wxMemoryDC *m_memoryDC;				/**< Memory context */
	wxRegion *m_region;					/**< Pointer to wxRegion */
public:
	/**
	* Constructor of transparent frame
	* @param parent frame parent
	* @param with_frame true if transparent frame contains a visible graphical frame
	* @param paramName name of parameter
	* @param id frame identifier
	* @param title title of frame
	* @param pos location of frame
	* @param size size of frame
	* @param style style of frame. For transparent windows it should be set to (wxFRAME_SHAPED | wxSIMPLE_BORDER | wxSTAY_ON_TOP | wxTRANSPARENT_WINDOW).
	*/
    TransparentFrame(wxWindow* parent,  bool with_frame = true, wxString paramName = wxT("no param"), int id = wxID_ANY, wxString title = wxT("Viszio"), wxPoint pos = wxDefaultPosition, wxSize size = wxSize( 300,100 ), int style = 0 );
    ~TransparentFrame();
    /**
	* Draws content of frame on given context
	* @param dc device context
	*/
    void DrawContent(wxDC&dc);
   /**
	* Sets configuration for this frame. 
	* It works similar to constructor, but it is mainly used to mainipulate of the 
	* existing frames according to configuration written in wxConfig
	* @param name parameter name
	* @param withFrame true if transparent frame contains a graphical frame 
	* @param locationX x location of this frame
	* @param locationY y location of this frame
	* @param frameColor color of frame
	* @param fontColor color of font
	* @param fontSize size of font
	* @param paramNameSizeAdjust if equals to 1, name of parameter is fully adjusted
	* @param desktopNumber desktop number (only for Linux)
	*/    
    void SetFrameConfiguration(wxString, bool, long, long, wxColour, wxColour, int, int, int);
    /**
	* Returns name of the parameter
	* @return name of the parameter
	*/
    wxString GetParameterName();
	/**
	* Returns value of the parameter
	* @return value of the parameter
	*/
    wxString GetParameterValue();
	/**
	* Refresh this frame
	*/    
    void RefreshTransparentFrame();	
	/**
	* Sets new name of the parameter
	* @param text new name 
	*/
    void SetParameterName(wxString text);
	/**
	* Sets new value of the parameter
	* @param text new value given as a text
	*/ 	    		
    void SetParameterValue(wxString text);
#ifndef MINGW32
	/**
	* Moves frame to given desktop (workspace)
	* @param number desktop (workspace) number 
	* @return true if succeded
	*/ 	    		
    bool MoveToDesktop(int number = -1);
#endif    
    static Configuration *config;		/**< Pointer to configuration of viszio */
};


class TextComponent
{
    wxPoint m_anchor;			/**< Location of text component */
    wxSize  m_size;				/**< Size of text component */
    wxString m_text;			/**< Value of text component */
    wxString m_textDown;		/**< Value of text component (second part if needed) */
    wxFont   *m_font;			/**< Pointer to font style */
    wxColour *m_textcolor;		/**< Pointer to text color */
    wxColour *m_forecolorPen;	/**< Pointer to pen color */
    wxColour *m_forecolorBrush;	/**< Pointer to brush color */
    bool 	 m_isFontAdjustable;/**< True if font is adjustable */
    bool     m_isFontAdjusted;	/**< True if font is adjusted, if it is false text needs to be adjusted */
public:
	/** Constructor.
	 * @param anchor component location 
	 * @param size component size
	 * @param font_size font size
	 * @param isFontAdjustable true if font is adjustable automaticaly
	 */
    TextComponent(wxPoint anchor, wxSize size, int font_size = 10, bool isFontAdjustable = false);
    ~TextComponent();
	/** Sets font adjustable flag.	 
	 * @param value value of adjustable flag 
	 */
    void SetAdjustable(bool value) { m_isFontAdjustable = value;m_isFontAdjusted=false;}
	/** Sets text to component.
	 * @param text text 
	 */
    void SetText(wxString text);
    /** Sets font and color of component.
	 * @param font font style 
	 * @param color font color 
	 */
    void SetFont(wxFont font, wxColour color);
	/** Returns if font is adjustable
	 * @return true if font is adjustable
	 */ 
    bool IsAdjustable() { return m_isFontAdjustable;}
   	/** Gets text of this component.
	 * @return text
	 */ 
    wxString GetText();
   	/** Gets font of this component.
	 * @return font
	 */ 
    wxFont* GetFont();    
	/** Paints this component on given device context.
	 * @param dc device context 
	 * @param transparentColor transparent color
	 */    
    void PaintComponent(wxDC&dc, wxColour transparentColor = wxColour(255,255,255));
};

#endif //__VISZIOTRANSPARENTFRAME_H__

