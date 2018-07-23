
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6842 2009-06-19 09:58:13Z reksio $

SUBDIRS =

LDADD=@WX_LIBS@ ../../libSzarp2/libSzarp2.la @XML_LIBS@ \
	../../libSzarp/libSzarp.la @CURL_LIBS@ @LDADD@ @LUA_LIBS@
AM_CPPFLAGS = \
	-I @srcdir@/../../include \
	-I @srcdir@/../../libSzarp2/include \
	-I @srcdir@/../../libSzarp/include \
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
	@srcdir@/version.h \
	@srcdir@/cconv.cc @srcdir@/cconv.h \
	@srcdir@/datechooser.cpp @srcdir@/datechooser.h \
	@srcdir@/dcolors.cc @srcdir@/dcolors.h \
	@srcdir@/fetchparams.cc @srcdir@/fetchparams.h \
	@srcdir@/fonts.cpp @srcdir@/fonts.h \
	@srcdir@/geometry.cpp @srcdir@/geometry.h \
	@srcdir@/htmlabout.cpp @srcdir@/htmlabout.h \
	@srcdir@/htmlview.cpp @srcdir@/htmlview.h \
	@srcdir@/parlist.cpp @srcdir@/parlist.h \
	@srcdir@/parselect.cpp @srcdir@/parselect.h \
	@srcdir@/serverdlg.cpp @srcdir@/serverdlg.h \
	@srcdir@/aboutdlg.cpp @srcdir@/aboutdlg.h \
	@srcdir@/imagepanel.cpp @srcdir@/imagepanel.h \
	@srcdir@/szapp.cpp @srcdir@/szapp.h \
	@srcdir@/szhlpctrl.cpp @srcdir@/szhlpctrl.h \
	@srcdir@/szmutex.cpp @srcdir@/szmutex.h \
	@srcdir@/ipc.cc @srcdir@/ipc.h \
	@srcdir@/cfgnames.cpp @srcdir@/cfgnames.h \
	@srcdir@/balloontaskbaritem.cpp @srcdir@/balloontaskbaritem.h \
	@srcdir@/authdiag.cpp @srcdir@/authdiag.h \
	@srcdir@/pscgui.cpp @srcdir@/pscgui.h \
	@srcdir@/biowxsock.h @srcdir@/biowxsock.cpp \
	@srcdir@/cfgdlg.h @srcdir@/cfgdlg.cpp \
	@srcdir@/szframe.h @srcdir@/szframe.cpp \
	@srcdir@/sztaskbaritem.h @srcdir@/sztaskbaritem.cpp \
	@srcdir@/singleinstance.h @srcdir@/singleinstance.cc \
	@srcdir@/md5.h @srcdir@/md5.cpp \
	@srcdir@/getprobersaddresses.h @srcdir@/getprobersaddresses.cpp \
	@srcdir@/GUI/resource.cpp \
	@srcdir@/helpctrlex/helpctrlex.cpp @srcdir@/helpctrlex/helpctrlex.h \
	@srcdir@/helpctrlex/helpdlgex.cpp @srcdir@/helpctrlex/helpdlgex.h \
	@srcdir@/helpctrlex/helpfrmex.cpp @srcdir@/helpctrlex/helpfrmex.h \
	@srcdir@/helpctrlex/helpwinex.cpp @srcdir@/helpctrlex/helpwinex.h \
	@srcdir@/validate.cpp @srcdir/validate.h

# Additional files for deleting on 'make maintainer-clean'

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

