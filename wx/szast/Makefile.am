
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 4336 2007-09-05 18:29:53Z schylek $

# No subdirectories
SUBDIRS =

LDADD=@LUA_LIBS@

# Compilations flags
AM_CPPFLAGS = @WX_CXXFLAGS@ @XML_CFLAGS@ @BOOST_CPPFLAGS@ \
	-I@srcdir@/../../libSzarp2/include -I@srcdir@/../../libSzarp/include \
        -I@srcdir@/../../include -I@srcdir@/../common \
        -I@srcdir@/../../resources/wx/icons \
        -DINSTALL_PREFIX='"@prefix@"' \
	@LUA_CFLAGS@

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la
LIBWXCOMMON=../common/libwxcommon.la

# Program to create
#
WIN_APP_NAME=SZAST

bin_PROGRAMS = szast

szast_SOURCES = \
        szastapp.cpp \
        szastapp.h \
        szastframe.h \
        szastframe.cpp \
        szastconnection.h \
        szastconnection.cpp \
        settingsdialog.h \
        settingsdialog.cpp \
	GUI/resource.cpp	

if MINGW32_OPT
RESOURCES = resources.o
else
RESOURCES = 
endif

szast_DEPENDENCIES = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP)
szast_LDADD = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP) @SSL_LIBS@ @XML_LIBS@ @WX_LIBS@ @LUA_LIBS@ @BOOST_LDFLAGS@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_DATE_TIME_LIB@ @BOOST_THREAD_LIB@ @BOOST_LOCALE_LIB@ @MINGW32_LIBS@

szast_LOCALES = pl/szast.mo
szast_LOCALES_SRC = pl/szast.po
szast_LOCALES_TMP = pl/szast.pot

GUIDIR=@srcdir@/GUI/
GUIFILES=xrc
TMP_GETTEXT=XRC_gettext_file


noinst_DATA = $(szast_LOCALES)

CLEANFILES = $(szast_LOCALES)
EXTRA_DIST = $(szast_LOCALES_SRC) $(szast_LOCALES)

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Creating resources file with icons for Windows
ICON_SRC := ../../resources/wx/icons/szast.xpm
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

# Install locales
install-data-local: $(szast_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(szast_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES

# Target to update po files from sources
update-po:
	@WXRC@ -g @srcdir@/$(GUIDIR)*.$(GUIFILES) > @srcdir@/$(TMP_GETTEXT).h
	@XGETTEXT@ -o $(szast_LOCALES_TMP) -L C++ --keyword=_  $(szast_SOURCES) @srcdir@/$(TMP_GETTEXT).h
	@MSGMERGE@ -U $(szast_LOCALES_SRC) $(szast_LOCALES_TMP)
	rm -f $(szast_LOCALES_TMP)

winsetup: ${WIN_APP_NAME}Setup.exe

${WIN_APP_NAME}Setup.exe: szau$(EXEEXT) $(szau_LOCALES)
	@srcdir@/../../script/nsis/winsetup.sh ${WIN_APP_NAME} szast$(EXEEXT) \
		-d resources/locales/pl_PL/LC_MESSAGES \
		$(szast_LOCALES) 
        
	
all: szast$(EXEEXT) $(szau_LOCALES)

@srcdir@/GUI/resource.cpp: @srcdir@/GUI/*xrc
	wxrc -c -o @srcdir@/GUI/resource.cpp @srcdir@/GUI/*xrc

