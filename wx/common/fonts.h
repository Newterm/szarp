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
/**
 * Obs³uga fontów.

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 * $Id$
 */

#ifndef __FONTS_H__
#define __FONTS_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/hashmap.h>

/** Macro for setting default font. */
#define szSetDefFont(widget) \
	szFontProvider::Get()->SetWidgetFont(widget)
/** Macro for setting named font. */
#define szSetFont(widget, fontname) \
	szFontProvider::Get()->SetWidgetFont(widget, fontname)

class szFontDialogButton;
class szFontProvider;
	
class szFontDialogBtn : public wxButton {
public:
	szFontDialogBtn(szFontProvider* fontProvider, wxString fontName,
			wxWindow* parent, wxWindowID id, const wxString& label,
			wxPoint pos = wxDefaultPosition,
			wxSize size = wxDefaultSize,
			long style = 0);
	void OnConfigureFont(wxCommandEvent& WXUNUSED(evt));
	void OnResetFont(wxCommandEvent& WXUNUSED(evt));
protected:
	szFontProvider* m_fontProvider;
	wxString m_fontName;
};


	
/**
 * Class holds information about fonts used by application. Information about 
 * fonts is saved in wxConfig object. Default object is used (obtained by 
 * calling wxConfig:Get() method, you can set it before calling szFontProvider 
 * methods. Widgets are available for changing fonts' type and size. 
 */
class szFontProvider {
public:
	/**
	 * Creates object, loads information from default wxConfig object.
	 * There's no need to use constructor - use static Get() method
	 * instead.
	 */
	szFontProvider();
	~szFontProvider();
	/**
	 * Returns pointer to default szFontProvider object, creates one if
	 * it doesn't exists.
	 */
	static szFontProvider* Get();
	/**
	 * Frees all memory used by library. Deletes default szFontProvider
	 * object.
	 */
	static void Cleanup();
	/**
	 * Returns copy of font with given name. Name can be any identifier
	 * used internally by program (doesn't have any meaning). If no font
	 * with given name exists, new one (default) is created. You can use
	 * empty string (or wxEmptyString) to get default font.
	 * @param name fonts name (identifier)
	 * @return copy of font object
	 */
	wxFont GetFont(wxString name = wxEmptyString);
	/**
	 * Configure font for given name. Changes font loaded from
	 * configuration.
	 * @see SetDefaultFont
	 * @param font font to use
	 * @param name name to use for font
	 */
	void SetFont(wxFont font, wxString name = wxEmptyString);
	/**
	 * Configure default font for given name. User can reset to
	 * default font, this font is also used when no font was loaded
	 * from config.
	 */
	void SetDefaultFont(wxFont, wxString name = wxEmptyString);
	/**
	 * Reset font to default. Removes font info from config file.
	 * @param name of font, wxEmptyString for default
	 */
	void ResetFont(wxString name = wxEmptyString);
	/**
	 * Set font for widget. Gets font using GetFont().
	 * @param name name of font, empty for default
	 */
	void SetWidgetFont(wxWindow* widget, wxString name = wxEmptyString);
	/**
	 * Shows dialog for configuring fonts. All changes are saved to
	 * configuration.
	 * @param parent parent widget
	 * @param name1 (identifier) of first font to configure, if omiited,
	 * default font is configured
	 * @param descr5 description of font (showed in dialog), if empty,
	 * 'Default font' is used
	 * @param name2 you can pass up to 5 name/description pairs to
	 * configure aditional fonts, if you need more, use other version of
	 * this method, with wxArrayString argument
	 */
	void ShowFontsDialog(wxWindow* parent,
			wxString name1 =  wxEmptyString, 
			wxString descr1 = wxEmptyString,
			wxString name2 =  wxEmptyString, 
			wxString descr2 = wxEmptyString,
			wxString name3 =  wxEmptyString, 
			wxString descr3 = wxEmptyString,
			wxString name4 =  wxEmptyString, 
			wxString descr4 = wxEmptyString,
			wxString name5 =  wxEmptyString, 
			wxString descr5 = wxEmptyString
			);
	/**
	 * More general version of this method.
	 * @param parent parent window
	 * @param fonts array of font name/font description pairs
	 */
	void ShowFontsDialog(wxWindow* parent, wxArrayString fonts);
	void ConfigureFont(wxString fontName);
protected:
	static szFontProvider* m_defaultObject;
					/**< Default application font provider, 
					 * used with Get() and Set() methods. */
	WX_DECLARE_STRING_HASH_MAP(wxFont, szFontHash);
	szFontHash m_fontHash;		/**< Hash of actual fonts. */
	szFontHash m_defFontHash;	/**< Hash of default fonts. */
	wxWindow* m_dlg;		/**< Pointer to currently displayed 
					  font dialog window. */
};


#endif /* __FONTS_H__ */
