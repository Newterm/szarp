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
 * Widgets identifiers.
 */

#ifndef __IDS_H__
#define __IDS_H__

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

#include "szframe.h"

/** Available widgets identifiers */
enum { drawID_SELDRAW = wxID_HIGHEST, 
	drawID_SELSET, 
	drawID_SELDRAWCB, 
	drawTB_MENUBAR, 
	drawTB_ABOUT, 
	drawTB_SUMWIN, 
	drawTB_SPLTCRS, 
	drawTB_REFRESH, 
	drawTB_FLORENCE, 
	drawTB_GOTOLATESTDATE, 
	drawTB_EXIT, 
	drawTB_FIND,
	drawTB_REMARK,
	drawTB_NEWDRAWVERSION,
	drawTB_DRAWTREE,
	drawID_SUMMWIN,
	seldrawID_CTX_BLOCK_MENU,
	seldrawID_CTX_DOC_MENU,
	seldrawID_CTX_COPY_PARAM_NAME_MENU,
	seldrawID_CTX_EDIT_PARAM,
	seldrawID_CTX_AVERAGE_VALUE,
	seldrawID_CTX_LAST_VALUE,
	seldrawID_CTX_DIFFERENCE_VALUE,
	seldrawID_PSC,
	drawpickID_COLOR,
	drawpickTB_SAVE,
	drawpickTB_LOAD,
	drawpickTB_REMOVE,
	drawpickTB_EDIT,
	drawpickTB_PARAM_EDIT,
	drawpickTB_COLOR,
	drawpickTB_UP,
	drawpickTB_DOWN,
	drawTB_FILTER,
	incsearch_TEXT,
	incsearch_DIALOG,
	incsearch_CHOICE,
	incsearch_LIST,
	incsearch_MENU_REMOVE,
	incsearch_MENU_EDIT,
	psc_REMOVE,
	psc_EDIT,
	psc_ADD,
	psc_TO_CLK,
	psc_RELOAD,
	XY_GRAPH_FRAME,
	XY_GRAPH_DIALOG,
	XY_START_TIME,
	XY_END_TIME,
	XY_XAXIS_BUTTON,
	XY_YAXIS_BUTTON,
	XY_ZAXIS_BUTTON,
	XY_CHOICE_PERIOD,
	XY_GOTO_GRAPH_BUTTON,
	drawID_XYDIAG,
	STAT_DIAG,
	STAT_DRAW_BUTTON,
	STAT_START_TIME,
	STAT_END_TIME,
	STAT_CHOICE_PERIOD,
	STAT_CALCULATE_BUTTON,
	STAT_CLOSE_BUTTON,
	XY_CHANGE_GRAPH,
	XY_PRINT,
	XY_PRINT_PAGE_SETUP,
	XY_PRINT_PREVIEW,
	XY_ZOOM_OUT,
	XYZ_CHANGE_GRAPH,
	XYZ_PRINT,
	XYZ_PRINT_PREVIEW,
	XYZ_PRINT_PAGE_SETUP,
	XYZ_CLOSE,
	drawID_STAT_DIAG,

    codeEditorID_INDENTINC,
    codeEditorID_INDENTRED,
    codeEditorID_SELECTLINE,
    codeEditorID_BRACEMATCH,
    codeEditorID_INDENTGUIDE,
    codeEditorID_LINENUMBER,
    codeEditorID_LONGLINEON,
    codeEditorID_WHITESPACE,
    codeEditorID_FOLDTOGGLE,
    codeEditorID_OVERTYPE,
    codeEditorID_READONLY,
    codeEditorID_WRAPMODEON,
    codeEditorID_CHARSETANSI,
    codeEditorID_CHARSETMAC,
    codeEditorID_CHANGELOWER,
    codeEditorID_CHANGEUPPER,
    codeEditorID_CONVERTCR,
    codeEditorID_CONVERTCRLF,
    codeEditorID_CONVERTLF,
    psc_CHOICE,
    LAST,
    first_frame_id } ;

/** Enumeration type for timer-driven action. Keyboard-invoked
 * actions are after mouse events, so it is easier in code to
 * check type of action. */
enum ActionKeyboardType { 
	NONE, 		/**< No action - update displayed time. */
	CURSOR_UP, 	/**< Move cursor up */
	CURSOR_DOWN, 	/**< Move cursor down */
	CURSOR_UP_KB,	/**< Move cursor up from keyboard,
			this is the first keyboard event. */
	CURSOR_DOWN_KB,	/**< Move cursor down from keyboard,
			this is the first keyboard event. */
	CURSOR_LEFT, 	/**< Move cursor left */
	CURSOR_RIGHT, 	/**< Move cursor write */
	CURSOR_LEFT_KB,	/**< Move cursor left from keyboard,
			this is the first keyboard event. */
	CURSOR_RIGHT_KB,
			/**< Move cursor write from keyboard */
	CURSOR_LONG_LEFT_KB,	/**< Move cursor long left from keyboard. */
	CURSOR_LONG_RIGHT_KB,	/**< Move cursor long right from keyboard. */
	CURSOR_HOME_KB,	/**< Move cursor to the begin of screen */
	CURSOR_END_KB,	/**< Move cursor to the end of screen */
	SCREEN_LEFT_KB, /**< Move screen left. */
	SCREEN_RIGHT_KB, /**< Move screen right. */
};

#ifdef MINGW32
#define DRAW3_BG_COLOR WIN_BACKGROUND_COLOR
#else
#define DRAW3_BG_COLOR wxSystemSettings::GetColour(wxSYS_COLOUR_BACKGROUND) 
#endif

#define RIGHT_PANEL_LENGTH	300

/** Enumeration type for period shown on time axis. */
typedef enum { PERIOD_T_DECADE = 0, PERIOD_T_YEAR, PERIOD_T_MONTH, PERIOD_T_WEEK,
	PERIOD_T_DAY, PERIOD_T_30MINUTE, PERIOD_T_SEASON, PERIOD_T_OTHER, PERIOD_T_LAST } PeriodType;

/**period names*/
const wxString period_names[PERIOD_T_LAST] = 
	{_("DECADE"),  _("YEAR"), _("MONTH"), _("WEEK"), _("DAY"), _("30 MINUTES"), _("HOUR"), _("SEASON") };

/**Type of database inquires identificators*/
typedef int InquirerId;

enum AverageValueCalculationMethod { AVERAGE_VALUE_CALCULATION_AVERAGE, AVERAGE_VALUE_CALCULATION_LAST, AVERAGE_VALUE_CALCULATION_LAST_FIRST };

#endif

