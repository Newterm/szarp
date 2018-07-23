
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6842 2009-06-19 09:58:13Z reksio $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'

if MINGW32_OPT
RESOURCES = resources.o
else
RESOURCES = 
endif

# Compilations flags
AM_CPPFLAGS = @WX_CXXFLAGS@ @XML_CFLAGS@ @LUA_CFLAGS@ @SSL_CFLAGS@ @BOOST_CPPFLAGS@ @CARES_CFLAGS@ @CGAL_CFLAGS@ \
	   @GDK_CFLAGS@ @GTK_CFLAGS@ @SQLITE3_CFLAGS@ @FREETYPE_CFLAGS@ @XMLRPC_EPI_CFLAGS@ \
	   -I@srcdir@/../../libSzarp/include \
	   -I@srcdir@/../../libSzarp2/include \
	   -I@srcdir@/../../iks/common \
	   -I@srcdir@/../../iks/client \
	   -I@srcdir@/../../include \
	   -I@srcdir@/../common \
	   -I@srcdir@/../../extern/wxscintilla/include \
	   -DPREFIX=\"@prefix@\"
	
LIBIKSCOMMON=../../iks/common/libiks-common.a
LIBIKSCLIENT=../../iks/client/libiks-client.a
LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la
LIBWXCOMMON=../common/libwxcommon.la
LIBWXSCINTILLA=../../extern/wxscintilla/libwxscintilla.la
GUIDIR=GUI/
GUIFILES=xrc
TMP_GETTEXT=XRC_gettext_file

# Program to create

bin_PROGRAMS = draw3

WIN_APP_NAME = Draw3

draw3_SOURCES = \
		classes.h \
		draw3.cpp \
		dbmgr.cpp dbmgr.h \
		database.h database.cpp \
		base_handler.h base_handler.cpp \
		disptime.cpp disptime.h \
		drawpnl.cpp drawpnl.h \
                drawswdg.cpp drawswdg.h \
		drawsctrl.h drawsctrl.cpp \
		drawapp.cpp drawapp.h \
		draw.cpp draw.h \
		coobs.h coobs.cpp \
		cfgmgr.cpp cfgmgr.h \
		ids.h \
		infowdg.cpp infowdg.h \
		drawview.h drawview.cpp \
		seldraw.cpp seldraw.h \
		selset.cpp selset.h \
		timewdg.cpp timewdg.h \
		drawtb.cpp drawtb.h \
		incsearch.cpp incsearch.h \
		drawobs.h drawobs.cpp \
		summwin.h summwin.cpp \
		drawfrm.h drawfrm.cpp \
		frmmgr.h frmmgr.cpp \
		timeformat.h timeformat.cpp \
		defcfg.h defcfg.cpp \
		drawpick.h drawpick.cpp \
		drawprint.h drawprint.cpp \
		piewin.cpp piewin.h \
		relwin.cpp relwin.h \
		xydiag.cpp xydiag.h \
		xygraph.cpp xygraph.h \
		dbinquirer.cpp dbinquirer.h \
		statdiag.h statdiag.cpp \
		drawdnd.h drawdnd.cpp \
		progfrm.h progfrm.cpp \
		remarks.h remarks.cpp \
		remarksdialog.h remarksdialog.cpp \
		remarksfetcher.h remarksfetcher.cpp \
		splashscreen.cpp splashscreen.h \
		drawipc.cpp drawipc.h \
		drawurl.cpp drawurl.h \
		errfrm.h errfrm.cpp \
		paramslist.cpp paramslist.h \
		paredit.cpp paredit.h \
		codeeditor.cpp codeeditor.h \
		drawpsc.h drawpsc.cpp \
		wxgraphs.h wxgraphs.cpp \
		glgraphs.h glgraphs.cpp \
		gcdcgraphs.h gcdcgraphs.cpp \
		graphsutils.h graphsutils.cpp \
		drawtime.h drawtime.cpp \
		vercheck.h vercheck.cpp \
		xyzgraph.h xyzgraph.cpp \
		triangulation.h triangulation.cpp \
		probadddiag.h probadddiag.cpp \
		drawtreedialog.h drawtreedialog.cpp \
		delqitem.h delqitem.cpp \
		seteditctrl.h parameditctrl.h \
		ikssetup.h ikssetup.cpp \
		GUI/resource.cpp

draw3_DEPENDENCIES = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP) $(LIBSZARP2) $(LIBWXSCINTILLA) $(LIBIKSCLIENT) $(LIBIKSCOMMON)
draw3_LDADD = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP) $(LIBWXSCINTILLA) $(LIBIKSCLIENT) $(LIBIKSCOMMON) @XSLT_LIBS@ @ZIP_LIBS@ @SSL_LIBS@ @XML_LIBS@ @WX_LIBS@ @LUA_LIBS@ @GDK_LIBS@ @SQLITE3_LIBS@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_LDFLAGS@ @BOOST_DATE_TIME_LIB@ @BOOST_THREAD_LIB@ @BOOST_LOCALE_LIB@ @FTGL_LIBS@  @WXGL_LIBS@ @XMLRPC_EPI_LIBS@ @CARES_LIBS@ @MINGW32_LIBS@ @CGAL_LIBS@ @ZMQ_LIBS@ @PROTOBUF_LIBS@

draw3_LOCALES_TMP = @srcdir@/draw3.tpo

CLEANFILES = fr/draw3.mo pl/draw3.mo de/draw3.mo $(draw3_LOCALES_TMP)
EXTRA_DIST = fr/draw3.po pl/draw3.po de/draw3.po

# Creating resources file with icons for Windows
ICON_SRC := ../../resources/wx/icons/draw64.xpm
ICON := $(shell basename $(ICON_SRC) .xpm)
ICON_ICO := $(ICON).ico

$(ICON_ICO): $(ICON_SRC)
	@CONVERT@ $< $@

resources.rc:
	echo "$(ICON) ICON $(ICON_ICO)" > $@
	echo "#include <wx/msw/wx.rc>" >> $@

resources.o: resources.rc $(ICON_ICO)
	@WINDRES@ $(shell for i in @WX_CPPFLAGS@; do echo $$i | grep '\(^-I\)\|\(-D\)' ; done) $< $@

# Rule to create mo files
%.mo: %.po
	mkdir -p `dirname $@`
	@MSGFMT@ -o $@ $<

all: draw3$(EXEEXT) fr/draw3.mo pl/draw3.mo de/draw3.po

# Install locales and GUIs
install-data-local: fr/draw3.mo pl/draw3.mo de/draw3.mo
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/fr_FR/LC_MESSAGES
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/de_DE/LC_MESSAGES
	$(INSTALL) pl/draw3.mo \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) fr/draw3.mo \
                $(DESTDIR)@prefix@/resources/locales/fr_FR/LC_MESSAGES
	$(INSTALL) de/draw3.mo \
                $(DESTDIR)@prefix@/resources/locales/de_DE/LC_MESSAGES
	$(INSTALL) -d $(DESTDIR)$(bindir)/$(GUIDIR)
               

@srcdir@/GUI/resource.cpp: @srcdir@/GUI/*xrc
	wxrc -c -o @srcdir@/GUI/resource.cpp @srcdir@/GUI/*xrc

# Target to update po files from sources
update-po:
	@WXRC@ -g @srcdir@/$(GUIDIR)*.$(GUIFILES) > @srcdir@/$(TMP_GETTEXT).h
	@XGETTEXT@ -o $(draw3_LOCALES_TMP) -L C++ --keyword=_  @srcdir@/*.cpp @srcdir@/*h
	@MSGMERGE@ -U @srcdir@/pl/draw3.po $(draw3_LOCALES_TMP)
	@MSGMERGE@ -U @srcdir@/fr/draw3.po $(draw3_LOCALES_TMP)
	@MSGMERGE@ -U @srcdir@/de/draw3.po $(draw3_LOCALES_TMP)
	rm -f $(draw3_LOCALES_TMP)
	rm @srcdir@/$(TMP_GETTEXT).h

winsetup: ${WIN_APP_NAME}Setup.exe

${WIN_APP_NAME}Setup.exe: draw3$(EXEEXT) pl/draw3.mo fr/draw3.mo de/draw3.mo
	@srcdir@/../../script/nsis/winsetup.sh ${WIN_APP_NAME} draw3$(EXEEXT) \
		-d resources/locales/pl_PL/LC_MESSAGES \
		pl/draw3.mo \
		-d resources/locales/fr_FR/LC_MESSAGES \
		fr/draw3.mo \
		-d resources/locales/de_DE/LC_MESSAGES \
		de/draw3.mo

