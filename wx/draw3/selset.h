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
 * draw3
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 * Widget for selecting set of draws.
 */

#ifndef __SELSET_H__
#define __SELSET_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

/**
 * Widget for selecting set of draws.
 */
class SelectSetWidget: public wxChoice, public DrawObserver, public ConfigObserver 
{
public:
    SelectSetWidget() : wxChoice()
    { }
    
    /**
     * @param cfg configuration manager
     * @param confid identifier of configuration
     * @param parent parent widget
     * @param id widget identifier
     * @param width widget width - strings that are to long are truncated
     * to fit within given width
     */
    SelectSetWidget(ConfigManager *cfg, DrawPanel *parent,
		    wxWindowID id, int width = RIGHT_PANEL_LENGTH);

    virtual ~SelectSetWidget();
    
    /** Event handler, called when choice was made. */
    void OnSetChanged(wxCommandEvent &event);
    
    void SelectWithoutEvent(int selected);

    virtual void SetSelection(int selected);

    virtual void DrawInfoChanged(Draw *draw);

    virtual void DrawInfoReloaded(Draw *draw);

    virtual void Attach(DrawsController *draws_controller);

    virtual void SetRemoved(wxString prefix, wxString name);

    virtual void SetModified(wxString prefix, wxString name, DrawSet *set);

    virtual void SetRenamed(wxString prefix, wxString from, wxString to, DrawSet *set);

    virtual void SetAdded(wxString prefix, wxString name, DrawSet *set);
protected:
    /** Adjust choice string to fit within widget width. Trucantes strings
     * that are to long.
     * @param n choice string index
     * @param width maximum string width
     */
    void AdjustString(int n, int width);

    void SetConfig();

    void SelectSet(DrawSet *set);

    DrawSet* GetSelected();
    
    /** main config manager */
    ConfigManager *m_cfg;

    /** configuration id */
    wxString m_prefix;

    /** set name*/
    wxString m_set_name;
   
    /** draws widget tobject */
    DrawsController *m_draws_controller;
    
    DECLARE_DYNAMIC_CLASS(SelectSetWidget)
};


#endif

