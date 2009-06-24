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

#include "cfgmgr.h"
#include "drawswdg.h"

#define SELSET_MAX_WIDTH 300

class SelectDrawWidget;
class DrawSet;
class DrawPanel;

/**
 * Widget for selecting set of draws.
 */
class SelectSetWidget: public wxChoice
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
    SelectSetWidget(ConfigManager *cfg, wxString confid,
			DrawPanel *parent, wxWindowID id, 
			DrawSet *set, int width = RIGHT_PANEL_LENGTH);

	virtual ~SelectSetWidget();
    
    /** @return selected draws set */
    DrawSet* GetSelected();
    
    /** Event handler, called when choice was made. */
    void OnSetChanged(wxCommandEvent &event);
    
    /** Sets select draw object to comunicate with. */
    void SetSelectDrawWidget(SelectDrawWidget *seldraw);

    /**Switches to given configuration and set
     * @param confid configuration name to swtich to
     * @param set to switch to*/
    void SelectSet(wxString confid, DrawSet *set);

    /**Select given set*/
    void SelectSet(DrawSet *set);

    /**Reacts to change of currently selected set. Informs @see DrawPanel and @see SelectDrawWidget about the change*/
    void SetChanged();

    /**Fills widget with names of currently selected draws*/
    void SetConfig();

    void SelectWithoutEvent(int selected);

protected:
    /** Adjust choice string to fit within widget width. Trucantes strings
     * that are to long.
     * @param n choice string index
     * @param width maximum string width
     */
    void AdjustString(int n, int width);
    
    /** main config manager */
    ConfigManager *cfg;

    /** Parent panel*/
    DrawPanel *panel;
    
    /** configuration id */
    wxString confid;
    
    /** draw selection object */
    SelectDrawWidget *seldraw;
    
    DECLARE_DYNAMIC_CLASS(SelectSetWidget)
};


#endif

