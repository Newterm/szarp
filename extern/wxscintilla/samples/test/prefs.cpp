//////////////////////////////////////////////////////////////////////////////
// File:        prefs.cpp
// Purpose:     wxScintilla test preferences
// Maintainer:  Otto Wyss
// Created:     2003-09-01
// RCS-ID:      $Id$
// Copyright:   (c) 2004 wxCode
// Licence:     wxWindows
//////////////////////////////////////////////////////////////////////////////

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

//! wxWindows/contrib headers

//! application headers
#include "defsext.h"     // Additional definitions
#include "prefs.h"       // Preferences


//============================================================================
// declarations
//============================================================================

//----------------------------------------------------------------------------
//! language types
const CommonInfo g_CommonPrefs = {
    // editor functionality prefs
    true,  // syntaxEnable
    true,  // foldEnable
    true,  // indentEnable
    // display defaults prefs
    false, // overTypeInitial
    false, // readOnlyInitial
    false,  // wrapModeInitial
    false, // displayEOLEnable
    false, // IndentGuideEnable
    true,  // lineNumberEnable
    false, // longLineOnEnable
    false, // whiteSpaceEnable
};

//----------------------------------------------------------------------------
// keywordlists
// C++
wxChar* CppWordlist1 =
    _T("asm auto bool break case catch char class const const_cast \
       continue default delete do double dynamic_cast else enum explicit \
       export extern false float for friend goto if inline int long \
       mutable namespace new operator private protected public register \
       reinterpret_cast return short signed sizeof static static_cast \
       struct switch template this throw true try typedef typeid \
       typename union unsigned using virtual void volatile wchar_t \
       while");
wxChar* CppWordlist2 =
    _T("file");
wxChar* CppWordlist3 =
    _T("a addindex addtogroup anchor arg attention author b brief bug c \
       class code date def defgroup deprecated dontinclude e em endcode \
       endhtmlonly endif endlatexonly endlink endverbatim enum example \
       exception f$ f[ f] file fn hideinitializer htmlinclude \
       htmlonly if image include ingroup internal invariant interface \
       latexonly li line link mainpage name namespace nosubgrouping note \
       overload p page par param post pre ref relates remarks return \
       retval sa section see showinitializer since skip skipline struct \
       subsection test throw todo typedef union until var verbatim \
       verbinclude version warning weakgroup $ @ \" & < > # { }");

// Python
wxChar* PythonWordlist1 =
    _T("and assert break class continue def del elif else except exec \
       finally for from global if import in is lambda None not or pass \
       print raise return try while yield");
wxChar* PythonWordlist2 =
    _T("ACCELERATORS ALT AUTO3STATE AUTOCHECKBOX AUTORADIOBUTTON BEGIN \
       BITMAP BLOCK BUTTON CAPTION CHARACTERISTICS CHECKBOX CLASS \
       COMBOBOX CONTROL CTEXT CURSOR DEFPUSHBUTTON DIALOG DIALOGEX \
       DISCARDABLE EDITTEXT END EXSTYLE FONT GROUPBOX ICON LANGUAGE \
       LISTBOX LTEXT MENU MENUEX MENUITEM MESSAGETABLE POPUP PUSHBUTTON \
       RADIOBUTTON RCDATA RTEXT SCROLLBAR SEPARATOR SHIFT STATE3 \
       STRINGTABLE STYLE TEXTINCLUDE VALUE VERSION VERSIONINFO VIRTKEY");


//----------------------------------------------------------------------------
//! languages
const LanguageInfo g_LanguagePrefs [] = {
    // C++
    {_T("C++"),
     _T("*.c;*.cc;*.cpp;*.cxx;*.cs;*.h;*.hh;*.hpp;*.hxx;*.sma"),
     wxSCI_LEX_CPP,
     {{TOKEN_DEFAULT, NULL},
      {TOKEN_COMMENT, NULL},
      {TOKEN_COMMENT_LINE, NULL},
      {TOKEN_COMMENT_DOC, NULL},
      {TOKEN_NUMBER, NULL},
      {TOKEN_WORD1, CppWordlist1}, // KEYWORDS
      {TOKEN_STRING, NULL},
      {TOKEN_CHARACTER, NULL},
      {TOKEN_UUID, NULL},
      {TOKEN_PREPROCESSOR, NULL},
      {TOKEN_OPERATOR, NULL},
      {TOKEN_IDENTIFIER, NULL},
      {TOKEN_STRING_EOL, NULL},
      {TOKEN_DEFAULT, NULL}, // VERBATIM
      {TOKEN_REGEX, NULL},
      {TOKEN_COMMENT_SPECIAL, NULL}, // DOXY
      {TOKEN_WORD2, CppWordlist2}, // EXTRA WORDS
      {TOKEN_WORD3, CppWordlist3}, // DOXY KEYWORDS
      {TOKEN_ERROR, NULL}, // KEYWORDS ERROR
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL}},
     FOLD_TYPE_COMMENT | FOLD_TYPE_COMPACT | FOLD_TYPE_PREPROC},
    // Python
    {_T("Python"),
     _T("*.py;*.pyw"),
     wxSCI_LEX_PYTHON,
     {{TOKEN_DEFAULT, NULL},
      {TOKEN_COMMENT_LINE, NULL},
      {TOKEN_NUMBER, NULL},
      {TOKEN_STRING, NULL},
      {TOKEN_CHARACTER, NULL},
      {TOKEN_WORD1, PythonWordlist1}, // KEYWORDS
      {TOKEN_DEFAULT, NULL}, // TRIPLE
      {TOKEN_DEFAULT, NULL}, // TRIPLEDOUBLE
      {TOKEN_DEFAULT, NULL}, // CLASSNAME
      {TOKEN_DEFAULT, PythonWordlist2}, // DEFNAME
      {TOKEN_OPERATOR, NULL},
      {TOKEN_IDENTIFIER, NULL},
      {TOKEN_DEFAULT, NULL}, // COMMENT_BLOCK
      {TOKEN_STRING_EOL, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL}},
     FOLD_TYPE_COMMENTPY | FOLD_TYPE_QUOTESPY},
    // * (any)
    {(wxChar *)DEFAULT_LANGUAGE,
     _T("*.*"),
     wxSCI_LEX_PROPERTIES,
     {{TOKEN_DEFAULT, NULL},
      {TOKEN_DEFAULT, NULL},
      {TOKEN_DEFAULT, NULL},
      {TOKEN_DEFAULT, NULL},
      {TOKEN_DEFAULT, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL},
      {-1, NULL}},
     0},
    };

const int g_LanguagePrefsSize = WXSIZEOF(g_LanguagePrefs);

//----------------------------------------------------------------------------
//! style types
const StyleInfo g_StylePrefs [] = {
    // TOKEN_DEFAULT
    {_T("Default"),
     _T("BLACK"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_WORD1
    {_T("Keyword1"),
     _T("BLUE"), _T("WHITE"),
     _T(""), 10, TOKEN_STYLE_BOLD, 0},

    // TOKEN_WORD2
    {_T("Keyword2"),
     _T("DARK BLUE"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_WORD3
    {_T("Keyword3"),
     _T("CORNFLOWER BLUE"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_WORD4
    {_T("Keyword4"),
     _T("CYAN"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_WORD5
    {_T("Keyword5"),
     _T("DARK GREY"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_WORD6
    {_T("Keyword6"),
     _T("GREY"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_COMMENT
    {_T("Comment"),
     _T("FOREST GREEN"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_COMMENT_DOC
    {_T("Comment (Doc)"),
     _T("FOREST GREEN"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_COMMENT_LINE
    {_T("Comment line"),
     _T("FOREST GREEN"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_COMMENT_SPECIAL
    {_T("Special comment"),
     _T("FOREST GREEN"), _T("WHITE"),
     _T(""), 10, TOKEN_STYLE_ITALIC, 0},

    // TOKEN_CHARACTER
    {_T("Character"),
     _T("KHAKI"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_CHARACTER_EOL
    {_T("Character (EOL)"),
     _T("KHAKI"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_STRING
    {_T("String"),
     _T("BROWN"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_STRING_EOL
    {_T("String (EOL)"),
     _T("BROWN"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_DELIMITER
    {_T("Delimiter"),
     _T("ORANGE"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_PUNCTUATION
    {_T("Punctuation"),
     _T("ORANGE"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_OPERATOR
    {_T("Operator"),
     _T("BLACK"), _T("WHITE"),
     _T(""), 10, TOKEN_STYLE_BOLD, 0},

    // TOKEN_BRACE
    {_T("Label"),
     _T("VIOLET"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_COMMAND
    {_T("Command"),
     _T("BLUE"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_IDENTIFIER
    {_T("Identifier"),
     _T("BLACK"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_LABEL
    {_T("Label"),
     _T("VIOLET"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_NUMBER
    {_T("Number"),
     _T("SIENNA"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_PARAMETER
    {_T("Parameter"),
     _T("VIOLET"), _T("WHITE"),
     _T(""), 10, TOKEN_STYLE_ITALIC, 0},

    // TOKEN_REGEX
    {_T("Regular expression"),
     _T("ORCHID"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_UUID
    {_T("UUID"),
     _T("ORCHID"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_VALUE
    {_T("Value"),
     _T("ORCHID"), _T("WHITE"),
     _T(""), 10, TOKEN_STYLE_ITALIC, 0},

    // TOKEN_PREPROCESSOR
    {_T("Preprocessor"),
     _T("GREY"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_SCRIPT
    {_T("Script"),
     _T("DARK GREY"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_ERROR
    {_T("Error"),
     _T("RED"), _T("WHITE"),
     _T(""), 10, 0, 0},

    // TOKEN_UNDEFINED
    {_T("Undefined"),
     _T("ORANGE"), _T("WHITE"),
     _T(""), 10, 0, 0}

    };

const int g_StylePrefsSize = WXSIZEOF(g_StylePrefs);

