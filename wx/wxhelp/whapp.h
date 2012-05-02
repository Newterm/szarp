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
 * wxhelp - simple wxWindows HTML help viewer
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 */

#ifndef __WHAPP_H__
#define __WHAPP_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/html/helpctrl.h>
#include <wx/config.h>
#include "htmlview.h"
#include "szhlpctrl.h"
#include "szapp.h"


/**
 * Main application class.
 */
class WHApp: public szApp<>
{
	/**
	 * Method called on application start.
	 */
	virtual bool OnInit();
	virtual int OnExit();
	
	private:
		szHelpController *m_help;
		HtmlViewerFrame *frame;
		wxLocale locale;
};



#endif
