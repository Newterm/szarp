
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6491 2009-03-18 23:25:18Z reksio $

# No subdirectories
SUBDIRS =

LDADD=@LUA_LIBS@

# Compilations flags
AM_CPPFLAGS = @WX_CXXFLAGS@ @XML_CFLAGS@ -I@srcdir@/../../libSzarp2/include \
	-I@srcdir@/../../libSzarp/include \
        -I@srcdir@/../../include -I@srcdir@/../common \
        -I@srcdir@/../../resources/wx/icons \
        -DINSTALL_PREFIX='"@prefix@"' \
	@LUA_CFLAGS@

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la
LIBWXCOMMON=../common/libwxcommon.la

# Program to create

bin_PROGRAMS = @IPKEDIT@
EXTRA_PROGRAMS=ipkedit

ipkedit_SOURCES = \
        ipkedit.cpp \
        cframe.cpp cframe.h \
        confapp.cpp confapp.h \
	deditdlg.cc deditdlg.h \
	dlistctrl.cc dlistctrl.h \
	../common/dcolors.h

ipkedit_LDADD = $(LIBWXCOMMON) @WX_LIBS@ @XML_LIBS@ $(LIBSZARP2) $(LIBSZARP) @BOOST_SYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_LOCALE_LIB@

ipkedit_LOCALES = pl/ipkedit.mo
ipkedit_LOCALES_SRC = pl/ipkedit.po
ipkedit_LOCALES_TMP = pl/ipkedit.pot

noinst_DATA = $(ipkedit_LOCALES)

CLEANFILES = $(ipkedit_LOCALES)
EXTRA_DIST = $(ipkedit_LOCALES_SRC) $(ipkedit_LOCALES)

# Additional files for deleting on 'make maintainer-clean'

# Rule to create mo files
%.mo: %.po
	mkdir -p `dirname $@`
	@MSGFMT@ -o $@ $<

# Install locales
install-data-local: $(ipkedit_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(ipkedit_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES

# Target to update po files from sources
update-po:
	@XGETTEXT@ -o $(ipkedit_LOCALES_TMP) -L C++ --keyword=_  $(ipkedit_SOURCES)
	@MSGMERGE@ -U $(ipkedit_LOCALES_SRC) $(ipkedit_LOCALES_TMP)
	rm -f $(ipkedit_LOCALES_TMP)
	

