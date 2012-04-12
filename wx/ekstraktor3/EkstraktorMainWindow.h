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
 * $Id$
 */

#ifndef EKSTRAKTOR_MAIN_WINDOW
#define EKSTRAKTOR_MAIN_WINDOW

#include <wx/wx.h>
#include <wx/progdlg.h>
#include <wx/cshelp.h>

#include "szbextr/extr.h"
#include "ids.h" 
#include "parselect.h"
#include "htmlview.h"
#include "parlist.h"
#include "szhlpctrl.h"
 

class EkstraktorWidget;

#define START_DATE_STR _("Start date: ")
#define STOP_DATE_STR _("Stop date: ")
#define FIRST_DATE_STR _("First date in database: ")
#define LAST_DATE_STR _("Last date in database: ")

/** Widget for main application window. */
class EkstraktorMainWindow : wxFrame
{
	EkstraktorWidget* mainWidget; 	/**< This shouldnt be called 'widget' - this is
					a containter for various application data. */

	szParSelect *ps;	/**< widget for selecting parameters */

	std::wstring filename; /**< file to write the output to */
	std::wstring directory; /**< directory where to write the output to */

	// SIZERS
        wxBoxSizer* sizer_top;
	  // level1
          wxBoxSizer* sizer1;
	    // level2
 	    wxBoxSizer* sizer1_1;
	      // level3
	      wxBoxSizer* sizer1_1_1;
	        // level4
	        wxBoxSizer* sizer1_1_1_1;
	        wxBoxSizer* sizer1_1_1_2;
	      // level3
	      wxBoxSizer* sizer1_1_2;
	    // level2
	    wxBoxSizer* sizer1_2;
	  // level1
          wxBoxSizer* sizer2;

	// BEGIN/END DATA
	wxStaticText *firstDatabaseText;
	wxStaticText *lastDatabaseText;
	wxStaticText *startDateText;
	wxStaticText *stopDateText;

	// BEGIN/END DATA BUTTONS 
	wxButton* setStartDateBtn;
	wxButton* setStopDateBtn;
	
	// MINIMIZE/UNMINIMIZE
	wxCheckBox *minimize;

	// LIST OF CHOSEN PARAMETERS
	wxListBox *parametersList;
				/**< List box for parameters */
	szParList *parlist;	/**< Object for holding current parameters
				  list. */

	wxProgressDialog *progressDialog;
				/**< Progress Bar (activated in the extraction time) */

	HtmlViewerFrame *hview; /**< Help window */

	void CreateMenu();	/**< menu construction */

	enum { PERIOD, COMMA } selectedSeparator; /**< selected separator */

	int selectedPeriod;	/**< selected time period*/
	SzbExtractor::ParamType selectedValueType; /**< selected value type */
	
	void onPeriodChange(wxCommandEvent &event);
	void onSeparatorChange(wxCommandEvent &event);

	public:
		int cancel_lval;	/** cancel controll value */

		EkstraktorMainWindow(EkstraktorWidget *widget, const wxPoint, const wxSize);
		~EkstraktorMainWindow();

		void onChooseParams(wxCommandEvent &event);
		void onAddParams(szParSelectEvent &event);
		void onDeleteParams(wxCommandEvent &event);
		void onReadParamListFromFile(wxCommandEvent &event);
		void onWriteParamListToFile(wxCommandEvent &event);
		void onWriteResults(wxCommandEvent &event);
		void onWriteAvailableParamsToDisk(wxCommandEvent &event);
		void onPrintParams(wxCommandEvent &event);
		void onPrintPreview(wxCommandEvent &event);
		void onQuit(wxCommandEvent &event);
		void onClose(wxCloseEvent &event);
		/** Shows fonts selecting dialog */
		void onFonts(wxCommandEvent &event);
		void onHelp(wxCommandEvent &event);
		void onShowContextHelp(wxCommandEvent& event);
		void onAbout(wxCommandEvent &event);
		void onMinimize(wxCommandEvent &event);
		void onTimeStep(wxCommandEvent &event);
		void onStartDate(wxCommandEvent &event);
		void onStopDate(wxCommandEvent &event);
		void onAverageTypeSet(wxCommandEvent &event);
		void onEndTypeSet(wxCommandEvent &event);

		void SetStartDate(time_t start_date);
		void SetStopDate(time_t stop_date);
		void SetFirstDatabaseDate(time_t first_date);
		void SetLastDatabaseDate(time_t last_date);

		void SetStartDateUndefined();
		void SetStopDateUndefined();
		void SetFirstDatabaseDateUndefined();
		void SetLastDatabaseDateUndefined();
	
		bool UpdateProgressBar(int percentage);
		void  ResumeProgressBar();
		
		void SetStatusText(const wxString &string) { 
			wxFrame::SetStatusText(string); 
		}

		void SetTitle(const wxString &string) { 
			wxFrame::SetTitle(wxString(EKSTRAKTOR_TITLEBAR) + _T(" ") + string); 
		}
		/** Checks if parameter list is empty and disables some menu items. */
		void TestEmpty();

	private:
		wxString FormatDate(time_t date);
		/* We need pointers to widgets that can be disabled/enabled. */
		wxMenu *fileMenu;	/**< File menu. */
		wxButton *del_par_bt;	/**< 'Delete parameter' button */
		wxButton *write_bt;	/**< 'Write results' button */
		szHelpController *m_help; /**< Help system. */
		szHelpControllerHelpProvider *m_provider; /**< Context help. */
		DECLARE_EVENT_TABLE()
};

void *print_progress(int progress, void *data);

#endif
