
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6842 2009-06-19 09:58:13Z reksio $

SUBDIRS = 

SOURCE_DIR=@srcdir@

LDADD=@WX_LIBS@ ../../libSzarp2/libSzarp2.la @XML_LIBS@ \
	../../libSzarp/libSzarp.la @CURL_LIBS@ @LDADD@ @LUA_LIBS@
INCLUDES=\
	-I $(SOURCE_DIR)/../../include \
	-I $(SOURCE_DIR)/../../libSzarp2/include \
	-I $(SOURCE_DIR)/../../libSzarp/include \
	@XML_CFLAGS@ @WX_CFLAGS@ @CURL_CFLAGS@ @BOOST_CPPFLAGS@ \
	-DPREFIX=\"@prefix@\" \
	@GDK_CFLAGS@ @GTK_CFLAGS@ @LUA_CFLAGS@

GTK_CFLAGS=__WXGTK__
GUIDIR=@srcdir@/GUI/
GUIFILES=xrc
TMP_GETTEXT=XRC_gettext_file


noinst_LTLIBRARIES = libwxcommon.la

WIN_APP_NAME=Szarp

BUILT_SOURCES=version.h

libwxcommon_la_SOURCES = \
		$(SOURCE_DIR)/version.h \
		$(SOURCE_DIR)/cconv.cc $(SOURCE_DIR)/cconv.h \
		$(SOURCE_DIR)/datechooser.cpp $(SOURCE_DIR)/datechooser.h \
		$(SOURCE_DIR)/dcolors.cc $(SOURCE_DIR)/dcolors.h \
		$(SOURCE_DIR)/fetchparams.cc $(SOURCE_DIR)/fetchparams.h \
		$(SOURCE_DIR)/fonts.cpp $(SOURCE_DIR)/fonts.h \
		$(SOURCE_DIR)/geometry.cpp $(SOURCE_DIR)/geometry.h \
		$(SOURCE_DIR)/htmlabout.cpp $(SOURCE_DIR)/htmlabout.h \
		$(SOURCE_DIR)/htmlview.cpp $(SOURCE_DIR)/htmlview.h \
		$(SOURCE_DIR)/parlist.cpp $(SOURCE_DIR)/parlist.h \
		$(SOURCE_DIR)/parselect.cpp $(SOURCE_DIR)/parselect.h \
		$(SOURCE_DIR)/serverdlg.cpp $(SOURCE_DIR)/serverdlg.h \
		$(SOURCE_DIR)/aboutdlg.cpp $(SOURCE_DIR)/aboutdlg.h \
		$(SOURCE_DIR)/imagepanel.cpp $(SOURCE_DIR)/imagepanel.h \
		$(SOURCE_DIR)/szapp.cpp $(SOURCE_DIR)/szapp.h \
		$(SOURCE_DIR)/szhlpctrl.cpp $(SOURCE_DIR)/szhlpctrl.h \
		$(SOURCE_DIR)/szmutex.cpp $(SOURCE_DIR)/szmutex.h \
		$(SOURCE_DIR)/ipc.cc $(SOURCE_DIR)/ipc.h \
		$(SOURCE_DIR)/cfgnames.cpp $(SOURCE_DIR)/cfgnames.h \
		$(SOURCE_DIR)/balloontaskbaritem.cpp $(SOURCE_DIR)/balloontaskbaritem.h \
		$(SOURCE_DIR)/authdiag.cpp $(SOURCE_DIR)/authdiag.h \
		$(SOURCE_DIR)/pscgui.cpp $(SOURCE_DIR)/pscgui.h \
		$(SOURCE_DIR)/biowxsock.h $(SOURCE_DIR)/biowxsock.cpp \
		$(SOURCE_DIR)/cfgdlg.h $(SOURCE_DIR)/cfgdlg.cpp \
		$(SOURCE_DIR)/szframe.h $(SOURCE_DIR)/szframe.cpp \
		$(SOURCE_DIR)/sztaskbaritem.h $(SOURCE_DIR)/sztaskbaritem.cpp \
		$(SOURCE_DIR)/singleinstance.h $(SOURCE_DIR)/singleinstance.cc \
		$(SOURCE_DIR)/md5.h $(SOURCE_DIR)/md5.cpp \
		$(SOURCE_DIR)/getprobersaddresses.h $(SOURCE_DIR)/getprobersaddresses.cpp \
		$(SOURCE_DIR)/GUI/resource.cpp \
		$(SOURCE_DIR)/helpctrlex/helpctrlex.cpp $(SOURCE_DIR)/helpctrlex/helpctrlex.h \
		$(SOURCE_DIR)/helpctrlex/helpdlgex.cpp $(SOURCE_DIR)/helpctrlex/helpdlgex.h \
		$(SOURCE_DIR)/helpctrlex/helpfrmex.cpp $(SOURCE_DIR)/helpctrlex/helpfrmex.h \
		$(SOURCE_DIR)/helpctrlex/helpwinex.cpp $(SOURCE_DIR)/helpctrlex/helpwinex.h \
		$(SOURCE_DIR)/wxlogging.cpp $(SOURCE_DIR)/wxlogging.h



# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

common_LOCALES_TMP = common.pot

CLEANFILES = pl/common.mo fr/common.mo pl/wx.mo fr/wx.mo de/wx.mo de/common.mo ${WIN_APP_NAME}Setup.exe

EXTRA_DIST = pl/common.po fr/common.po de/common.po pl/wx.po fr/wx.po de/wx.po

# Rule to create mo files
%.mo: %.po
	mkdir -p `dirname $@`
	@MSGFMT@ -o $@ $<

all: @srcdir@/version.h $(noinst_LIBRARIES) $(common_LOCALES) $(wx_LOCALES) pl/common.mo fr/common.mo de/common.mo pl/wx.mo fr/wx.mo de/wx.mo

# Install locales
install-data-local: fr/common.mo pl/common.mo pl/wx.mo fr/wx.mo de/wx.mo de/common.mo
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/fr_FR/LC_MESSAGES
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/de_DE/LC_MESSAGES
	$(INSTALL) pl/common.mo \
		$(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) fr/common.mo \
		$(DESTDIR)@prefix@/resources/locales/fr_FR/LC_MESSAGES
	$(INSTALL) de/common.mo \
		$(DESTDIR)@prefix@/resources/locales/de_DE/LC_MESSAGES
	$(INSTALL) pl/wx.mo \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) fr/wx.mo \
                $(DESTDIR)@prefix@/resources/locales/fr_FR/LC_MESSAGES
	$(INSTALL) de/wx.mo \
                $(DESTDIR)@prefix@/resources/locales/de_DE/LC_MESSAGES

update-po:
	@WXRC@ -g @srcdir@/$(GUIDIR)*.$(GUIFILES) > @srcdir@/$(TMP_GETTEXT).h
	@XGETTEXT@ -o $(common_LOCALES_TMP) -L C++ --from-code=UTF-8 --keyword=_  $(libwxcommon_la_SOURCES) @srcdir@/$(TMP_GETTEXT).h
	@MSGMERGE@ -U pl/common.po $(common_LOCALES_TMP)
	@MSGMERGE@ -U fr/common.po $(common_LOCALES_TMP)
	@MSGMERGE@ -U de/common.po $(common_LOCALES_TMP)
	rm -f $(common_LOCALES_TMP)

winsetup: ${WIN_APP_NAME}Setup.exe

@srcdir@/version.h: @top_srcdir@/debian/changelog Makefile.am
	echo -n "#define SZARP_VERSION \"" > @srcdir@/version.h
	@ @top_srcdir@/script/version-info -v @top_srcdir@/debian/changelog >> @srcdir@/version.h;
	echo "\"" >> @srcdir@/version.h

@srcdir@/GUI/resource.cpp: @srcdir@/GUI/*xrc
	wxrc -n InitCommonXmlResource -c -o @srcdir@/GUI/resource.cpp @srcdir@/GUI/*xrc

${WIN_APP_NAME}Setup.exe: $(common_LOCALES) $(wx_LOCALES)
	@srcdir@/../../script/nsis/winsetup.sh -c \
		-d resources \
		-f szarp.cfg @srcdir@/../../resources/SZARP_CFG_TEMPLATE \
		-f szarp_in.cfg @srcdir@/../../resources/SZARP_IN_CFG_TEMPLATE \
		-d resources/dtd \
		@srcdir@/../../resources/dtd/params-list.rng \
		-d resources/locales/pl_PL/LC_MESSAGES \
		pl/wx.mo \
		pl/common.mo \
		-d resources/locales/fr_FR/LC_MESSAGES \
		fr/wx.mo \
		fr/common.mo \
		-d resources/locales/de_DE/LC_MESSAGES \
		de/wx.mo \
		de/common.mo \
		-d resources/wx/icons \
		@srcdir@/../../resources/wx/icons/*.xpm \
		-d resources/wx/images \
		@srcdir@/../../resources/wx/images/*.png
		
