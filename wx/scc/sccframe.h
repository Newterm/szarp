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
 * scc - Szarp Control Center
 * SZARP

 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 */

#ifndef __SCCFRAME_H__
#define __SCCFRAME_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/dynarray.h>
#include <wx/timer.h>
#include <wx/taskbar.h>
#include "sccmenu.h"
#include "scctunnelframe.h"
#include "szhlpctrl.h"
#include "sztaskbaritem.h"
#include "scchideselectionframe.h"

/**
 * Class represents main program frame. It's is a small window with animated
 * icon and contex menus called on mouse clicks. Window can be moved by
 * dragging it with right mouse button.
 */
class SCCTaskBarItem : public szTaskBarItem
{
	public:
		/**
		 * @param menu main menu to display
		 * @param prefix prefix of animation frames images
		 * @param suffix animation frames images suffix
		 */
		SCCTaskBarItem(SCCMenu *menu,
				wxString prefix = _T("/opt/szarp/resources/wx/anim/praterm"),
				wxString suffix = _T(".png"));
		~SCCTaskBarItem();
		/**
		 * Event handler, called on left mouse button click, activates
		 * SZARP popup menu.
		 */
		void OnMouseDown(wxTaskBarIconEvent & event);
		/**
		 * Event handler, called on middle mouse button click. It
		 * popups program 'system' menu.
		 */
		void OnMouseMiddleDown(wxTaskBarIconEvent& event);
		/**
		 * Event handler, called on 'About' menu command executed.
		 * Displays about dialog.
		 */
		void OnAbout(wxCommandEvent& WXUNUSED(event));
		/**
		 * Event handler, called on 'Help' menu command executed.
		 * Displays program help.
		 */
		void OnHelp(wxCommandEvent& WXUNUSED(event));
		/**
		 * Event handler, called on 'Support Tunnel' menu command execution.
		 * Asks if user really wants to start a tunnel. If yes shows an appriopriate
		 * window.
		 */
		void OnSupportTunnel(wxCommandEvent& WXUNUSED(event));

		/**Allows to switch szarp data direcotory*/
		void OnSzarpDataDir(wxCommandEvent& WXUNUSED(event));
#ifndef MINGW32
		/**
		 * Event handler, called on 'SZARP fonts' menu command execution.
		 * Shows dialog for setting default SZARP apps font.
		 */
		void OnFonts(wxCommandEvent& WXUNUSED(event));
#endif /* ! MINGW32 */
		/**
		 * Event handler, called on 'Quit' menu command executed.
		 * Terminates program.
		 */
		void OnQuit(wxCommandEvent& WXUNUSED(event));
		/**
		 * Event handler, called when 'SZARP' menu item is selected.
		 * Executes specified command.
		 */
		void OnMenu(wxCommandEvent& event);
		/**
		 * Event handler, called on closing the window. Tries to veto
		 * this action if it's possible.
		 */
		void OnClose(wxCloseEvent& event);
		/**
		 * Event handler, called on 'SZARP hide databases' menu command execution.
		 * Shows dialog for setting which SZARP databases will be shown or will be hidden.
		 */
    		void OnHideBases(wxCommandEvent& event);
		/**
		 * Replaces existing main menu
		 * @param _menu menu the existing one should be replaced with
		 */
		void ReplaceMenu(SCCMenu *_menu);

	private:
		/**
		 * Creates program's system menu.
		 * @return pointer to newly created menu
		 */
		wxMenu* CreateSystemMenu();

		/**
		 * Loads images for icon animation.
		 * @param prefix prefix of image, files to load, program tries
		 * to load files with names <prefix><X><suffix>, where X are
		 * 0 padded numbers (from '01').
		 * @param suffix @see prefix
		 * @return pointer to newly created array of images, NULL
		 * on error
		 */
		wxMenu*	system_menu;
				/**< Program's 'system' menu, activated on
				 * right mouse button click. */
		SCCMenu* menu;	/**< Main menu structure */
		SCCMenu* new_menu; /**<New menu that will be shown*/
		wxMenu* wxmenu;	/**< Corresponding wxMenu object */
		SCCSelectionFrame *m_sel_frame; /**< Pointer to frame with selection of databases to be hidden*/
		SCCTunnelFrame* tunnel_frame;
				/**< Pointer to tunnel frame*/
		szHelpController *help; /**< Pointer to new help*/

		DECLARE_EVENT_TABLE()
};


#endif
