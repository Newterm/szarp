
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6789 2009-05-29 12:03:33Z pawel $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'

# Compilations flags
AM_CPPFLAGS = @WX_CXXFLAGS@ @XML_CFLAGS@ @LUA_CFLAGS@ @BOOST_CPPFLAGS@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
        -I@srcdir@/../../include \
	-I@srcdir@/../common  \
	-DPREFIX=\"@prefix@\"

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la
LIBWXCOMMON=../common/libwxcommon.la

# Program to create

bin_PROGRAMS = ekstraktor3

ekstraktor3_SOURCES = \
                ekstraktor3.cpp \
		ids.h \
                EkstraktorWidget.cpp EkstraktorWidget.h \
                EkstraktorMainWindow.cpp EkstraktorMainWindow.h \
		extraction.cpp extraction.h 

if MINGW32_OPT
RESOURCES = resources.o
else
RESOURCES = 
endif

ekstraktor3_DEPENDENCIES = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZ) $(LIBSZARP2) $(LIBSZAR)
ekstraktor3_LDADD = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP) @XML_LIBS@ @WX_LIBS@ \
		    @XSLT_LIBS@ @LUA_LIBS@ @ZIP_LIBS@ \
		    @BOOST_LDFLAGS@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ @BOOST_LOCALE_LIB@ @MINGW32_LIBS@ @ZMQ_LIBS@ @PROTOBUF_LIBS@

ekstraktor3_LOCALES = pl/ekstraktor3.mo
ekstraktor3_LOCALES_SRC = pl/ekstraktor3.po
ekstraktor3_LOCALES_TMP = pl/ekstraktor3.pot

CLEANFILES = $(ekstraktor3_LOCALES)

EXTRA_DIST = $(ekstraktor3_LOCALES_SRC) $(ekstraktor3_LOCALES)

# Creating resources file with icons for Windows
ICON_SRC := ../../resources/wx/icons/extr64.xpm
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

all: ekstraktor3$(EXEEXT) $(ekstraktor3_LOCALES)

# Install locales
install-data-local: $(ekstraktor3_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(ekstraktor3_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES

update-po:
	@XGETTEXT@ -o $(ekstraktor3_LOCALES_TMP) -L C++ --keyword=_  $(ekstraktor3_SOURCES)
	@MSGMERGE@ -U $(ekstraktor3_LOCALES_SRC) $(ekstraktor3_LOCALES_TMP)
	rm -f $(ekstraktor3_LOCALES_TMP)
