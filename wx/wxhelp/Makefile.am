
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6199 2008-11-20 08:28:37Z reksio $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'

# Compilations flags
AM_CPPFLAGS = @WX_CXXFLAGS@ -I@srcdir@/../common -DPREFIX=\"@prefix@\" \
	-I@srcdir@/../../libSzarp/include

LIBWXCOMMON=../common/libwxcommon.la
LIBSZARP=../../libSzarp/libSzarp.la

# Program to create

bin_PROGRAMS = wxhelp

wxhelp_SOURCES = \
        whapp.cpp \
        whapp.h \
        wxhelp.cpp 

if MINGW32_OPT
RESOURCES = resources.o
else
RESOURCES = 
endif

wxhelp_DEPENDENCIES = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP)
wxhelp_LDADD = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP) @WX_LIBS@ @BOOST_LDFLAGS@ @BOOST_SYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_LOCALE_LIB@ @XML_LIBS@

# Creating resources file with icons for Windows
# Application does not use this icon - but Windows use it
# to display icon in folder view
ICON_SRC := ../../resources/wx/icons/wxhelp.xpm
ICON := $(shell basename $(ICON_SRC) .xpm)
ICON_ICO := $(ICON).ico

$(ICON_ICO): $(ICON_SRC)
	@CONVERT@ $< $@

resources.rc:
	echo "$(ICON) ICON $(ICON_ICO)" > $@
	echo "#include <wx/msw/wx.rc>" >> $@

resources.o: resources.rc $(ICON_ICO)
	@WINDRES@ $(shell for i in @WX_CPPFLAGS@; do echo $$i | grep '\(^-I\)\|\(-D\)' ; done) $< $@

