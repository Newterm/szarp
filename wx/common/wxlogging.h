/*
 * SZARP: SCADA software
 *
 * Copyright (C)
 * 2011 - Jakub Kotur <qba@newterm.pl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/** 
 * @file wxlogging.h
 * @brief This file declares logging class and defines macros for
 *        logged events
 *
 * Maros are similar to onec defined in wx/event.h and should be
 * used just like EVT_XXX betwean BEGIN_EVENT_TABLE/END_EVENT_TABLE
 * wx macros. To define logger function you must call
 * DECLARE_LOGGER( msg ) macro just near DECLARE_EVENT_TABLE() 
 *
 * Example of usage:
 *
 *     BEGIN_EVENT_TABLE(DrawFrame, wxFrame)
 *       EVT_MENU(XRCID("Quit"), DrawFrame::OnExit)
 *       LOG_EVT_CLOSE( DrawFrame , OnClose , "close" )
 *     END_EVENT_TABLE()
 *
 * To change standard wx bindings to logged onec use:
 * sed -i "/\<EVT_/s/::/ , /;/\<EVT_/s/)$/, \"\" )/;/\<EVT_/s/EVT_/LOG_EVT_/" <file>
 *
 * unfortunately you still must to have to fill parameter name.
 *
 *
 * @author Jakub Kotur
 * @version 0.3
 * @date 2011-10-10
 */

#ifndef __WXLOGGING_H__

#define __WXLOGGING_H__

#include "config.h"

#include <string>

#include <wx/wx.h>

#include "liblog.h"

/** 
 * @brief sets const char* string in std::string
 * 
 * @param out string to fill
 * @param in source string
 */
void strset( std::string& out , const char* in );

/** 
 * @brief fully static class to provide logging parameters to logdmn
 *
 * This class simply construct messages for logdmn and sends them to
 * specified target. At default address is localhost and port is 7777.
 *
 * Default appname is "szarp"
 * 
 * Class uses boost-asio library, cause wxWidgets are able to send UDPLogger
 * datagrams since version 2.9
 */
class UDPLogger {
public:
	/** 
	 * @brief Inner class for logging events. It just contains string
	 * but is needed to recognize logged events basing on RTTI.
	 */
	class Data {
	public:
		Data( const char* str ) : str(str) {}
		virtual ~Data() {}
		const char* get() const { return str; }
	private:
		const char* str;
	};
	
	static void SetAppName( const char* _appname )
	{	strset(appname,_appname); }
	static void SetAddress( const char* _address )
	{	strset(address,_address); }
	static void SetPort   ( const char* _port    )
	{	strset(port   ,_port   ); }

	/** 
	 * @brief chooses events that should be logged and sends
	 * datagram if needed.
	 *
	 * This method should be called in wxApp::HandleEvent 
	 */
	static void HandleEvent(wxEvtHandler *handler, wxEventFunction func, wxEvent& event);
	/** 
	 * @brief Sends specified parameter in UDP datagram to logdmn
	 *
	 * errors are logged
	 * 
	 * @param msg name of parameter
	 */
	static void LogEvent( const char * msg );
	static int ResolveAdress();
private:
	enum states {
		UNINIT ,
		WORKING ,
		BROKEN ,
	};

	static enum states state;

	static std::string appname;
	static std::string address;
	static std::string port;
};

// Events macros copied from wx/event.h (wx-2.8)
//
// after update to newer wx there may be need to copy this
// macros once again. Do this with regexp shown below and
// some hand changes. Regexp applies to all code below them.

// s/wx__/wxlog__/g
// s/fn/fn , type , msg/

#define wxlog__DECLARE_EVT2(evt, id1, id2, fn , type , msg) \
    DECLARE_EVENT_TABLE_ENTRY(evt, id1, id2, fn , (wxObject*)new UDPLogger::Data(msg) ),
#define wxlog__DECLARE_EVT1(evt, id, fn , type , msg) \
    wxlog__DECLARE_EVT2(evt, id, wxID_ANY, fn , type , msg)
#define wxlog__DECLARE_EVT0(evt, fn , type , msg) \
    wxlog__DECLARE_EVT1(evt, wxID_ANY, fn , type , msg)

// s/\<EVT/LOG_EVT/
// s/func/type , func , msg/
// s/(func)/(type::func)/g
// s/))/),type,msg)/

// EVT_COMMAND
#define LOG_EVT_COMMAND(winid, event, type , func , msg) wxlog__DECLARE_EVT1(event, winid, wxCommandEventHandler(type::func),type)
#define LOG_EVT_COMMAND_RANGE(id1, id2, event, type , func , msg) wxlog__DECLARE_EVT2(event, id1, id2, wxCommandEventHandler(type::func),type)

#define LOG_EVT_NOTIFY(event, winid, type , func , msg) wxlog__DECLARE_EVT1(event, winid, wxNotifyEventHandler(type::func),type)
#define LOG_EVT_NOTIFY_RANGE(event, id1, id2, type , func , msg) wxlog__DECLARE_EVT2(event, id1, id2, wxNotifyEventHandler(type::func),type)

// Miscellaneous
#define LOG_EVT_SIZE(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_SIZE, wxSizeEventHandler(type::func),type,msg)
#define LOG_EVT_SIZING(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_SIZING, wxSizeEventHandler(type::func),type,msg)
#define LOG_EVT_MOVE(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_MOVE, wxMoveEventHandler(type::func),type,msg)
#define LOG_EVT_MOVING(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_MOVING, wxMoveEventHandler(type::func),type,msg)
#define LOG_EVT_CLOSE(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(type::func),type,msg)
#define LOG_EVT_END_SESSION(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_END_SESSION, wxCloseEventHandler(type::func),type,msg)
#define LOG_EVT_QUERY_END_SESSION(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_QUERY_END_SESSION, wxCloseEventHandler(type::func),type,msg)
#define LOG_EVT_PAINT(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_PAINT, wxPaintEventHandler(type::func),type,msg)
#define LOG_EVT_NC_PAINT(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_NC_PAINT, wxNcPaintEventHandler(type::func),type,msg)
#define LOG_EVT_ERASE_BACKGROUND(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(type::func),type,msg)
#define LOG_EVT_CHAR(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_CHAR, wxCharEventHandler(type::func),type,msg)
#define LOG_EVT_KEY_DOWN(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_KEY_DOWN, wxKeyEventHandler(type::func),type,msg)
#define LOG_EVT_KEY_UP(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_KEY_UP, wxKeyEventHandler(type::func),type,msg)
#if wxUSE_HOTKEY
#define LOG_EVT_HOTKEY(winid, type , func , msg)  wxlog__DECLARE_EVT1(wxEVT_HOTKEY, winid, wxCharEventHandler(type::func),type,msg)
#endif
#define LOG_EVT_CHAR_HOOK(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_CHAR_HOOK, wxCharEventHandler(type::func),type,msg)
#define LOG_EVT_MENU_OPEN(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_MENU_OPEN, wxMenuEventHandler(type::func),type,msg)
#define LOG_EVT_MENU_CLOSE(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_MENU_CLOSE, wxMenuEventHandler(type::func),type,msg)
#define LOG_EVT_MENU_HIGHLIGHT(winid, type , func , msg)  wxlog__DECLARE_EVT1(wxEVT_MENU_HIGHLIGHT, winid, wxMenuEventHandler(type::func),type,msg)
#define LOG_EVT_MENU_HIGHLIGHT_ALL(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_MENU_HIGHLIGHT, wxMenuEventHandler(type::func),type,msg)
#define LOG_EVT_SET_FOCUS(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_SET_FOCUS, wxFocusEventHandler(type::func),type,msg)
#define LOG_EVT_KILL_FOCUS(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_KILL_FOCUS, wxFocusEventHandler(type::func),type,msg)
#define LOG_EVT_CHILD_FOCUS(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_CHILD_FOCUS, wxChildFocusEventHandler(type::func),type,msg)
#define LOG_EVT_ACTIVATE(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_ACTIVATE, wxActivateEventHandler(type::func),type,msg)
#define LOG_EVT_ACTIVATE_APP(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_ACTIVATE_APP, wxActivateEventHandler(type::func),type,msg)
#define LOG_EVT_HIBERNATE(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_HIBERNATE, wxActivateEventHandler(type::func),type,msg)
#define LOG_EVT_END_SESSION(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_END_SESSION, wxCloseEventHandler(type::func),type,msg)
#define LOG_EVT_QUERY_END_SESSION(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_QUERY_END_SESSION, wxCloseEventHandler(type::func),type,msg)
#define LOG_EVT_DROP_FILES(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_DROP_FILES, wxDropFilesEventHandler(type::func),type,msg)
#define LOG_EVT_INIT_DIALOG(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_INIT_DIALOG, wxInitDialogEventHandler(type::func),type,msg)
#define LOG_EVT_SYS_COLOUR_CHANGED(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SYS_COLOUR_CHANGED, wxSysColourChangedEventHandler(type::func),type,msg)
#define LOG_EVT_DISPLAY_CHANGED(type , func , msg)  wxlog__DECLARE_EVT0(wxEVT_DISPLAY_CHANGED, wxDisplayChangedEventHandler(type::func),type,msg)
#define LOG_EVT_SHOW(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SHOW, wxShowEventHandler(type::func),type,msg)
#define LOG_EVT_MAXIMIZE(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_MAXIMIZE, wxMaximizeEventHandler(type::func),type,msg)
#define LOG_EVT_ICONIZE(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_ICONIZE, wxIconizeEventHandler(type::func),type,msg)
#define LOG_EVT_NAVIGATION_KEY(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_NAVIGATION_KEY, wxNavigationKeyEventHandler(type::func),type,msg)
#define LOG_EVT_PALETTE_CHANGED(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_PALETTE_CHANGED, wxPaletteChangedEventHandler(type::func),type,msg)
#define LOG_EVT_QUERY_NEW_PALETTE(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_QUERY_NEW_PALETTE, wxQueryNewPaletteEventHandler(type::func),type,msg)
#define LOG_EVT_WINDOW_CREATE(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_CREATE, wxWindowCreateEventHandler(type::func),type,msg)
#define LOG_EVT_WINDOW_DESTROY(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_DESTROY, wxWindowDestroyEventHandler(type::func),type,msg)
#define LOG_EVT_SET_CURSOR(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SET_CURSOR, wxSetCursorEventHandler(type::func),type,msg)
#define LOG_EVT_MOUSE_CAPTURE_CHANGED(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_MOUSE_CAPTURE_CHANGED, wxMouseCaptureChangedEventHandler(type::func),type,msg)
#define LOG_EVT_MOUSE_CAPTURE_LOST(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler(type::func),type,msg)

// Mouse events
#define LOG_EVT_LEFT_DOWN(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_LEFT_DOWN, wxMouseEventHandler(type::func),type,msg)
#define LOG_EVT_LEFT_UP(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_LEFT_UP, wxMouseEventHandler(type::func),type,msg)
#define LOG_EVT_MIDDLE_DOWN(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_MIDDLE_DOWN, wxMouseEventHandler(type::func),type,msg)
#define LOG_EVT_MIDDLE_UP(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_MIDDLE_UP, wxMouseEventHandler(type::func),type,msg)
#define LOG_EVT_RIGHT_DOWN(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_RIGHT_DOWN, wxMouseEventHandler(type::func),type,msg)
#define LOG_EVT_RIGHT_UP(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_RIGHT_UP, wxMouseEventHandler(type::func),type,msg)
#define LOG_EVT_MOTION(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_MOTION, wxMouseEventHandler(type::func),type,msg)
#define LOG_EVT_LEFT_DCLICK(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_LEFT_DCLICK, wxMouseEventHandler(type::func),type,msg)
#define LOG_EVT_MIDDLE_DCLICK(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_MIDDLE_DCLICK, wxMouseEventHandler(type::func),type,msg)
#define LOG_EVT_RIGHT_DCLICK(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_RIGHT_DCLICK, wxMouseEventHandler(type::func),type,msg)
#define LOG_EVT_LEAVE_WINDOW(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(type::func),type,msg)
#define LOG_EVT_ENTER_WINDOW(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_ENTER_WINDOW, wxMouseEventHandler(type::func),type,msg)
#define LOG_EVT_MOUSEWHEEL(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_MOUSEWHEEL, wxMouseEventHandler(type::func),type,msg)

// All mouse events
#define LOG_EVT_MOUSE_EVENTS(type , func , msg) \
    LOG_EVT_LEFT_DOWN(type , func , msg) \
    LOG_EVT_LEFT_UP(type , func , msg) \
    LOG_EVT_MIDDLE_DOWN(type , func , msg) \
    LOG_EVT_MIDDLE_UP(type , func , msg) \
    LOG_EVT_RIGHT_DOWN(type , func , msg) \
    LOG_EVT_RIGHT_UP(type , func , msg) \
    LOG_EVT_MOTION(type , func , msg) \
    LOG_EVT_LEFT_DCLICK(type , func , msg) \
    LOG_EVT_MIDDLE_DCLICK(type , func , msg) \
    LOG_EVT_RIGHT_DCLICK(type , func , msg) \
    LOG_EVT_LEAVE_WINDOW(type , func , msg) \
    LOG_EVT_ENTER_WINDOW(type , func , msg) \
    LOG_EVT_MOUSEWHEEL(type , func , msg)

// Scrolling from wxWindow (sent to wxScrolledWindow)
#define LOG_EVT_SCROLLWIN_TOP(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLLWIN_TOP, wxScrollWinEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLLWIN_BOTTOM(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLLWIN_BOTTOM, wxScrollWinEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLLWIN_LINEUP(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLLWIN_LINEUP, wxScrollWinEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLLWIN_LINEDOWN(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLLWIN_LINEDOWN, wxScrollWinEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLLWIN_PAGEUP(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLLWIN_PAGEUP, wxScrollWinEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLLWIN_PAGEDOWN(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLLWIN_PAGEDOWN, wxScrollWinEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLLWIN_THUMBTRACK(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLLWIN_THUMBTRACK, wxScrollWinEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLLWIN_THUMBRELEASE(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLLWIN_THUMBRELEASE, wxScrollWinEventHandler(type::func),type,msg)

#define LOG_EVT_SCROLLWIN(type , func , msg) \
    LOG_EVT_SCROLLWIN_TOP(type , func , msg) \
    LOG_EVT_SCROLLWIN_BOTTOM(type , func , msg) \
    LOG_EVT_SCROLLWIN_LINEUP(type , func , msg) \
    LOG_EVT_SCROLLWIN_LINEDOWN(type , func , msg) \
    LOG_EVT_SCROLLWIN_PAGEUP(type , func , msg) \
    LOG_EVT_SCROLLWIN_PAGEDOWN(type , func , msg) \
    LOG_EVT_SCROLLWIN_THUMBTRACK(type , func , msg) \
    LOG_EVT_SCROLLWIN_THUMBRELEASE(type , func , msg)

// Scrolling from wxSlider and wxScrollBar
#define LOG_EVT_SCROLL_TOP(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLL_TOP, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLL_BOTTOM(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLL_BOTTOM, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLL_LINEUP(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLL_LINEUP, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLL_LINEDOWN(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLL_PAGEUP(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLL_PAGEUP, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLL_PAGEDOWN(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLL_THUMBTRACK(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLL_THUMBRELEASE(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_SCROLL_CHANGED(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_SCROLL_CHANGED, wxScrollEventHandler(type::func),type,msg)

#define LOG_EVT_SCROLL(type , func , msg) \
    LOG_EVT_SCROLL_TOP(type , func , msg) \
    LOG_EVT_SCROLL_BOTTOM(type , func , msg) \
    LOG_EVT_SCROLL_LINEUP(type , func , msg) \
    LOG_EVT_SCROLL_LINEDOWN(type , func , msg) \
    LOG_EVT_SCROLL_PAGEUP(type , func , msg) \
    LOG_EVT_SCROLL_PAGEDOWN(type , func , msg) \
    LOG_EVT_SCROLL_THUMBTRACK(type , func , msg) \
    LOG_EVT_SCROLL_THUMBRELEASE(type , func , msg) \
    LOG_EVT_SCROLL_CHANGED(type , func , msg)

// Scrolling from wxSlider and wxScrollBar, with an id
#define LOG_EVT_COMMAND_SCROLL_TOP(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_SCROLL_TOP, winid, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_SCROLL_BOTTOM(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_SCROLL_BOTTOM, winid, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_SCROLL_LINEUP(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_SCROLL_LINEUP, winid, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_SCROLL_LINEDOWN(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_SCROLL_LINEDOWN, winid, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_SCROLL_PAGEUP(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_SCROLL_PAGEUP, winid, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_SCROLL_PAGEDOWN(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_SCROLL_PAGEDOWN, winid, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_SCROLL_THUMBTRACK(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_SCROLL_THUMBTRACK, winid, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_SCROLL_THUMBRELEASE(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_SCROLL_THUMBRELEASE, winid, wxScrollEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_SCROLL_CHANGED(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_SCROLL_CHANGED, winid, wxScrollEventHandler(type::func),type,msg)

#define LOG_EVT_COMMAND_SCROLL(winid, type , func , msg) \
    LOG_EVT_COMMAND_SCROLL_TOP(winid, type , func , msg) \
    LOG_EVT_COMMAND_SCROLL_BOTTOM(winid, type , func , msg) \
    LOG_EVT_COMMAND_SCROLL_LINEUP(winid, type , func , msg) \
    LOG_EVT_COMMAND_SCROLL_LINEDOWN(winid, type , func , msg) \
    LOG_EVT_COMMAND_SCROLL_PAGEUP(winid, type , func , msg) \
    LOG_EVT_COMMAND_SCROLL_PAGEDOWN(winid, type , func , msg) \
    LOG_EVT_COMMAND_SCROLL_THUMBTRACK(winid, type , func , msg) \
    LOG_EVT_COMMAND_SCROLL_THUMBRELEASE(winid, type , func , msg) \
    LOG_EVT_COMMAND_SCROLL_CHANGED(winid, type , func , msg)

#if WXWIN_COMPATIBILITY_2_6
    // compatibility macros for the old name, deprecated in 2.8
    #define wxEVT_SCROLL_ENDSCROLL wxEVT_SCROLL_CHANGED
    #define LOG_EVT_COMMAND_SCROLL_ENDSCROLL EVT_COMMAND_SCROLL_CHANGED
    #define LOG_EVT_SCROLL_ENDSCROLL EVT_SCROLL_CHANGED
#endif // WXWIN_COMPATIBILITY_2_6

// Convenience macros for commonly-used commands
#define LOG_EVT_CHECKBOX(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_CHECKBOX_CLICKED, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_CHOICE(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_CHOICE_SELECTED, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_LISTBOX(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_LISTBOX_SELECTED, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_LISTBOX_DCLICK(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_MENU(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_MENU_SELECTED, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_MENU_RANGE(id1, id2, type , func , msg) wxlog__DECLARE_EVT2(wxEVT_COMMAND_MENU_SELECTED, id1, id2, wxCommandEventHandler(type::func),type,msg)
#if defined(__SMARTPHONE__)
#  define LOG_EVT_BUTTON(winid, type , func , msg) EVT_MENU(winid, func)
#else
#  define LOG_EVT_BUTTON(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_BUTTON_CLICKED, winid, wxCommandEventHandler(type::func),type,msg)
#endif
#define LOG_EVT_SLIDER(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_SLIDER_UPDATED, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_RADIOBOX(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_RADIOBOX_SELECTED, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_RADIOBUTTON(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_RADIOBUTTON_SELECTED, winid, wxCommandEventHandler(type::func),type,msg)
// LOG_EVT_SCROLLBAR is now obsolete since we use EVT_COMMAND_SCROLL... events
#define LOG_EVT_SCROLLBAR(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_SCROLLBAR_UPDATED, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_VLBOX(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_VLBOX_SELECTED, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_COMBOBOX(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_COMBOBOX_SELECTED, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_TOOL(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_TOOL_CLICKED, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_TOOL_RANGE(id1, id2, type , func , msg) wxlog__DECLARE_EVT2(wxEVT_COMMAND_TOOL_CLICKED, id1, id2, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_TOOL_RCLICKED(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_TOOL_RCLICKED, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_TOOL_RCLICKED_RANGE(id1, id2, type , func , msg) wxlog__DECLARE_EVT2(wxEVT_COMMAND_TOOL_RCLICKED, id1, id2, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_TOOL_ENTER(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_TOOL_ENTER, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_CHECKLISTBOX(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, winid, wxCommandEventHandler(type::func),type,msg)

// Generic command events
#define LOG_EVT_COMMAND_LEFT_CLICK(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_LEFT_CLICK, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_LEFT_DCLICK(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_LEFT_DCLICK, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_RIGHT_CLICK(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_RIGHT_CLICK, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_RIGHT_DCLICK(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_RIGHT_DCLICK, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_SET_FOCUS(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_SET_FOCUS, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_KILL_FOCUS(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_KILL_FOCUS, winid, wxCommandEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_ENTER(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_ENTER, winid, wxCommandEventHandler(type::func),type,msg)

// Joystick events

#define LOG_EVT_JOY_BUTTON_DOWN(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_JOY_BUTTON_DOWN, wxJoystickEventHandler(type::func),type,msg)
#define LOG_EVT_JOY_BUTTON_UP(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_JOY_BUTTON_UP, wxJoystickEventHandler(type::func),type,msg)
#define LOG_EVT_JOY_MOVE(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_JOY_MOVE, wxJoystickEventHandler(type::func),type,msg)
#define LOG_EVT_JOY_ZMOVE(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_JOY_ZMOVE, wxJoystickEventHandler(type::func),type,msg)

// These are obsolete, see _BUTTON_ events
#if WXWIN_COMPATIBILITY_2_4
    #define LOG_EVT_JOY_DOWN(type , func , msg) EVT_JOY_BUTTON_DOWN(type::func)
    #define LOG_EVT_JOY_UP(type , func , msg) EVT_JOY_BUTTON_UP(type::func)
#endif // WXWIN_COMPATIBILITY_2_4

// All joystick events
#define LOG_EVT_JOYSTICK_EVENTS(type , func , msg) \
    LOG_EVT_JOY_BUTTON_DOWN(type , func , msg) \
    LOG_EVT_JOY_BUTTON_UP(type , func , msg) \
    LOG_EVT_JOY_MOVE(type , func , msg) \
    LOG_EVT_JOY_ZMOVE(type , func , msg)

// Idle event
#define LOG_EVT_IDLE(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_IDLE, wxIdleEventHandler(type::func),type,msg)

// Update UI event
#define LOG_EVT_UPDATE_UI(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_UPDATE_UI, winid, wxUpdateUIEventHandler(type::func),type,msg)
#define LOG_EVT_UPDATE_UI_RANGE(id1, id2, type , func , msg) wxlog__DECLARE_EVT2(wxEVT_UPDATE_UI, id1, id2, wxUpdateUIEventHandler(type::func),type,msg)

// Help events
#define LOG_EVT_HELP(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_HELP, winid, wxHelpEventHandler(type::func),type,msg)
#define LOG_EVT_HELP_RANGE(id1, id2, type , func , msg) wxlog__DECLARE_EVT2(wxEVT_HELP, id1, id2, wxHelpEventHandler(type::func),type,msg)
#define LOG_EVT_DETAILED_HELP(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_DETAILED_HELP, winid, wxHelpEventHandler(type::func),type,msg)
#define LOG_EVT_DETAILED_HELP_RANGE(id1, id2, type , func , msg) wxlog__DECLARE_EVT2(wxEVT_DETAILED_HELP, id1, id2, wxHelpEventHandler(type::func),type,msg)

// Context Menu Events
#define LOG_EVT_CONTEXT_MENU(type , func , msg) wxlog__DECLARE_EVT0(wxEVT_CONTEXT_MENU, wxContextMenuEventHandler(type::func),type,msg)
#define LOG_EVT_COMMAND_CONTEXT_MENU(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_CONTEXT_MENU, winid, wxContextMenuEventHandler(type::func),type,msg)

// Clipboard text Events
#define LOG_EVT_TEXT_CUT(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_TEXT_CUT, winid, wxClipboardTextEventHandler(type::func),type,msg)
#define LOG_EVT_TEXT_COPY(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_TEXT_COPY, winid, wxClipboardTextEventHandler(type::func),type,msg)
#define LOG_EVT_TEXT_PASTE(winid, type , func , msg) wxlog__DECLARE_EVT1(wxEVT_COMMAND_TEXT_PASTE, winid, wxClipboardTextEventHandler(type::func),type,msg)

#endif /* __WXLOGGING_H__ */

