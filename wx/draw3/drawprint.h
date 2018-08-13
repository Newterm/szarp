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
#ifndef __DRAWPRINT_H__
#define __DRAWPRINT_H__
/*
 * draw3 program
 * SZARP
 
 
 * $Id$
 */

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/print.h>
#endif

#include <set>

class DrawsController;

/**Class for graphs printing*/
class Print {
public:
	/**Object holding wxWidgets' internal printing data*/
	static wxPrintData *print_data;
	/**Object holding page setup*/
	static wxPageSetupData *page_setup_data;

	static wxPageSetupDialogData *page_setup_dialog_data;

	/**Initializes @see print_data and @see page_setup_data objects (if they have not yet been initialized)*/
	static void InitData();
	/**Prints graphs.
	 * @param parent parent window 
	 * @param draws array of draws to print
	 * @param count number of draws in the array*/
	static void DoPrint(wxWindow *parent, DrawsController *draws_ctrl, std::vector<Draw*> draws, int count);

	/**Shows print preview frame.
	 * @param draws array of draws to print
	 * @param count number of draws in the array*/
	static void DoPrintPreviev(DrawsController *draws_ctrl, std::vector<Draw*> draws, int count);

	/**Print XY graph. 
	 * @paran parent window for print dialog
	 * @param graph XY graph to print*/
	static void DoXYPrint(wxWindow *parent, XYGraph *graph);

	/**Prints XY graphs.
	 * @param graph XY graph to preview*/
	static void DoXYPrintPreview(XYGraph *graph);

	/**Print XY graph. 
	 * @paran parent window for print dialog
	 * @param graph XY graph to print*/
	static void DoXYZPrint(wxWindow *parent, XYGraph *graph, wxImage graph_image);

	/**Print XY graph. 
	 * @paran parent window for print dialog
	 * @param graph XY graph to print*/
	static void DoXYZPrintPreview(wxWindow *parent, XYGraph *graph, wxImage graph_image);

	static void PageSetup(wxWindow *parent);



};

#endif
