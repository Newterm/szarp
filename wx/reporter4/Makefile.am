
#
# $Id: Makefile.am 6491 2009-03-18 23:25:18Z reksio $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'

# Compilations flags
AM_CPPFLAGS = @WX_CXXFLAGS@ @XML_CFLAGS@ @CURL_CFLAGS@ @LUA_CFLAGS@ @BOOST_CPPFLAGS@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
	-I@srcdir@/../../include \
	-I@srcdir@/../../iks/common \
	-I@srcdir@/../../iks/client \
	-I@srcdir@/../common \
	-DPREFIX=\"@prefix@\"

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la 
LIBIKSCOMMON=../../iks/common/libiks-common.a
LIBIKSCLIENT=../../iks/client/libiks-client.a
LIBWXCOMMON=../common/libwxcommon.la

# Program to create

bin_PROGRAMS = reporter4

WIN_APP_NAME=Reporter4

ICONS=\
	bmp_grey.xpm \
	bmp_start.xpm \
	bmp_green1.xpm \
	bmp_green2.xpm \
	bmp_yellow1.xpm \
	bmp_yellow2.xpm \
	bmp_red1.xpm \
	bmp_red2.xpm

reporter4_SOURCES = \
	repapp.cpp \
	reporter_view.cpp reporter_view.h \
	reporter_controller.hpp reporter_controller.h \
	report_edit.cpp report_edit.h \
	userreports.cpp userreports.h \
	report.h report.cpp \
	data_provider.cpp config_handler.cpp \
	$(ICONS)

if MINGW32_OPT
RESOURCES = resources.o
else
RESOURCES = 
endif

reporter4_DEPENDENCIES = $(RESOURCES) $(LIBWXCOMMON) $(LIBIKSCLIENT) $(LIBIKSCOMMON) $(LIBSZARP2) $(LIBSZARP)
reporter4_LDADD = $(RESOURCES) $(LIBWXCOMMON) $(LIBIKSCLIENT) $(LIBIKSCOMMON) $(LIBSZARP2) $(LIBSZARP) \
	@XML_LIBS@ @WX_LIBS@ @CURL_LIBS@ @LUA_LIBS@ @BOOST_LDFLAGS@ @BOOST_PROGRAM_OPTIONS_LIB@ @BOOST_DATE_TIME_LIB@ @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_LOCALE_LIB@

reporter4_LOCALES = pl/reporter4.mo
reporter4_LOCALES_SRC = pl/reporter4.po
reporter4_LOCALES_TMP = pl/reporter4.pot

CLEANFILES = $(reporter4_LOCALES) ${WIN_APP_NAME}Setup.exe

EXTRA_DIST = $(reporter4_LOCALES_SRC)

# Creating resources file with icons for Windows
ICON_SRC := ../../resources/wx/icons/rap16.xpm
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

all: reporter4$(EXEEXT) $(reporter4_LOCALES)

# Install locales
install-data-local: $(reporter4_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(reporter4_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES

update-po:
	@XGETTEXT@ -o $(reporter4_LOCALES_TMP) -L C++ --keyword=_ $(reporter4_SOURCES)
	@MSGMERGE@ -U $(reporter4_LOCALES_SRC) $(reporter4_LOCALES_TMP)
	rm -f $(reporter4_LOCALES_TMP)

# Target for creating Windows version zip-package.
windist: reporter4$(EXEEXT) $(reporter4_LOCALES)
	@srcdir@/../../script/windist.sh reporter4 @srcdir@ 
	
winsetup: ${WIN_APP_NAME}Setup.exe

${WIN_APP_NAME}Setup.exe: reporter4$(EXEEXT) $(reporter4_LOCALES)
	@srcdir@/../../script/nsis/winsetup.sh ${WIN_APP_NAME} reporter4$(EXEEXT) \
		-d resources/documentation/new/raporter/html \
		@srcdir@/../../resources/documentation/new/raporter/html/*.h* \
		-d resources/documentation/new/raporter/html/pics \
		@srcdir@/../../resources/documentation/new/raporter/html/*.* \
		-d resources/locales/pl_PL/LC_MESSAGES \
		$(reporter4_LOCALES) \
		-d resources/wx/html/raporterabou.html \
		@srcdir@/../../resources/wx/html/raporterabout.html
	@echo Make sure docs are generated!!!
		

