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

#ifndef _CODE_EDITOR_H_
#define _CODE_EDITOR_H_

#include <wx/wxscintilla.h>

class CodeEditor: public wxScintilla {

public:
	CodeEditor (wxWindow *parent, wxWindowID id = -1,
			const wxPoint &pos = wxDefaultPosition,
			const wxSize &size = wxDefaultSize,
			long style = wxSUNKEN_BORDER|wxVSCROLL
			);

	~CodeEditor ();

	void OnSize( wxSizeEvent &event );
	void OnEditRedo (wxCommandEvent &event);
	void OnEditUndo (wxCommandEvent &event);
	void OnEditClear (wxCommandEvent &event);
	void OnEditCut (wxCommandEvent &event);
	void OnEditCopy (wxCommandEvent &event);
	void OnEditPaste (wxCommandEvent &event);
	void OnBraceMatch (wxCommandEvent &event);
	void OnEditIndentInc (wxCommandEvent &event);
	void OnEditIndentRed (wxCommandEvent &event);
	void OnEditSelectAll (wxCommandEvent &event);
	void OnEditSelectLine (wxCommandEvent &event);
	void OnIndentGuide (wxCommandEvent &event);
	void OnLineNumber (wxCommandEvent &event);
	void OnLongLineOn (wxCommandEvent &event);
	void OnWhiteSpace (wxCommandEvent &event);
	void OnFoldToggle (wxCommandEvent &event);
	void OnSetOverType (wxCommandEvent &event);
	void OnSetReadOnly (wxCommandEvent &event);
	void OnWrapmodeOn (wxCommandEvent &event);
	void OnMarginClick (wxScintillaEvent &event);
	void OnCharAdded  (wxScintillaEvent &event);

	bool InitializePrefs (const wxString &name);

private:
	wxString m_filename;

	int m_LineNrID;
	int m_LineNrMargin;
	int m_FoldingID;
	int m_FoldingMargin;
	int m_DividerID;

	DECLARE_EVENT_TABLE()
};

#endif //_CODE_EDITOR_H_
