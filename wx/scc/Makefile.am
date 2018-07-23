
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6725 2009-05-18 09:16:48Z reksio $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'

# Compilations flags
AM_CPPFLAGS = @WX_CXXFLAGS@ @XML_CFLAGS@ @LUA_CFLAGS@ @BOOST_CPPFLAGS@ @GTK_CFLAGS@ @GDK_CFLAGS@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
        -I@srcdir@/../../include \
	-I@srcdir@/../common \
	-DPREFIX=\"@prefix@\"

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la
LIBWXCOMMON=../common/libwxcommon.la

DISTRO=@distro@

WIN_APP_NAME=SCC

scc_LOCALES = pl/scc.mo
scc_LOCALES_SRC = pl/scc.po
scc_LOCALES_TMP = pl/scc.pot

CLEANFILES = $(scc_LOCALES)

EXTRA_DIST = $(scc_LOCALES_SRC) $(scc_LOCALES)

# Program to create

bin_PROGRAMS = scc

scc_SOURCES = \
        @srcdir@/scc.cpp \
        @srcdir@/sccapp.cpp @srcdir@/sccapp.h \
        @srcdir@/sccreloader.cpp @srcdir@/sccreloader.h \
        @srcdir@/sccframe.cpp @srcdir@/sccframe.h \
        @srcdir@/sccipc.cpp @srcdir@/sccipc.h \
        @srcdir@/sccmenu.cpp @srcdir@/sccmenu.h \
        @srcdir@/scctunnelframe.cpp @srcdir@/scctunnelframe.h \
        @srcdir@/szarpconfigs.cpp @srcdir@/szarpconfigs.h \
        @srcdir@/szarpfonts.cpp @srcdir@/szarpfonts.h \
	@srcdir@/scchideselectionframe.cpp @srcdir@/scchideselectionframe.h

if MINGW32_OPT
RESOURCES = resources.o
else
RESOURCES = 
endif

scc_DEPENDENCIES = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP) $(LIBSZARP2)
scc_LDADD = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP) @WX_LIBS@ @XML_LIBS@ @LUA_LIBS@ @BOOST_LDFLAGS@ @BOOST_DATE_TIME_LIB@ @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_LOCALE_LIB@

# Creating resources file with icons for Windows
ICON_SRC := ../../resources/wx/icons/szarp64.xpm
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

all: scc$(EXEEXT) $(scc_LOCALES)

if !DEB_BUILD
# Automatyczne odpalanie scc przy starcie X-ów.
autostart:
	@if [ -d /etc/X11/xinit/xinitrc.d ] ; then \
		XINITDIR=/etc/X11/xinit/xinitrc.d ; \
	elif [ -d /etc/X11/Xsession.d ] ; then \
		XINITDIR=/etc/X11/Xsession.d ; \
	elif [ -d /etc/X11/xinit.d ] ; then \
		XINITDIR=/etc/X11/xinit.d ; \
	fi; \
	if [ "x$$XINITDIR" != "x" ] ; then \
	  if [ $(DISTRO) = "redhat" ] ; then \
			echo -e "#!/bin/sh \nif [ -x @prefix@/bin/scc ] ; then \nexport LANG=pl_PL\n@prefix@/bin/scc -geometry +0-30 \$$USER &\nfi"\
				> $$XINITDIR/scc ; \
			chmod +x $$XINITDIR/scc ; \
		elif [ $(DISTRO) = "debian" ] ; then \
			echo -e "#!/bin/sh \nif [ -x @prefix@/bin/scc ] ; then \nexport LANG=pl_PL\n/@prefix@/bin/scc -geometry +0-30 \$$USER &\nfi"\
		 		> $$XINITDIR/79scc ; \
			if [ ! -f /etc/xprofile ] ; then \
				echo ". $$XINITDIR/79scc" > /etc/xprofile ; \
			fi ;\
		fi ; \
	fi ; 

else DEB_BUILD
autostart:

endif

install-data-hook: autostart

# Install locales
install-data-local: $(scc_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(scc_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES

winsetup: ${WIN_APP_NAME}Setup.exe

${WIN_APP_NAME}Setup.exe: scc$(EXEEXT) $(scc_LOCALES)
	@srcdir@/../../script/nsis/winsetup.sh ${WIN_APP_NAME} scc$(EXEEXT) \
		-d resources/locales/pl_PL/LC_MESSAGES \
		$(scc_LOCALES) \
		-d resources/wx/anim \
		@srcdir@/../../resources/wx/anim/*png \
		-d resources/documentation/new/scc/html \
		@srcdir@/../../resources/documentation/new/scc/html/*.h* \
		-d resources/documentation/new/scc/html \
		@srcdir@/../../resources/documentation/new/scc/html/*.map 
        
update-po:
	@XGETTEXT@ -o $(scc_LOCALES_TMP) -L C++ --keyword=_ $(scc_SOURCES)
	@MSGMERGE@ -U $(scc_LOCALES_SRC) $(scc_LOCALES_TMP)
