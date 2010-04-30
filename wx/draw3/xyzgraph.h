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
 * $Id: xygraph.h 237 2009-12-21 11:35:22Z reksi0 $
 */


#ifndef __XYZGRAPH_H__
#define __XYZGRAPH_H__
// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_GLCANVAS
#ifdef HAVE_FTGL

#include <utility>
#include <deque>
#include <vector>

#include <wx/frame.h>

class XYZCanvas;

class XYZFrame: public XFrame, public szFrame {
	XYZCanvas *m_canvas;

	/**prefix of configuration users initially chooses draws from*/
	wxString m_default_prefix;
	/**@see DatabaseManager*/
	DatabaseManager *m_db_manager;
	/**@see ConfigManager*/
	ConfigManager *m_cfg_manager;

	FrameManager *m_frame_manager;

	XYDialog *m_xydialog;
public:
	XYZFrame(wxString default_prefix, DatabaseManager *dbmanager, ConfigManager *cfgmanager, FrameManager *frame_manager);

	virtual int GetDimCount();

	void GetNewGraph();

	virtual void SetGraph(XYGraph *graph);

	void OnChangeGraph(wxCommandEvent &event);

	void OnQuit(wxCommandEvent &event);

	void OnPrint(wxCommandEvent &event);

	void OnPrintPreview(wxCommandEvent &event);

	void OnPageSetup(wxCommandEvent &event);

	void OnHelp(wxCommandEvent &event);

	virtual ~XYZFrame();

	DECLARE_EVENT_TABLE()
};

#endif
#endif

#endif
