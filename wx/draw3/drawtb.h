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
 
 * Micha³ Blajerski <nameless@praterm.com.pl>
 *
 * $Id: drawtb.h 15 2009-07-18 14:40:33Z reksi0 $
 * Mini toolbar to control menubar and most used draw3 functions.
 */

#ifndef __DRAWTB_H__
#define __DRAWTB_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/toolbar.h>
#include <wx/menu.h>
#include "version.h"
#include "cconv.h"

#include "wxlogging.h"


/**
 * DrawToolBar with most used buttons...
 * Important: EVENTS for toolbar buttonts must be connected in DrawFrame class
 * @see drawapp.cpp and drawapp.h
 */
class DrawToolBar : public wxToolBar
{
public:
	/**
	 * Constructor creates toolbar with most used buttons...
	 */
	DrawToolBar(wxWindow *parent);

	~DrawToolBar();

	/**'Unchecks' double cursor tool*/
	void DoubleCursorToolUncheck();

	/**'Checks' double cursor tool*/
	void DoubleCursorToolCheck();
	
	/**Set's icon representing given filter level*/
	void SetFilterToolIcon(int i);

	void NewDrawVersionAvailable();

	void NewVersionToolClicked(wxCommandEvent &e);

	DECLARE_EVENT_TABLE();

};
#endif

