
SUBDIRS = 


SOURCE_DIR=@srcdir@
INCLUDE_DIR=$(SOURCE_DIR)/include

if MINGW32_OPT
SCINTILLA_DEF = -DSCI_LEXER -D__WX__=1 -DLINK_LEXERS
else
SCINTILLA_DEF = -DSCI_LEXER -DGTK -DLINK_LEXERS
endif

AM_CXXFLAGS = \
	   -I $(SOURCE_DIR)/include \
	   -I $(SOURCE_DIR)/src/scintilla/include \
	   -I $(SOURCE_DIR)/src/scintilla/src \
	   @WX_CFLAGS@ \
	   $(SCINTILLA_DEF)
LDADD = @WX_LIBS@ @LDADD@

noinst_LTLIBRARIES = libwxscintilla.la

libwxscintilla_la_SOURCES = \
			src/PlatWX.cpp \
			src/ScintillaWX.cpp \
			src/wxscintilla.cpp \
			src/scintilla/src/AutoComplete.cxx \
			src/scintilla/src/CallTip.cxx \
			src/scintilla/src/CellBuffer.cxx \
			src/scintilla/src/CharClassify.cxx \
			src/scintilla/src/ContractionState.cxx \
			src/scintilla/src/DocumentAccessor.cxx \
			src/scintilla/src/Document.cxx \
			src/scintilla/src/Editor.cxx \
			src/scintilla/src/ExternalLexer.cxx \
			src/scintilla/src/Indicator.cxx \
			src/scintilla/src/KeyMap.cxx \
			src/scintilla/src/KeyWords.cxx \
			src/scintilla/src/LexAda.cxx \
			src/scintilla/src/LexAPDL.cxx \
			src/scintilla/src/LexAsm.cxx \
			src/scintilla/src/LexAsn1.cxx \
			src/scintilla/src/LexAU3.cxx \
			src/scintilla/src/LexAVE.cxx \
			src/scintilla/src/LexBaan.cxx \
			src/scintilla/src/LexBash.cxx \
			src/scintilla/src/LexBasic.cxx \
			src/scintilla/src/LexBullant.cxx \
			src/scintilla/src/LexCaml.cxx \
			src/scintilla/src/LexCLW.cxx \
			src/scintilla/src/LexConf.cxx \
			src/scintilla/src/LexCPP.cxx \
			src/scintilla/src/LexCrontab.cxx \
			src/scintilla/src/LexCsound.cxx \
			src/scintilla/src/LexCSS.cxx \
			src/scintilla/src/LexEiffel.cxx \
			src/scintilla/src/LexErlang.cxx \
			src/scintilla/src/LexEScript.cxx \
			src/scintilla/src/LexFlagship.cxx \
			src/scintilla/src/LexForth.cxx \
			src/scintilla/src/LexFortran.cxx \
			src/scintilla/src/LexGui4Cli.cxx \
			src/scintilla/src/LexHaskell.cxx \
			src/scintilla/src/LexHTML.cxx \
			src/scintilla/src/LexInno.cxx \
			src/scintilla/src/LexKix.cxx \
			src/scintilla/src/LexLisp.cxx \
			src/scintilla/src/LexLout.cxx \
			src/scintilla/src/LexLua.cxx \
			src/scintilla/src/LexMatlab.cxx \
			src/scintilla/src/LexMetapost.cxx \
			src/scintilla/src/LexMMIXAL.cxx \
			src/scintilla/src/LexMPT.cxx \
			src/scintilla/src/LexMSSQL.cxx \
			src/scintilla/src/LexNsis.cxx \
			src/scintilla/src/LexOpal.cxx \
			src/scintilla/src/LexOthers.cxx \
			src/scintilla/src/LexPascal.cxx \
			src/scintilla/src/LexPB.cxx \
			src/scintilla/src/LexPerl.cxx \
			src/scintilla/src/LexPOV.cxx \
			src/scintilla/src/LexPS.cxx \
			src/scintilla/src/LexPython.cxx \
			src/scintilla/src/LexRebol.cxx \
			src/scintilla/src/LexRuby.cxx \
			src/scintilla/src/LexScriptol.cxx \
			src/scintilla/src/LexSmalltalk.cxx \
			src/scintilla/src/LexSpecman.cxx \
			src/scintilla/src/LexSpice.cxx \
			src/scintilla/src/LexSQL.cxx \
			src/scintilla/src/LexTADS3.cxx \
			src/scintilla/src/LexTCL.cxx \
			src/scintilla/src/LexTeX.cxx \
			src/scintilla/src/LexVB.cxx \
			src/scintilla/src/LexVerilog.cxx \
			src/scintilla/src/LexVHDL.cxx \
			src/scintilla/src/LexYAML.cxx \
			src/scintilla/src/LineMarker.cxx \
			src/scintilla/src/PropSet.cxx \
			src/scintilla/src/RESearch.cxx \
			src/scintilla/src/ScintillaBase.cxx \
			src/scintilla/src/StyleContext.cxx \
			src/scintilla/src/Style.cxx \
			src/scintilla/src/UniConversion.cxx \
			src/scintilla/src/ViewStyle.cxx \
			src/scintilla/src/WindowAccessor.cxx \
			src/scintilla/src/XPM.cxx 

test:
	echo $(libwxscintilla_la_SOURCES)

