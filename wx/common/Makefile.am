
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
	@srcdir@/ipc.cc @srcdir@/ipc.h \
	@srcdir@/cfgnames.cpp @srcdir@/cfgnames.h \
	@srcdir@/balloontaskbaritem.cpp @srcdir@/balloontaskbaritem.h \
	@srcdir@/authdiag.cpp @srcdir@/authdiag.h \
	@srcdir@/biowxsock.h @srcdir@/biowxsock.cpp \
	@srcdir@/cfgdlg.h @srcdir@/cfgdlg.cpp \
	@srcdir@/szframe.h @srcdir@/szframe.cpp \
	@srcdir@/sztaskbaritem.h @srcdir@/sztaskbaritem.cpp \
	@srcdir@/singleinstance.h @srcdir@/singleinstance.cc \
	@srcdir@/md5.h @srcdir@/md5.cpp \
	@srcdir@/getprobersaddresses.h @srcdir@/getprobersaddresses.cpp \
	@srcdir@/helpctrlex/helpctrlex.cpp @srcdir@/helpctrlex/helpctrlex.h \
	@srcdir@/helpctrlex/helpdlgex.cpp @srcdir@/helpctrlex/helpdlgex.h \
	@srcdir@/helpctrlex/helpfrmex.cpp @srcdir@/helpctrlex/helpfrmex.h \
	@srcdir@/helpctrlex/helpwinex.cpp @srcdir@/helpctrlex/helpwinex.h \
	@srcdir@/validate.cpp @srcdir@/validate.h \
	@srcdir@/user_def_ipk.cpp @srcdir@/user_def_ipk.h \
	@srcdir@/sys.cpp @srcdir@/sys.h

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
	@XGETTEXT@ -o $(common_LOCALES_TMP) -L C++ --from-code=UTF-8 --keyword=_  $(libwxcommon_la_SOURCES) @srcdir@/$(TMP_GETTEXT).h
	@MSGMERGE@ -U pl/common.po $(common_LOCALES_TMP)
	@MSGMERGE@ -U fr/common.po $(common_LOCALES_TMP)
	@MSGMERGE@ -U de/common.po $(common_LOCALES_TMP)
	rm -f $(common_LOCALES_TMP)

@srcdir@/version.h: @top_srcdir@/debian/changelog Makefile.am
	echo -n "#define SZARP_VERSION \"" > @srcdir@/version.h
	@ @top_srcdir@/script/version-info -v @top_srcdir@/debian/changelog >> @srcdir@/version.h;
	echo "\"" >> @srcdir@/version.h

