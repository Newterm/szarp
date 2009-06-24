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

#include <wx/wxprec.h>

#include "codeeditor.h"
#include "ids.h"

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

BEGIN_EVENT_TABLE (CodeEditor, wxScintilla)
    EVT_SIZE (                         CodeEditor::OnSize)
    EVT_MENU (wxID_CLEAR,              CodeEditor::OnEditClear)
    EVT_MENU (wxID_CUT,                CodeEditor::OnEditCut)
    EVT_MENU (wxID_COPY,               CodeEditor::OnEditCopy)
    EVT_MENU (wxID_PASTE,              CodeEditor::OnEditPaste)
    EVT_MENU (codeEditorID_INDENTINC,          CodeEditor::OnEditIndentInc)
    EVT_MENU (codeEditorID_INDENTRED,          CodeEditor::OnEditIndentRed)
    EVT_MENU (wxID_SELECTALL,          CodeEditor::OnEditSelectAll)
    EVT_MENU (codeEditorID_SELECTLINE,         CodeEditor::OnEditSelectLine)
    EVT_MENU (wxID_REDO,               CodeEditor::OnEditRedo)
    EVT_MENU (wxID_UNDO,               CodeEditor::OnEditUndo)
    EVT_MENU (codeEditorID_BRACEMATCH,         CodeEditor::OnBraceMatch)
    EVT_MENU (codeEditorID_INDENTGUIDE,        CodeEditor::OnIndentGuide)
    EVT_MENU (codeEditorID_LINENUMBER,         CodeEditor::OnLineNumber)
    EVT_MENU (codeEditorID_LONGLINEON,         CodeEditor::OnLongLineOn)
    EVT_MENU (codeEditorID_WHITESPACE,         CodeEditor::OnWhiteSpace)
    EVT_MENU (codeEditorID_FOLDTOGGLE,         CodeEditor::OnFoldToggle)
    EVT_MENU (codeEditorID_OVERTYPE,           CodeEditor::OnSetOverType)
    EVT_MENU (codeEditorID_READONLY,           CodeEditor::OnSetReadOnly)
    EVT_MENU (codeEditorID_WRAPMODEON,         CodeEditor::OnWrapmodeOn)
//    EVT_SCI_MARGINCLICK (-1,           CodeEditor::OnMarginClick)
//    EVT_SCI_CHARADDED (-1,             CodeEditor::OnCharAdded)
END_EVENT_TABLE()

CodeEditor::CodeEditor (wxWindow *parent, wxWindowID id,
            const wxPoint &pos,
            const wxSize &size,
            long style)
    : wxScintilla (parent, id, pos, size, style) {

    m_filename = _T("");

    m_LineNrID = 0;
    m_LineNrMargin = TextWidth (wxSCI_STYLE_LINENUMBER, _T("_999999"));
    m_FoldingID = 1;
    m_FoldingMargin = 16;
    m_DividerID = 2;

    SetViewEOL (false);
    SetIndentationGuides (true);
    SetMarginWidth (m_LineNrID,
                    true? m_LineNrMargin: 0);
    SetEdgeMode (wxSCI_EDGE_LINE);
    SetViewWhiteSpace (wxSCI_WS_INVISIBLE);
    SetOvertype (true);
    SetReadOnly (false);
    SetWrapMode (true?
                 wxSCI_WRAP_WORD: wxSCI_WRAP_NONE);
    wxFont font (10, wxTELETYPE, wxNORMAL, wxNORMAL);
    StyleSetFont (wxSCI_STYLE_DEFAULT, font);
    StyleSetForeground (wxSCI_STYLE_DEFAULT, wxColour (_T("BLACK")));
    StyleSetBackground (wxSCI_STYLE_DEFAULT, wxColour (_T("WHITE")));
    StyleSetForeground (wxSCI_STYLE_LINENUMBER, wxColour (_T("DARK GREY")));
    StyleSetBackground (wxSCI_STYLE_LINENUMBER, wxColour (_T("WHITE")));
    StyleSetForeground(wxSCI_STYLE_INDENTGUIDE, wxColour (_T("DARK GREY")));
    InitializePrefs (_T("Lua"));

    SetVisiblePolicy (wxSCI_VISIBLE_STRICT|wxSCI_VISIBLE_SLOP, 1);
    SetXCaretPolicy (wxSCI_CARET_EVEN|wxSCI_VISIBLE_STRICT|wxSCI_CARET_SLOP, 1);
    SetYCaretPolicy (wxSCI_CARET_EVEN|wxSCI_VISIBLE_STRICT|wxSCI_CARET_SLOP, 1);

    MarkerDefine (wxSCI_MARKNUM_FOLDER, wxSCI_MARK_BOXPLUS);
    MarkerSetBackground (wxSCI_MARKNUM_FOLDER, wxColour (_T("BLACK")));
    MarkerSetForeground (wxSCI_MARKNUM_FOLDER, wxColour (_T("WHITE")));
    MarkerDefine (wxSCI_MARKNUM_FOLDEROPEN, wxSCI_MARK_BOXMINUS);
    MarkerSetBackground (wxSCI_MARKNUM_FOLDEROPEN, wxColour (_T("BLACK")));
    MarkerSetForeground (wxSCI_MARKNUM_FOLDEROPEN, wxColour (_T("WHITE")));
    MarkerDefine (wxSCI_MARKNUM_FOLDERSUB, wxSCI_MARK_EMPTY);
    MarkerDefine (wxSCI_MARKNUM_FOLDEREND, wxSCI_MARK_SHORTARROW);
    MarkerDefine (wxSCI_MARKNUM_FOLDEROPENMID, wxSCI_MARK_ARROWDOWN);
    MarkerDefine (wxSCI_MARKNUM_FOLDERMIDTAIL, wxSCI_MARK_EMPTY);
    MarkerDefine (wxSCI_MARKNUM_FOLDERTAIL, wxSCI_MARK_EMPTY);

#if !defined(__WXGTK__)
    CmdKeyClear (wxSCI_KEY_TAB, 0);
    CmdKeyClear (wxSCI_KEY_TAB, wxSCI_SCMOD_SHIFT);
#endif
    CmdKeyClear ('A', wxSCI_SCMOD_CTRL);
#if !defined(__WXGTK__)
    CmdKeyClear ('C', wxSCI_SCMOD_CTRL);
#endif
    CmdKeyClear ('D', wxSCI_SCMOD_CTRL);
    CmdKeyClear ('D', wxSCI_SCMOD_SHIFT | wxSCI_SCMOD_CTRL);
    CmdKeyClear ('F', wxSCI_SCMOD_ALT | wxSCI_SCMOD_CTRL);
    CmdKeyClear ('L', wxSCI_SCMOD_CTRL);
    CmdKeyClear ('L', wxSCI_SCMOD_SHIFT | wxSCI_SCMOD_CTRL);
    CmdKeyClear ('T', wxSCI_SCMOD_CTRL);
    CmdKeyClear ('T', wxSCI_SCMOD_SHIFT | wxSCI_SCMOD_CTRL);
    CmdKeyClear ('U', wxSCI_SCMOD_CTRL);
    CmdKeyClear ('U', wxSCI_SCMOD_SHIFT | wxSCI_SCMOD_CTRL);
#if !defined(__WXGTK__)
    CmdKeyClear ('V', wxSCI_SCMOD_CTRL);
    CmdKeyClear ('X', wxSCI_SCMOD_CTRL);
#endif
    CmdKeyClear ('Y', wxSCI_SCMOD_CTRL);
#if !defined(__WXGTK__)
    CmdKeyClear ('Z', wxSCI_SCMOD_CTRL);
#endif

    UsePopUp (0);
    SetLayoutCache (wxSCI_CACHE_PAGE);
    SetBufferedDraw (1);

}

CodeEditor::~CodeEditor () {}

void CodeEditor::OnSize( wxSizeEvent& event ) {
    int x = GetClientSize().x +
            (true? m_LineNrMargin: 0) +
            (false? m_FoldingMargin: 0);
    if (x > 0) SetScrollWidth (x);
    event.Skip();
}

void CodeEditor::OnEditRedo (wxCommandEvent &WXUNUSED(event)) {
    if (!CanRedo()) return;
    Redo ();
}

void CodeEditor::OnEditUndo (wxCommandEvent &WXUNUSED(event)) {
    if (!CanUndo()) return;
    Undo ();
}

void CodeEditor::OnEditClear (wxCommandEvent &WXUNUSED(event)) {
    if (GetReadOnly()) return;
    Clear ();
}

void CodeEditor::OnEditCut (wxCommandEvent &WXUNUSED(event)) {
    if (GetReadOnly() || (GetSelectionEnd()-GetSelectionStart() <= 0)) return;
    Cut ();
}

void CodeEditor::OnEditCopy (wxCommandEvent &WXUNUSED(event)) {
    if (GetSelectionEnd()-GetSelectionStart() <= 0) return;
    Copy ();
}

void CodeEditor::OnEditPaste (wxCommandEvent &WXUNUSED(event)) {
    if (!CanPaste()) return;
    Paste ();
}

void CodeEditor::OnBraceMatch (wxCommandEvent &WXUNUSED(event)) {
    int min = GetCurrentPos ();
    int max = BraceMatch (min);
    if (max > (min+1)) {
        BraceHighlight (min+1, max);
        SetSelection (min+1, max);
    }else{
        BraceBadLight (min);
    }
}

void CodeEditor::OnEditIndentInc (wxCommandEvent &WXUNUSED(event)) {
    CmdKeyExecute (wxSCI_CMD_TAB);
}

void CodeEditor::OnEditIndentRed (wxCommandEvent &WXUNUSED(event)) {
    CmdKeyExecute (wxSCI_CMD_DELETEBACK);
}

void CodeEditor::OnEditSelectAll (wxCommandEvent &WXUNUSED(event)) {
    SetSelection (0, GetLength());
}

void CodeEditor::OnEditSelectLine (wxCommandEvent &WXUNUSED(event)) {
    int lineStart = PositionFromLine (GetCurrentLine());
    int lineEnd = PositionFromLine (GetCurrentLine() + 1);
    SetSelection (lineStart, lineEnd);
}

void CodeEditor::OnIndentGuide (wxCommandEvent &WXUNUSED(event)) {
    SetIndentationGuides (!GetIndentationGuides());
}

void CodeEditor::OnLineNumber (wxCommandEvent &WXUNUSED(event)) {
    SetMarginWidth (m_LineNrID,
                    GetMarginWidth (m_LineNrID) == 0? m_LineNrMargin: 0);
}

void CodeEditor::OnLongLineOn (wxCommandEvent &WXUNUSED(event)) {
    SetEdgeMode (GetEdgeMode() == 0? wxSCI_EDGE_LINE: wxSCI_EDGE_NONE);
}

void CodeEditor::OnWhiteSpace (wxCommandEvent &WXUNUSED(event)) {
    SetViewWhiteSpace (GetViewWhiteSpace() == 0?
                       wxSCI_WS_VISIBLEALWAYS: wxSCI_WS_INVISIBLE);
}

void CodeEditor::OnFoldToggle (wxCommandEvent &WXUNUSED(event)) {
    ToggleFold (GetFoldParent(GetCurrentLine()));
}

void CodeEditor::OnSetOverType (wxCommandEvent &WXUNUSED(event)) {
    SetOvertype (!GetOvertype());
}

void CodeEditor::OnSetReadOnly (wxCommandEvent &WXUNUSED(event)) {
    SetReadOnly (!GetReadOnly());
}

void CodeEditor::OnWrapmodeOn (wxCommandEvent &WXUNUSED(event)) {
    SetWrapMode (GetWrapMode() == 0? wxSCI_WRAP_WORD: wxSCI_WRAP_NONE);
}

//! misc
void CodeEditor::OnMarginClick (wxScintillaEvent &event) {
    if (event.GetMargin() == 2) {
        int lineClick = LineFromPosition (event.GetPosition());
        int levelClick = GetFoldLevel (lineClick);
        if ((levelClick & wxSCI_FOLDLEVELHEADERFLAG) > 0) {
            ToggleFold (lineClick);
        }
    }
}

void CodeEditor::OnCharAdded (wxScintillaEvent &event) {
    char chr = event.GetKey();
    int currentLine = GetCurrentLine();
    // Change this if support for mac files with \r is needed
    if (chr == '\n') {
        int lineInd = 0;
        if (currentLine > 0) {
            lineInd = GetLineIndentation(currentLine - 1);
        }
        if (lineInd == 0) return;
        SetLineIndentation (currentLine, lineInd);
        GotoPos(PositionFromLine (currentLine) + lineInd);
    }
}

bool CodeEditor::InitializePrefs (const wxString &name) {

	// initialize styles
	StyleClearAll();

	// set lexer and language
	SetLexer (wxSCI_LEX_LUA);

	// set margin for line numbers
	SetMarginType (m_LineNrID, wxSCI_MARGIN_NUMBER);
	StyleSetForeground (wxSCI_STYLE_LINENUMBER, wxColour (_T("DARK GREY")));
	StyleSetBackground (wxSCI_STYLE_LINENUMBER, wxColour (_T("WHITE")));
	SetMarginWidth (m_LineNrID, m_LineNrMargin);

	// set common styles
	StyleSetForeground (wxSCI_STYLE_DEFAULT, wxColour (_T("DARK GREY")));
	StyleSetForeground (wxSCI_STYLE_INDENTGUIDE, wxColour (_T("DARK GREY")));

	int keywordnr = 0;
	int Nr = 0;
	const wxChar *pwords;

// 	{
// 		wxFont font (10, wxTELETYPE, wxNORMAL, wxNORMAL, false);
// 		StyleSetFont (Nr, font);
// 		StyleSetForeground (Nr, wxColour (_T("CYAN")));
// 		StyleSetBold (Nr, 1);
// 		StyleSetItalic (Nr, 0);
// 		StyleSetUnderline (Nr, 0);
// 		StyleSetVisible (Nr, 1);
// 		StyleSetCase (Nr, 0);
// // 		pwords = _T("* / - +");
// // 		SetKeyWords (keywordnr, pwords);
// 	}
	Nr++;
 	{
// 		wxFont font (10, wxTELETYPE, wxNORMAL, wxNORMAL, false);
// 		StyleSetFont (Nr, font);
// 		StyleSetForeground (Nr, wxColour (_T("GREEN")));
//		StyleSetBold (Nr, 1);
// 		StyleSetItalic (Nr, 0);
// 		StyleSetUnderline (Nr, 0);
//		StyleSetVisible (Nr, 1);
// 		StyleSetCase (Nr, 0);
//  		pwords = _T("* / - + ( )");
// 		SetKeyWords (keywordnr, pwords);
 	}
	Nr++;
 	{
 		wxFont font (10, wxTELETYPE, wxNORMAL, wxNORMAL, false);
 		StyleSetFont (Nr, font);
 		StyleSetForeground (Nr, wxColour (_T("BLUE")));
 		StyleSetBold (Nr, 1);
 		StyleSetItalic (Nr, 0);
 		StyleSetUnderline (Nr, 0);
 		StyleSetVisible (Nr, 1);
 		StyleSetCase (Nr, 0);
 	}
	Nr++;
// 	{
// 		wxFont font (10, wxTELETYPE, wxNORMAL, wxNORMAL, false);
// 		StyleSetFont (Nr, font);
// 		StyleSetForeground (Nr, wxColour (_T("BLUE")));
// 		StyleSetBold (Nr, 1);
// 		StyleSetItalic (Nr, 0);
// 		StyleSetUnderline (Nr, 0);
// 		StyleSetVisible (Nr, 1);
// 		StyleSetCase (Nr, 0);
// 		pwords = _T("* / - + = ~");
// 		SetKeyWords (keywordnr, pwords);
// 	}
	Nr++;
	{ //4 Number
		wxFont font (10, wxTELETYPE, wxNORMAL, wxNORMAL, false);
		StyleSetFont (Nr, font);
		StyleSetForeground (Nr, wxColour (_T("NAVY")));
		StyleSetBold (Nr, 1);
		StyleSetItalic (Nr, 0);
		StyleSetUnderline (Nr, 0);
		StyleSetVisible (Nr, 1);
		StyleSetCase (Nr, 0);
// 		pwords = _T("[ ] ;");
// 		SetKeyWords (keywordnr, pwords);
	}
	Nr++;
	{
		wxFont font (10, wxTELETYPE, wxNORMAL, wxNORMAL, false);
		StyleSetFont (Nr, font);
		StyleSetForeground (Nr, wxColour (_T("BROWN")));
		StyleSetBold (Nr, 1);
		StyleSetItalic (Nr, 0);
		StyleSetUnderline (Nr, 0);
		StyleSetVisible (Nr, 1);
		StyleSetCase (Nr, 0);
// 		pwords = _T("* / - + < >");
// 		SetKeyWords (keywordnr, pwords);
	}
	Nr++;
	{//6 ""
		wxFont font (10, wxTELETYPE, wxNORMAL, wxNORMAL, false);
		StyleSetFont (Nr, font);
		StyleSetForeground (Nr, wxColour (_T("ORANGE")));
		StyleSetBold (Nr, 1);
		StyleSetItalic (Nr, 0);
		StyleSetUnderline (Nr, 0);
		StyleSetVisible (Nr, 1);
		StyleSetCase (Nr, 0);
	}
	Nr++;
	{//7 ''
		wxFont font (10, wxTELETYPE, wxNORMAL, wxNORMAL, false);
		StyleSetFont (Nr, font);
		StyleSetForeground (Nr, wxColour (_T("ORANGE")));
		StyleSetBold (Nr, 1);
		StyleSetItalic (Nr, 0);
		StyleSetUnderline (Nr, 0);
		StyleSetVisible (Nr, 1);
		StyleSetCase (Nr, 0);
	}
	Nr++;
	{
		wxFont font (10, wxTELETYPE, wxNORMAL, wxNORMAL, false);
		StyleSetFont (Nr, font);
		StyleSetForeground (Nr, wxColour (_T("YELLOW")));
		StyleSetBold (Nr, 1);
		StyleSetItalic (Nr, 0);
		StyleSetUnderline (Nr, 0);
		StyleSetVisible (Nr, 1);
		StyleSetCase (Nr, 0);
		pwords = _T("* / - + ^ % : #");
		SetKeyWords (keywordnr, pwords);
	}
	

	// set margin as unused
	SetMarginType (m_DividerID, wxSCI_MARGIN_SYMBOL);
	SetMarginWidth (m_DividerID, 8);
	SetMarginSensitive (m_DividerID, false);

	// folding
	SetProperty (_T("fold"), _T("0"));

	// set spaces and indention
	SetTabWidth (4);
	SetUseTabs (true);
	SetTabIndents (true);
	SetBackSpaceUnIndents (true);
	SetIndent (4);

	// others
	SetViewEOL (false);
	SetIndentationGuides (true);
	SetEdgeColumn (80);
	SetEdgeMode (wxSCI_EDGE_NONE);
	SetViewWhiteSpace (wxSCI_WS_INVISIBLE);
	SetOvertype (false);
	SetReadOnly (false);
	//SetWrapMode (wxSCI_WRAP_NONE);
	SetWrapMode (wxSCI_WRAP_WORD);

	return true;
}

