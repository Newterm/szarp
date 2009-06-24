//////////////////////////////////////////////////////////////////////////////
// File:        edit.cpp
// Purpose:     wxScintilla test module
// Maintainer:  Otto Wyss
// Created:     2003-09-01
// RCS-ID:      $Id$
// Copyright:   (c) 2004wxCode
// Licence:     wxWindows
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// informations
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// headers
//----------------------------------------------------------------------------

// For compilers that support precompilation, includes <wx/wx.h>.
#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all 'standard' wxWindows headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

//! wxWindows headers
#include <wx/file.h>     // raw file io support
#include <wx/filename.h> // filename support

//! application headers
#include "defsext.h"     // additional definitions

#include "edit.h"        // edit module


//----------------------------------------------------------------------------
// resources
//----------------------------------------------------------------------------


//============================================================================
// declarations
//============================================================================


//============================================================================
// implementation
//============================================================================

//----------------------------------------------------------------------------
// Edit
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE (Edit, wxScintilla)
    // common
    EVT_SIZE (                         Edit::OnSize)
    // edit
    EVT_MENU (wxID_CLEAR,              Edit::OnEditClear)
    EVT_MENU (wxID_CUT,                Edit::OnEditCut)
    EVT_MENU (wxID_COPY,               Edit::OnEditCopy)
    EVT_MENU (wxID_PASTE,              Edit::OnEditPaste)
    EVT_MENU (myID_INDENTINC,          Edit::OnEditIndentInc)
    EVT_MENU (myID_INDENTRED,          Edit::OnEditIndentRed)
    EVT_MENU (wxID_SELECTALL,          Edit::OnEditSelectAll)
    EVT_MENU (myID_SELECTLINE,         Edit::OnEditSelectLine)
    EVT_MENU (wxID_REDO,               Edit::OnEditRedo)
    EVT_MENU (wxID_UNDO,               Edit::OnEditUndo)
    // find
    EVT_MENU (wxID_FIND,               Edit::OnFind)
    EVT_MENU (myID_FINDNEXT,           Edit::OnFindNext)
    EVT_MENU (myID_REPLACE,            Edit::OnReplace)
    EVT_MENU (myID_REPLACENEXT,        Edit::OnReplaceNext)
    EVT_MENU (myID_BRACEMATCH,         Edit::OnBraceMatch)
    EVT_MENU (myID_GOTO,               Edit::OnGoto)
    // view
    EVT_MENU_RANGE (myID_HILIGHTFIRST, myID_HILIGHTLAST,
                                       Edit::OnHilightLang)
    EVT_MENU (myID_DISPLAYEOL,         Edit::OnDisplayEOL)
    EVT_MENU (myID_INDENTGUIDE,        Edit::OnIndentGuide)
    EVT_MENU (myID_LINENUMBER,         Edit::OnLineNumber)
    EVT_MENU (myID_LONGLINEON,         Edit::OnLongLineOn)
    EVT_MENU (myID_WHITESPACE,         Edit::OnWhiteSpace)
    EVT_MENU (myID_FOLDTOGGLE,         Edit::OnFoldToggle)
    EVT_MENU (myID_OVERTYPE,           Edit::OnSetOverType)
    EVT_MENU (myID_READONLY,           Edit::OnSetReadOnly)
    EVT_MENU (myID_WRAPMODEON,         Edit::OnWrapmodeOn)
    EVT_MENU (myID_CHARSETANSI,        Edit::OnUseCharset)
    EVT_MENU (myID_CHARSETMAC,         Edit::OnUseCharset)
    // extra
    EVT_MENU (myID_CHANGELOWER,        Edit::OnChangeCase)
    EVT_MENU (myID_CHANGEUPPER,        Edit::OnChangeCase)
    EVT_MENU (myID_CONVERTCR,          Edit::OnConvertEOL)
    EVT_MENU (myID_CONVERTCRLF,        Edit::OnConvertEOL)
    EVT_MENU (myID_CONVERTLF,          Edit::OnConvertEOL)
    // scintilla
    EVT_SCI_MARGINCLICK (-1,           Edit::OnMarginClick)
    EVT_SCI_CHARADDED (-1,             Edit::OnCharAdded)
END_EVENT_TABLE()

Edit::Edit (wxWindow *parent, wxWindowID id,
            const wxPoint &pos,
            const wxSize &size,
            long style)
    : wxScintilla (parent, id, pos, size, style) {

    m_filename = _T("");
    m_language = NULL;

    m_LineNrID = 0;
    m_LineNrMargin = TextWidth (wxSCI_STYLE_LINENUMBER, _T("_999999"));
    m_FoldingID = 1;
    m_FoldingMargin = 16;
    m_DividerID = 2;

    // default font for all styles
    SetViewEOL (g_CommonPrefs.displayEOLEnable);
    SetIndentationGuides (g_CommonPrefs.indentGuideEnable);
    SetMarginWidth (m_LineNrID,
                    g_CommonPrefs.lineNumberEnable? m_LineNrMargin: 0);
    SetEdgeMode (g_CommonPrefs.longLineOnEnable?
                 wxSCI_EDGE_LINE: wxSCI_EDGE_NONE);
    SetViewWhiteSpace (g_CommonPrefs.whiteSpaceEnable?
                       wxSCI_WS_VISIBLEALWAYS: wxSCI_WS_INVISIBLE);
    SetOvertype (g_CommonPrefs.overTypeInitial);
    SetReadOnly (g_CommonPrefs.readOnlyInitial);
    SetWrapMode (g_CommonPrefs.wrapModeInitial?
                 wxSCI_WRAP_WORD: wxSCI_WRAP_NONE);
    wxFont font (10, wxTELETYPE, wxNORMAL, wxNORMAL);
    StyleSetFont (wxSCI_STYLE_DEFAULT, font);
    StyleSetForeground (wxSCI_STYLE_DEFAULT, wxColour (_T("BLACK")));
    StyleSetBackground (wxSCI_STYLE_DEFAULT, wxColour (_T("WHITE")));
    StyleSetForeground (wxSCI_STYLE_LINENUMBER, wxColour (_T("DARK GREY")));
    StyleSetBackground (wxSCI_STYLE_LINENUMBER, wxColour (_T("WHITE")));
    StyleSetForeground(wxSCI_STYLE_INDENTGUIDE, wxColour (_T("DARK GREY")));
    InitializePrefs (DEFAULT_LANGUAGE);

    // set visibility
    SetVisiblePolicy (wxSCI_VISIBLE_STRICT|wxSCI_VISIBLE_SLOP, 1);
    SetXCaretPolicy (wxSCI_CARET_EVEN|wxSCI_VISIBLE_STRICT|wxSCI_CARET_SLOP, 1);
    SetYCaretPolicy (wxSCI_CARET_EVEN|wxSCI_VISIBLE_STRICT|wxSCI_CARET_SLOP, 1);

    // markers
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

    // clear wrong default keys
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

    // miscelaneous
    UsePopUp (0);
    SetLayoutCache (wxSCI_CACHE_PAGE);
    SetBufferedDraw (1);

}

Edit::~Edit () {}

//----------------------------------------------------------------------------
// common event handlers
void Edit::OnSize( wxSizeEvent& event ) {
    int x = GetClientSize().x +
            (g_CommonPrefs.lineNumberEnable? m_LineNrMargin: 0) +
            (g_CommonPrefs.foldEnable? m_FoldingMargin: 0);
    if (x > 0) SetScrollWidth (x);
    event.Skip();
}

// edit event handlers
void Edit::OnEditRedo (wxCommandEvent &WXUNUSED(event)) {
    if (!CanRedo()) return;
    Redo ();
}

void Edit::OnEditUndo (wxCommandEvent &WXUNUSED(event)) {
    if (!CanUndo()) return;
    Undo ();
}

void Edit::OnEditClear (wxCommandEvent &WXUNUSED(event)) {
    if (GetReadOnly()) return;
    Clear ();
}

void Edit::OnEditCut (wxCommandEvent &WXUNUSED(event)) {
    if (GetReadOnly() || (GetSelectionEnd()-GetSelectionStart() <= 0)) return;
    Cut ();
}

void Edit::OnEditCopy (wxCommandEvent &WXUNUSED(event)) {
    if (GetSelectionEnd()-GetSelectionStart() <= 0) return;
    Copy ();
}

void Edit::OnEditPaste (wxCommandEvent &WXUNUSED(event)) {
    if (!CanPaste()) return;
    Paste ();
}

void Edit::OnFind (wxCommandEvent &WXUNUSED(event)) {
}

void Edit::OnFindNext (wxCommandEvent &WXUNUSED(event)) {
}

void Edit::OnReplace (wxCommandEvent &WXUNUSED(event)) {
}

void Edit::OnReplaceNext (wxCommandEvent &WXUNUSED(event)) {
}

void Edit::OnBraceMatch (wxCommandEvent &WXUNUSED(event)) {
    int min = GetCurrentPos ();
    int max = BraceMatch (min);
    if (max > (min+1)) {
        BraceHighlight (min+1, max);
        SetSelection (min+1, max);
    }else{
        BraceBadLight (min);
    }
}

void Edit::OnGoto (wxCommandEvent &WXUNUSED(event)) {
}

void Edit::OnEditIndentInc (wxCommandEvent &WXUNUSED(event)) {
    CmdKeyExecute (wxSCI_CMD_TAB);
}

void Edit::OnEditIndentRed (wxCommandEvent &WXUNUSED(event)) {
    CmdKeyExecute (wxSCI_CMD_DELETEBACK);
}

void Edit::OnEditSelectAll (wxCommandEvent &WXUNUSED(event)) {
    SetSelection (0, GetLength());
}

void Edit::OnEditSelectLine (wxCommandEvent &WXUNUSED(event)) {
    int lineStart = PositionFromLine (GetCurrentLine());
    int lineEnd = PositionFromLine (GetCurrentLine() + 1);
    SetSelection (lineStart, lineEnd);
}

void Edit::OnHilightLang (wxCommandEvent &event) {
    InitializePrefs (g_LanguagePrefs [event.GetId() - myID_HILIGHTFIRST].name);
}

void Edit::OnDisplayEOL (wxCommandEvent &WXUNUSED(event)) {
    SetViewEOL (!GetViewEOL());
}

void Edit::OnIndentGuide (wxCommandEvent &WXUNUSED(event)) {
    SetIndentationGuides (!GetIndentationGuides());
}

void Edit::OnLineNumber (wxCommandEvent &WXUNUSED(event)) {
    SetMarginWidth (m_LineNrID,
                    GetMarginWidth (m_LineNrID) == 0? m_LineNrMargin: 0);
}

void Edit::OnLongLineOn (wxCommandEvent &WXUNUSED(event)) {
    SetEdgeMode (GetEdgeMode() == 0? wxSCI_EDGE_LINE: wxSCI_EDGE_NONE);
}

void Edit::OnWhiteSpace (wxCommandEvent &WXUNUSED(event)) {
    SetViewWhiteSpace (GetViewWhiteSpace() == 0?
                       wxSCI_WS_VISIBLEALWAYS: wxSCI_WS_INVISIBLE);
}

void Edit::OnFoldToggle (wxCommandEvent &WXUNUSED(event)) {
    ToggleFold (GetFoldParent(GetCurrentLine()));
}

void Edit::OnSetOverType (wxCommandEvent &WXUNUSED(event)) {
    SetOvertype (!GetOvertype());
}

void Edit::OnSetReadOnly (wxCommandEvent &WXUNUSED(event)) {
    SetReadOnly (!GetReadOnly());
}

void Edit::OnWrapmodeOn (wxCommandEvent &WXUNUSED(event)) {
    SetWrapMode (GetWrapMode() == 0? wxSCI_WRAP_WORD: wxSCI_WRAP_NONE);
}

void Edit::OnUseCharset (wxCommandEvent &event) {
    int Nr;
    int charset = GetCodePage();
    switch (event.GetId()) {
        case myID_CHARSETANSI: {charset = wxSCI_CHARSET_ANSI; break;}
        case myID_CHARSETMAC: {charset = wxSCI_CHARSET_ANSI; break;}
    }
    for (Nr = 0; Nr < wxSCI_STYLE_LASTPREDEFINED; Nr++) {
        StyleSetCharacterSet (Nr, charset);
    }
    SetCodePage (charset);
}

void Edit::OnChangeCase (wxCommandEvent &event) {
    switch (event.GetId()) {
        case myID_CHANGELOWER: {
            CmdKeyExecute (wxSCI_CMD_LOWERCASE);
            break;
        }
        case myID_CHANGEUPPER: {
            CmdKeyExecute (wxSCI_CMD_UPPERCASE);
            break;
        }
    }
}

void Edit::OnConvertEOL (wxCommandEvent &event) {
    int eolMode = GetEOLMode();
    switch (event.GetId()) {
        case myID_CONVERTCR: { eolMode = wxSCI_EOL_CR; break;}
        case myID_CONVERTCRLF: { eolMode = wxSCI_EOL_CRLF; break;}
        case myID_CONVERTLF: { eolMode = wxSCI_EOL_LF; break;}
    }
    ConvertEOLs (eolMode);
    SetEOLMode (eolMode);
}

//! misc
void Edit::OnMarginClick (wxScintillaEvent &event) {
    if (event.GetMargin() == 2) {
        int lineClick = LineFromPosition (event.GetPosition());
        int levelClick = GetFoldLevel (lineClick);
        if ((levelClick & wxSCI_FOLDLEVELHEADERFLAG) > 0) {
            ToggleFold (lineClick);
        }
    }
}

void Edit::OnCharAdded (wxScintillaEvent &event) {
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


//----------------------------------------------------------------------------
// private functions
wxString Edit::DeterminePrefs (const wxString &filename) {

    LanguageInfo const* curInfo;

    // determine language from filepatterns
    int languageNr;
    for (languageNr = 0; languageNr < g_LanguagePrefsSize; languageNr++) {
        curInfo = &g_LanguagePrefs [languageNr];
        wxString filepattern = curInfo->filepattern;
        filepattern.Lower();
        while (!filepattern.IsEmpty()) {
            wxString cur = filepattern.BeforeFirst (';');
            if ((cur == filename) ||
                (cur == (filename.BeforeLast ('.') + _T(".*"))) ||
                (cur == (_T("*.") + filename.AfterLast ('.')))) {
                return curInfo->name;
            }
            filepattern = filepattern.AfterFirst (';');
        }
    }
    return wxEmptyString;

}

bool Edit::InitializePrefs (const wxString &name) {

    // initialize styles
    StyleClearAll();
    LanguageInfo const* curInfo = NULL;

    // determine language
    bool found = false;
    int languageNr;
    for (languageNr = 0; languageNr < g_LanguagePrefsSize; languageNr++) {
        curInfo = &g_LanguagePrefs [languageNr];
        if (curInfo->name == name) {
            found = true;
            break;
        }
    }
    if (!found) return false;

    // set lexer and language
    SetLexer (curInfo->lexer);
    m_language = curInfo;

    // set margin for line numbers
    SetMarginType (m_LineNrID, wxSCI_MARGIN_NUMBER);
    StyleSetForeground (wxSCI_STYLE_LINENUMBER, wxColour (_T("DARK GREY")));
    StyleSetBackground (wxSCI_STYLE_LINENUMBER, wxColour (_T("WHITE")));
    SetMarginWidth (m_LineNrID,
                    g_CommonPrefs.lineNumberEnable? m_LineNrMargin: 0);

    // set common styles
    StyleSetForeground (wxSCI_STYLE_DEFAULT, wxColour (_T("DARK GREY")));
    StyleSetForeground (wxSCI_STYLE_INDENTGUIDE, wxColour (_T("DARK GREY")));

    // initialize settings
    if (g_CommonPrefs.syntaxEnable) {
        int keywordnr = 0;
        int Nr;
        for (Nr = 0; Nr < STYLE_TYPES_COUNT; Nr++) {
            if (curInfo->styles[Nr].type == -1) continue;
            const StyleInfo &curType = g_StylePrefs [curInfo->styles[Nr].type];
            wxFont font (curType.fontsize, wxTELETYPE, wxNORMAL, wxNORMAL, false,
                         curType.fontname);
            StyleSetFont (Nr, font);
            if (curType.foreground) {
                StyleSetForeground (Nr, wxColour (curType.foreground));
            }
            if (curType.background) {
                StyleSetBackground (Nr, wxColour (curType.background));
            }
            StyleSetBold (Nr, (curType.fontstyle & TOKEN_STYLE_BOLD) > 0);
            StyleSetItalic (Nr, (curType.fontstyle & TOKEN_STYLE_ITALIC) > 0);
            StyleSetUnderline (Nr, (curType.fontstyle & TOKEN_STYLE_UNDERL) > 0);
            StyleSetVisible (Nr, (curType.fontstyle & TOKEN_STYLE_HIDDEN) == 0);
            StyleSetCase (Nr, curType.lettercase);
            const wxChar *pwords = curInfo->styles[Nr].words;
            if (pwords) {
                SetKeyWords (keywordnr, pwords);
                keywordnr += 1;
            }
        }
    }

    // set margin as unused
    SetMarginType (m_DividerID, wxSCI_MARGIN_SYMBOL);
    SetMarginWidth (m_DividerID, 8);
    SetMarginSensitive (m_DividerID, false);

    // folding
    SetMarginType (m_FoldingID, wxSCI_MARGIN_SYMBOL);
    SetMarginMask (m_FoldingID, wxSCI_MASK_FOLDERS);
    StyleSetBackground (m_FoldingID, wxColour (_T("WHITE")));
    SetMarginWidth (m_FoldingID, 0);
    SetMarginSensitive (m_FoldingID, false);
    if (g_CommonPrefs.foldEnable) {
        SetMarginWidth (m_FoldingID, curInfo->folds != 0? m_FoldingMargin: 0);
        SetMarginSensitive (m_FoldingID, curInfo->folds != 0);
        SetProperty (_T("fold"), curInfo->folds != 0? _T("1"): _T("0"));
        SetProperty (_T("fold.comment"),
                     (curInfo->folds & FOLD_TYPE_COMMENT) > 0? _T("1"): _T("0"));
        SetProperty (_T("fold.compact"),
                     (curInfo->folds & FOLD_TYPE_COMPACT) > 0? _T("1"): _T("0"));
        SetProperty (_T("fold.preprocessor"),
                     (curInfo->folds & FOLD_TYPE_PREPROC) > 0? _T("1"): _T("0"));
        SetProperty (_T("fold.html"),
                     (curInfo->folds & FOLD_TYPE_HTML) > 0? _T("1"): _T("0"));
        SetProperty (_T("fold.html.preprocessor"),
                     (curInfo->folds & FOLD_TYPE_HTMLPREP) > 0? _T("1"): _T("0"));
        SetProperty (_T("fold.comment.python"),
                     (curInfo->folds & FOLD_TYPE_COMMENTPY) > 0? _T("1"): _T("0"));
        SetProperty (_T("fold.quotes.python"),
                     (curInfo->folds & FOLD_TYPE_QUOTESPY) > 0? _T("1"): _T("0"));
    }
    SetFoldFlags (wxSCI_FOLDFLAG_LINEBEFORE_CONTRACTED |
                  wxSCI_FOLDFLAG_LINEAFTER_CONTRACTED);

    // set spaces and indention
    SetTabWidth (4);
    SetUseTabs (false);
    SetTabIndents (true);
    SetBackSpaceUnIndents (true);
    SetIndent (g_CommonPrefs.indentEnable? 4: 0);

    // others
    SetViewEOL (g_CommonPrefs.displayEOLEnable);
    SetIndentationGuides (g_CommonPrefs.indentGuideEnable);
    SetEdgeColumn (80);
    SetEdgeMode (g_CommonPrefs.longLineOnEnable? wxSCI_EDGE_LINE: wxSCI_EDGE_NONE);
    SetViewWhiteSpace (g_CommonPrefs.whiteSpaceEnable?
                       wxSCI_WS_VISIBLEALWAYS: wxSCI_WS_INVISIBLE);
    SetOvertype (g_CommonPrefs.overTypeInitial);
    SetReadOnly (g_CommonPrefs.readOnlyInitial);
    SetWrapMode (g_CommonPrefs.wrapModeInitial?
                 wxSCI_WRAP_WORD: wxSCI_WRAP_NONE);

    return true;
}

bool Edit::LoadFile () {

    // get filname
    if (!m_filename) {
        wxFileDialog dlg (this, _T("Open file"), _T(""), _T(""),
                          _T("Any file (*)|*"), wxOPEN | wxFILE_MUST_EXIST | wxCHANGE_DIR);
        if (dlg.ShowModal() != wxID_OK) return false;
        m_filename = dlg.GetPath();
    }

    // load file
    return LoadFile (m_filename);
}

bool Edit::LoadFile (const wxString &filename) {

    // load file in edit and clear undo
    if (!filename.IsEmpty()) m_filename = filename;
    if (!wxScintilla::LoadFile (m_filename)) return false;

    // determine lexer language
    wxFileName fname (m_filename);
    InitializePrefs (DeterminePrefs (fname.GetFullName()));

    return true;
}

bool Edit::SaveFile () {

    // return if no change
    if (!Modified()) return true;

    // get filname
    if (!m_filename) {
        wxFileDialog dlg (this, _T("Save file"), _T(""), _T(""), _T("Any file (*)|*"),
                          wxSAVE | wxOVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK) return false;
        m_filename = dlg.GetPath();
    }

    // save file
    return SaveFile (m_filename);
}

bool Edit::SaveFile (const wxString &filename) {

    // return if no change
    if (!Modified()) return true;

    return wxScintilla::SaveFile(filename);

}

bool Edit::Modified () {

    // return modified state
    return (GetModify() && !GetReadOnly());
}

//----------------------------------------------------------------------------
// EditProperties
//----------------------------------------------------------------------------

EditProperties::EditProperties (Edit *edit,
                                long style)
        : wxDialog (edit, -1, wxEmptyString,
                    wxDefaultPosition, wxDefaultSize,
                    style | wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {

    // sets the application title
    SetTitle (_("Properties"));
    wxString text;

    // fullname
    wxBoxSizer *fullname = new wxBoxSizer (wxHORIZONTAL);
    fullname->Add (10, 0);
    fullname->Add (new wxStaticText (this, -1, _("Full filename"),
                                     wxDefaultPosition, wxSize(80, -1)),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    fullname->Add (new wxStaticText (this, -1, edit->GetFilename()),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);

    // text info
    wxGridSizer *textinfo = new wxGridSizer (4, 0, 2);
    textinfo->Add (new wxStaticText (this, -1, _("Language"),
                                     wxDefaultPosition, wxSize(80, -1)),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    textinfo->Add (new wxStaticText (this, -1, edit->m_language->name),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
    textinfo->Add (new wxStaticText (this, -1, _("Lexer-ID: "),
                                     wxDefaultPosition, wxSize(80, -1)),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    text = wxString::Format (_T("%d"), edit->GetLexer());
    textinfo->Add (new wxStaticText (this, -1, text),
                   0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
    wxString EOLtype = _T("");
    switch (edit->GetEOLMode()) {
        case wxSCI_EOL_CR: {EOLtype = _T("CR (Unix)"); break; }
        case wxSCI_EOL_CRLF: {EOLtype = _T("CRLF (Windows)"); break; }
        case wxSCI_EOL_LF: {EOLtype = _T("CR (Macintosh)"); break; }
    }
    textinfo->Add (new wxStaticText (this, -1, _("Line endings"),
                                     wxDefaultPosition, wxSize(80, -1)),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    textinfo->Add (new wxStaticText (this, -1, EOLtype),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);

    // text info box
    wxStaticBoxSizer *textinfos = new wxStaticBoxSizer (
                     new wxStaticBox (this, -1, _("Informations")),
                     wxVERTICAL);
    textinfos->Add (textinfo, 0, wxEXPAND);
    textinfos->Add (0, 6);

    // statistic
    wxGridSizer *statistic = new wxGridSizer (4, 0, 2);
    statistic->Add (new wxStaticText (this, -1, _("Total lines"),
                                     wxDefaultPosition, wxSize(80, -1)),
                    0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    text = wxString::Format (_T("%d"), edit->GetLineCount());
    statistic->Add (new wxStaticText (this, -1, text),
                    0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
    statistic->Add (new wxStaticText (this, -1, _("Total chars"),
                                     wxDefaultPosition, wxSize(80, -1)),
                    0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    text = wxString::Format (_T("%d"), edit->GetTextLength());
    statistic->Add (new wxStaticText (this, -1, text),
                    0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
    statistic->Add (new wxStaticText (this, -1, _("Current line"),
                                     wxDefaultPosition, wxSize(80, -1)),
                    0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    text = wxString::Format (_T("%d"), edit->GetCurrentLine());
    statistic->Add (new wxStaticText (this, -1, text),
                    0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
    statistic->Add (new wxStaticText (this, -1, _("Current pos"),
                                     wxDefaultPosition, wxSize(80, -1)),
                    0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    text = wxString::Format (_T("%d"), edit->GetCurrentPos());
    statistic->Add (new wxStaticText (this, -1, text),
                    0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);

    // char/line statistics
    wxStaticBoxSizer *statistics = new wxStaticBoxSizer (
                     new wxStaticBox (this, -1, _("Statistics")),
                     wxVERTICAL);
    statistics->Add (statistic, 0, wxEXPAND);
    statistics->Add (0, 6);

    // total pane
    wxBoxSizer *totalpane = new wxBoxSizer (wxVERTICAL);
    totalpane->Add (fullname, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    totalpane->Add (0, 6);
    totalpane->Add (textinfos, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    totalpane->Add (0, 10);
    totalpane->Add (statistics, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    totalpane->Add (0, 6);
    wxButton *okButton = new wxButton (this, wxID_OK, _("OK"));
    okButton->SetDefault();
    totalpane->Add (okButton, 0, wxALIGN_CENTER | wxALL, 10);

    SetSizerAndFit (totalpane);

    ShowModal();
}

//----------------------------------------------------------------------------
// EditPrint
//----------------------------------------------------------------------------

EditPrint::EditPrint (Edit *edit, wxChar *title)
              : wxPrintout(title) {
    m_edit = edit;
    m_printed = 0;

}

bool EditPrint::OnPrintPage (int page) {

    wxDC *dc = GetDC();
    if (!dc) return false;

    // scale DC
    PrintScaling (dc);

    // print page
    if (page == 1) m_printed = 0;
    m_printed = m_edit->FormatRange (1, m_printed, m_edit->GetLength(),
                                     dc, dc, m_printRect, m_pageRect);

    return true;
}

bool EditPrint::OnBeginDocument (int startPage, int endPage) {

    if (!wxPrintout::OnBeginDocument (startPage, endPage)) {
        return false;
    }

    return true;
}

void EditPrint::GetPageInfo (int *minPage, int *maxPage, int *selPageFrom, int *selPageTo) {

    // initialize values
    *minPage = 0;
    *maxPage = 0;
    *selPageFrom = 0;
    *selPageTo = 0;

    // scale DC if possible
    wxDC *dc = GetDC();
    if (!dc) return;
    PrintScaling (dc);

    // get print page informations and convert to printer pixels
    wxSize ppiScr;
    GetPPIScreen (&ppiScr.x, &ppiScr.y);
    wxSize page = g_pageSetupData->GetPaperSize();
    page.x = static_cast<int> (page.x * ppiScr.x / 25.4);
    page.y = static_cast<int> (page.y * ppiScr.y / 25.4);
    m_pageRect = wxRect (0,
                         0,
                         page.x,
                         page.y);

    // get margins informations and convert to printer pixels
    int  top = 25; // default 25
    int  bottom = 25; // default 25
    int  left = 20; // default 20
    int  right = 20; // default 20
    wxPoint (top, left) = g_pageSetupData->GetMarginTopLeft();
    wxPoint (bottom, right) = g_pageSetupData->GetMarginBottomRight();
    top = static_cast<int> (top * ppiScr.y / 25.4);
    bottom = static_cast<int> (bottom * ppiScr.y / 25.4);
    left = static_cast<int> (left * ppiScr.x / 25.4);
    right = static_cast<int> (right * ppiScr.x / 25.4);
    m_printRect = wxRect (left,
                          top,
                          page.x - (left + right),
                          page.y - (top + bottom));

    // count pages
    while (HasPage (*maxPage)) {
        m_printed = m_edit->FormatRange (0, m_printed, m_edit->GetLength(),
                                       dc, dc, m_printRect, m_pageRect);
        *maxPage += 1;
    }
    if (*maxPage > 0) *minPage = 1;
    *selPageFrom = *minPage;
    *selPageTo = *maxPage;
    m_printed = 0;
}

bool EditPrint::HasPage (int WXUNUSED(page)) {

    return (m_printed < m_edit->GetLength());
}

bool EditPrint::PrintScaling (wxDC *dc){

    // check for dc, return if none
    if (!dc) return false;

    // get printer and screen sizing values
    wxSize ppiScr;
    GetPPIScreen (&ppiScr.x, &ppiScr.y);
    if (ppiScr.x == 0) { // most possible guess 96 dpi
        ppiScr.x = 96;
        ppiScr.y = 96;
    }
    wxSize ppiPrt;
    GetPPIPrinter (&ppiPrt.x, &ppiPrt.y);
    if (ppiPrt.x == 0) { // scaling factor to 1
        ppiPrt.x = ppiScr.x;
        ppiPrt.y = ppiScr.y;
    }
    wxSize dcSize = dc->GetSize();
    wxSize pageSize;
    GetPageSizePixels (&pageSize.x, &pageSize.y);

    // set user scale
    float scale_x = (float)(ppiPrt.x * dcSize.x) /
                    (float)(ppiScr.x * pageSize.x);
    float scale_y = (float)(ppiPrt.y * dcSize.y) /
                    (float)(ppiScr.y * pageSize.y);
    dc->SetUserScale (scale_x, scale_y);

    return true;
}

