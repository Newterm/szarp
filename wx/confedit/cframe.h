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
 * ipkedit - wxWindows SZARP configuration editor
 * SZARP, 2002 Pawe³ Pa³ucha
 *
 * $Id$
 */

#ifndef __CFRAME_H__
#define __CFRAME_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxml/tree.h>
#include <libxml/relaxng.h>
#include <wx/notebook.h>
#include <wx/imaglist.h>

#include "szframe.h"
#include "cconv.h"
#include "dlistctrl.h"

/**
 * xmlNodeList is a type of list containing pointers to XML nodes.
 * List of this type are used as client data for listbox widgets. */
WX_DECLARE_LIST(xmlNode, xmlNodeList);

/**
 * This class is the application main frame. It implements almost all
 * of the program logic.
 */
class ConfFrame : public szFrame {
public:
        /**
         * @param filename name of file to load on start, may be wxEmptyString
         * @param initial program window position
         * @param initial program window size
         */
	ConfFrame(wxString filename, const wxPoint& pos, const wxSize& size);
private:
	DECLARE_EVENT_TABLE()
       	/** Event handler - called on exit command. */
	void OnExit(wxCommandEvent& event);
       	/** Event handler - displays program help. */
	void OnHelp(wxCommandEvent& event);
       	/** Event handler - displays "About" box. */
	void OnAbout(wxCommandEvent& event);
	/** Event handler - called on close event. */
	void OnClose(wxCloseEvent& event);
	/** Event handler - opens 'load file' dialog. */
	void OnOpen(wxCommandEvent& event);
	/** Event handler - reload currently loaded file. */
	void OnReload(wxCommandEvent& event);
	/** Event handler - saves current file. */
	void OnSave(wxCommandEvent& event);
	/** Event handler - display dialog for 'save as' command. */
	void OnSaveAs(wxCommandEvent& event);
	/** Event handler - clears all the attributes. */
	void OnClear(wxCommandEvent& event);
        /** Event handler, called when raport is selected. Updates list of
         * raport's items. */
        void RaportSelected(wxCommandEvent& event);
        /** Event handler, called when draw window is selected. Updates list of
         * draws. */
        void DrawSelected(wxCommandEvent& event);
        /** Event handler, called when 'up' button on toolbar
         * is pressed. */
        void UpPressed(wxCommandEvent& event);
        /** Event handler, called when 'down' button on toolbar
         * is pressed. */
        void DownPressed(wxCommandEvent& event);
 
	/** If file was modified, ask if user want to save changes and
	 * saves them if user says so. 
	 * @return 0 if we can proceed, 1 if user chose 'Cancel' or error
	 * occured while saving file.
	 */
	int AskForSave(void);
        /**
         * Tries to load configuration from file with given path,
         * set modified flag to FALSE if file was loaded. Popup message
         * box on error. */
        void LoadFile(wxString path);
	/** Saves document to file with given path, popups message box
	 * on error. On success set modified flag to FALSE.
	 * @return 0 on success, 1 on error */
        int SaveFile(wxString path);
        /** Loads IPK schema, display message box on error. Loaded schema is stored
         * in ipk_rng attribute.
         * @return 0 on success, 1 on error. */
        int LoadSchema(void);
        /** Called when file data is (re)loaded. Fills the listboxes with
         * new values. */
        void ReloadParams();
        /* Searches XML node and its children for 'draw' and 'raport'
         * elements. Called from ReloadParams, calls 'AddDraw' or 
         * 'AddRaport' methods. 
         * @param node XML node to search */
        void FindElements(xmlNodePtr node);
        /** Add raport to raplist. */
        void AddRaport(xmlNodePtr node);
        /** Add draw to drawlist. */
        void AddDraw(xmlNodePtr node);
        
        /** Moves currently selected raport item up in the list. All 
         * Move... functions assume that they can move the object - for 
         * example is isn't the last one for moving down. They update
         * corresponding widgets and set correct values of 'order' or 'prior'
         * attributes for corresponding XML nodes. */
        void MoveRapItemUp(void);
        /** Moves currently selected raport item down in the list. */
        void MoveRapItemDown(void);
        /** Moves currently selected draw item up in the list. */
        void MoveDrawItemUp(void);
        /** Moves currently selected draw item down in the list. */
        void MoveDrawItemDown(void);
        /** Moves currently selected draw up in the list. */
        void MoveDrawUp(void);
        /** Moves currently selected draw down in the list. */
        void MoveDrawDown(void);
        
        /** Assign 'order' attributes for list of XML nodes. Helper function
         * for Move* methods.
         * @param list list of nodes
         * @param pos zero-based index of last item to assign prior to */
        void AssignItemsPriors(xmlNodeList* list, int pos);
	/** Assing priors for draws windows list elements. Helper function
         * for MoveDraw* method.
	 * @param pos zero-based index of last item to assign prior to */
	void AssignWindowPriors(int pos);

	/** Helper function for ClearAll(). Searches for all 'draw' and
	 * 'raport' items in given list and removes 'order' and 'prior'
	 * attributes in list elements and all theirs children.
	 * @param list list of XML elements
	 */
	void ClearElements(xmlNodePtr list);
	/** Clears all the order and prior attributes. */
	void ClearAll();
	/** Assign prior and order attributes  corresponding to current 
	 * positions in GUI. */
	void ResetAll();

	static int wxCALLBACK sort_callback(long item1, long item2, long ignored);
 
	/** Application icon. */
	wxIcon m_icon;
	/* Was document modified ? */
	bool modified;
        /** IPK RelaxNG schema, used for validating loaded documents. */
        xmlRelaxNGPtr ipk_rng;
        /** Name of file loaded. */
        wxString filename;
        /** Data loaded, may be NULL if no document loaded. */
        xmlDocPtr params;
        
        wxNotebook* notebook;   /** Notebook widget. Page names are used to
                                  decide what actions are taken after hitting
                                  toolbar buttons, so do not change them or
                                  change them everywhere. */
        wxListBox* raplist;     /**< List of raports. Each raport has a list
                                  of XML nodes with raport items attached as
                                  client data. */
        wxListBox* ritemslist;  /**< List of raport items. Content of this 
                                  listbox is generated from raplist client
                                  data. */
        wxListBox* drawlist;	/**< List of draws (draw's windows or sets 
				  really). List of draw items is attached to 
				  each window as client data. */
        DrawsListCtrl* ditemslist;	
				/**< List of draw items. Content of this
				  list control is generated from drawlist client
				  data. */
};

#endif

