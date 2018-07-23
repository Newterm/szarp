
#
# $Id: Makefile.am 6491 2009-03-18 23:25:18Z reksio $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'

# Compilations flags
AM_CPPFLAGS = @WX_CXXFLAGS@ @XML_CFLAGS@ @CURL_CFLAGS@ @LUA_CFLAGS@ @BOOST_CPPFLAGS@ @WNCK_CFLAGS@  \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
        -I@srcdir@/../../include \
	-I@srcdir@/../common \
	-DPREFIX=\"@prefix@\" \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE


LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la 
LIBWXCOMMON=../common/libwxcommon.la

# Program to create

bin_PROGRAMS = viszio

WIN_APP_NAME=viszio

viszio_SOURCES = \
	viszioApp.cpp viszioFetchFrame.cpp viszioTransparentFrame.cpp viszioParamadd.cpp viszioConfiguration.cpp \
	viszioApp.h viszioFetchFrame.h viszioTransparentFrame.h viszioParamadd.h viszioConfiguration.h

if MINGW32_OPT
RESOURCES = resources.o
else
RESOURCES = 
endif

viszio_DEPENDENCIES = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP) $(LIBSZARP2)
viszio_LDADD = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP) @XML_LIBS@ @WX_LIBS@ @LUA_LIBS@ \
		@CURL_LIBS@ @BOOST_LDFLAGS@ @BOOST_DATE_TIME_LIB@ @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ \
		@BOOST_FILESYSTEM_LIB@ @GDK_LIBS@ @GLIB_LIBS@ @WNCK_LIBS@

viszio_LOCALES = pl/viszio.mo
viszio_LOCALES_SRC = pl/viszio.po
viszio_LOCALES_TMP = pl/viszio.pot

CLEANFILES = $(viszio_LOCALES) ${WIN_APP_NAME}Setup.exe
EXTRA_DIST = $(viszio_LOCALES_SRC) $(viszio_LOCALES)


# Creating resources file with icons for Windows
ICON_SRC := ../../resources/wx/icons/viszio16.xpm
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

all: viszio$(EXEEXT) $(viszio_LOCALES)

# Install locales
install-data-local: $(viszio_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(viszio_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES

update-po:
	@XGETTEXT@ -o $(viszio_LOCALES_TMP) -L C++ --keyword=_ $(viszio_SOURCES)
	@MSGMERGE@ -U $(viszio_LOCALES_SRC) $(viszio_LOCALES_TMP)
	rm -f $(viszio_LOCALES_TMP)

# Target for creating Windows version zip-package.
windist: viszio$(EXEEXT) $(viszio_LOCALES)
	@srcdir@/../../script/windist.sh viszio @srcdir@ 
	
winsetup: ${WIN_APP_NAME}Setup.exe

${WIN_APP_NAME}Setup.exe: viszio$(EXEEXT) $(viszio_LOCALES)
	@srcdir@/../../script/nsis/winsetup.sh ${WIN_APP_NAME} viszio$(EXEEXT) \
		-d resources/documentation/new/viszio/html \
		@srcdir@/../../resources/documentation/new/viszio/html/*.h* \
		-d resources/documentation/new/viszio/html/pics \
		@srcdir@/../../resources/documentation/new/viszio/html/*.* \
		-d resources/locales/pl_PL/LC_MESSAGES \
		$(viszio_LOCALES) 
	@echo Make sure docs are generated!!!
		

