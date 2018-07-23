
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 5372 2008-03-26 13:15:02Z schylek $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'

# Compilations flags
AM_CPPFLAGS = @WX_CXXFLAGS@ @CURL_CFLAGS@ @SSL_CFLAGS@ @XML_CFLAGS@ @BOOST_CPPFLAGS@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
        -I@srcdir@/../../include \
	-I@srcdir@/../common \
	-DPREFIX=\"@prefix@\"

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la
LIBWXCOMMON=../common/libwxcommon.la

DISTRO=@distro@

WIN_APP_NAME=SZAU

szau_LOCALES = pl/szau.mo
szau_LOCALES_SRC = pl/szau.po
szau_LOCALES_TMP = pl/szau.pot

CLEANFILES = $(szau_LOCALES)

EXTRA_DIST = $(szau_LOCALES_SRC) $(szau_LOCALES)

# Program to create

bin_PROGRAMS = szau

szau_SOURCES = \
        @srcdir@/szauapp.cpp @srcdir@/szauapp.h \
        @srcdir@/downloader.cpp @srcdir@/downloader.h \
        @srcdir@/updater.cpp @srcdir@/updater.h \
        @srcdir@/szautaskbaritem.cpp @srcdir@/szautaskbaritem.h 

if MINGW32_OPT
RESOURCES = resources.o
else
RESOURCES = 
endif

szau_DEPENDENCIES = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP) $(LIBSZARP2)
szau_LDADD = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP) @WX_LIBS@ @CURL_LIBS@ @BOOST_LDFLAGS@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_LOCALE_LIB@ @SSL_LIBS@ $(LIBSZARP2) @XML_LIBS@

# Creating resources file with icons for Windows
ICON_SRC := ../../resources/wx/icons/szau_16.xpm
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

all: szau$(EXEEXT) $(szau_LOCALES)


# Install locales
install-data-local: $(szau_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(szau_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES

winsetup: ${WIN_APP_NAME}Setup.exe

${WIN_APP_NAME}Setup.exe: szau$(EXEEXT) $(szau_LOCALES)
	@srcdir@/../../script/nsis/winsetup.sh ${WIN_APP_NAME} szau$(EXEEXT) \
		-d resources/locales/pl_PL/LC_MESSAGES \
		$(szau_LOCALES) 
        
update-po:
	@XGETTEXT@ -o $(szau_LOCALES_TMP) -L C++ --keyword=_ $(szau_SOURCES)
	@MSGMERGE@ -U $(szau_LOCALES_SRC) $(szau_LOCALES_TMP)
