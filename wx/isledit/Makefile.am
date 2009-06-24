
#
# $Id: Makefile.am 4579 2007-10-30 14:59:26Z schylek $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Compilations flags
INCLUDES = @WX_CXXFLAGS@ @XML_CFLAGS@ @CURL_CFLAGS@ @LUA_CFLAGS@ @BOOST_CPPFLAGS@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
        -I@srcdir@/../../include \
	-I@srcdir@/../common \
	-DPREFIX=\"@prefix@\"

LIBSZARP=../../libSzarp/libSzarp.a
LIBSZARP2=../../libSzarp2/libSzarp2.a 
LIBWXCOMMON=../common/libwxcommon.a

# Program to create

bin_PROGRAMS = isledit

isledit_SOURCES = \
	isledit.cpp isledit.h

isledit_DEPENDENCIES = $(LIBWXCOMMON) $(LIBSZARP) $(LIBSZARP2)
isledit_LDADD = $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP) \
	@XML_LIBS@ @WX_LIBS@ @CURL_LIBS@ @LUA_LIBS@ @BOOST_LDFLAGS@ @BOOST_DATE_TIME_LIB@ @BOOST_THREAD_LIB@ @BOOST_FILESYSTEM_LIB@

isledit_LOCALES = @srcdir@/isledit.mo
isledit_LOCALES_SRC = pl/isledit.po
isledit_LOCALES_TMP = pl/isledit.pot

#.xrc.cc:
#	wxrc -v -c $< -o $@

CLEANFILES = $(isledit_LOCALES) ${WIN_APP_NAME}Setup.exe

EXTRA_DIST = $(isledit_LOCALES_SRC) $(isledit_LOCALES)

# Rule to create mo files
$(isledit_LOCALES): $(isledit_LOCALES_SRC)
	@MSGFMT@ -o $@ $<

all: isledit$(EXEEXT) $(isledit_LOCALES)

# Install locales
install-data-local: $(isledit_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(isledit_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) -d $(DESTDIR)/usr/share/inkscape/extensions
	$(INSTALL) isledit.inx $(DESTDIR)/usr/share/inkscape/extensions

update-po:
	@XGETTEXT@ -o $(isledit_LOCALES_TMP) -L C++ --keyword=_ $(isledit_SOURCES)
	@MSGMERGE@ -U $(isledit_LOCALES_SRC) $(isledit_LOCALES_TMP)
	rm -f $(isledit_LOCALES_TMP)

