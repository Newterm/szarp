
#
# $Id: Makefile.am 6491 2009-03-18 23:25:18Z reksio $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'

# Compilations flags
AM_CPPFLAGS = @WX_CXXFLAGS@ @XML_CFLAGS@ @LUA_CFLAGS@ @BOOST_CPPFLAGS@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
        -I@srcdir@/../../include -I@srcdir@/../common 

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la 
LIBWXCOMMON=../common/libwxcommon.la

# Program to create

bin_PROGRAMS = kontroler3

kontroler3_SOURCES = \
	kontr.cc kontr.h kontroler.cc kontroler.h kontradd.cc kontradd.h \
	parcalc.cc parcalc.h kontropt.cc kontropt.h kontralarm.h kontralarm.cc

if MINGW32_OPT
RESOURCES = resources.o
else
RESOURCES = 
endif

kontroler3_DEPENDENCIES = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP) $(LIBSZARP2)
kontroler3_LDADD = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP) @XML_LIBS@ @WX_LIBS@ @LUA_LIBS@ @CURL_LIBS@ @BOOST_LDFLAGS@ @BOOST_DATE_TIME_LIB@ @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_LOCALE_LIB@

kontroler3_LOCALES = pl/kontroler3.mo
kontroler3_LOCALES_SRC = pl/kontroler3.po
kontroler3_LOCALES_TMP = pl/kontroler3.pot

CLEANFILES = $(kontroler3_LOCALES)

EXTRA_DIST = $(kontroler3_LOCALES_SRC) $(kontroler3_LOCALES)

# Creating resources file with icons for Windows
ICON_SRC := ../../resources/wx/icons/kontr64.xpm
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


all: kontroler3$(EXEEXT) $(kontroler3_LOCALES)

# Install locales
install-data-local: $(kontroler3_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(kontroler3_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES

update-po:
	@XGETTEXT@ -o $(kontroler3_LOCALES_TMP) -L C++ --keyword=_ $(kontroler3_SOURCES)
	@MSGMERGE@ -U $(kontroler3_LOCALES_SRC) $(kontroler3_LOCALES_TMP)
	rm -f $(kontroler3_LOCALES_TMP)
