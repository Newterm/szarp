#
# $Id: Makefile.am 6725 2009-05-18 09:16:48Z reksio $

SUBDIRS =

if MINGW32_OPT
#XXX - autoconf should handle this
MINGW32_LIBS = -liconv -lwsock32 -lgdi32 -lshlwapi -liphlpapi -lws2_32
RESOURCES := resources.o
else
MINGW32_LIBS =
SSS_rule = sss$(EXEEXT)
RESOURCES :=
endif

SSC_rule = ssc$(EXEEXT)

INCLUDES = @GDK_CFLAGS@ @GTK_CFLAGS@ @WX_CXXFLAGS@ @XML_CFLAGS@ @RSYNC_CFLAGS@ @SSL_CFLAGS@ @LUA_CFLAGS@ @BOOST_CPPFLAGS@ @CARES_CFLAGS@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
        -I@srcdir@/../../include \
	-I@srcdir@/../common \
	-I@srcdir@/../../libSzarp2/misc \
	-DPREFIX=\"@prefix@\"

LIBSZARP=../../libSzarp/libSzarp.a
LIBSZARP2=../../libSzarp2/libSzarp2.a
LIBWXCOMMON=../common/libwxcommon.a

ssc_DEPENDENCIES = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP) $(LIBSZARP2)
ssc_LDADD = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP) $(LIBSZARP2) \
	@WX_LIBS@ @RSYNC_LIBS@ @SSL_LIBS@ @LUA_LIBS@ @XML_LIBS@ $(MINGW32_LIBS) @BOOST_LDFLAGS@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @CARES_LIBS@ @BOOST_THREAD_LIB@
sss_LDADD = @RSYNC_LIBS@ @SSL_LIBS@ @PAM_LIBS@ @XML_LIBS@ @LUA_LIBS@ @BOOST_LDFLAGS@ @BOOST_THREAD_LIB@ $(LIBSZARP) $(LIBSZARP2)

ssc_CXXFLAGS= -DUSE_WX $(AM_FLAGS)

ssc_LOCALES = pl/ssc.mo
ssc_LOCALES_SRC = pl/ssc.po
ssc_LOCALES_TMP = pl/ssc.pot

bin_SCRIPTS = ssc_sudowrap.sh ssconfd/ssconf.py
if MINGW32_OPT
bin_PROGRAMS = ssc
else
bin_PROGRAMS = ssc sss
endif

WIN_APP_NAME=SSC

# Creating resources file with icons for Windows
ICON_SRC := ../../resources/wx/icons/ssc64.xpm
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

ssc_SOURCES = \
	ssexception.h \
	ssexception.cc \
	sspackexchange.h \
	sspackexchange.cc \
	ssclient.h \
	ssclient.cc \
	ssutil.h \
	ssutil.cc \
	ssfsutil.h \
	ssfsutil.cc \
	ssstring.h \
	hwkey.cc \
	hwkey.h

sss_SOURCES = \
	ssexception.h \
	ssexception.cc \
	sspackexchange.h \
	sspackexchange.cc \
	ssserver.h \
	ssserver.cc \
	ssfsutil.h \
	ssfsutil.cc \
	ssutil.h \
	ssutil.cc \
	ssuserdb.h \
	ssuserdb.cc \
	sstring.h \
	hwkey.cc \
	latin2.h \
	latin2.c \
	hwkey.h


all: $(SSC_rule) $(SSS_rule) $(ssc_LOCALES)

if !DEB_BUILD
install-data-local: install-locales
else
install-data-local: install-locales install-ssconf-etc
endif

install-locales: $(ssc_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(ssc_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES

install-ssconf-etc:
	$(INSTALL) -d $(DESTDIR)/etc/szarp
	$(INSTALL) ssconfd/ssconf.cfg $(DESTDIR)/etc/szarp

update-po:
	@XGETTEXT@ -o $(ssc_LOCALES_TMP) -L C++ --keyword=_ $(ssc_SOURCES)
	@MSGMERGE@ -U $(ssc_LOCALES_SRC) $(ssc_LOCALES_TMP)
	rm -f $(ssc_LOCALES_TMP)

winsetup: ${WIN_APP_NAME}Setup.exe

${WIN_APP_NAME}Setup.exe: ssc$(EXEEXT) $(ssc_LOCALES)
	@srcdir@/../../script/nsis/winsetup.sh ${WIN_APP_NAME} ssc$(EXEEXT) \
		-d resources/locales/pl_PL/LC_MESSAGES \
		$(ssc_LOCALES) \
		-d resources/documentation/new/ssc/html \
		@srcdir@/../../resources/documentation/new/ssc/html/*.h* \
		-d resources/documentation/new/ssc/html \
		@srcdir@/../../resources/documentation/new/ssc/html/*.map \
		-d resources/documentation/new/ssc/html/pics \
		@srcdir@/../../resources/documentation/new/ssc/html/pics/*.* 
	@echo Make sure docs are generated!!!

