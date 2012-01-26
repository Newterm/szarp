
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6573 2009-04-03 12:55:39Z pawel $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Compilations flags
INCLUDES = @WX_CXXFLAGS@ @WXGL_CFLAGS@ @XML_CFLAGS@ @LUA_CFLAGS@ @FREETYPE_CFLAGS@ @FTGL_CFLAGS@ @BOOST_CPPFLAGS@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
        -I@srcdir@/../../include \
	-I@srcdir@/../common \
	-DPREFIX=\"@prefix@\" 

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la 
LIBWXCOMMON=../common/libwxcommon.la

DISTRO=@distro@

WIN_APP_NAME=KOPER

koper_LOCALES = pl/koper.mo
koper_LOCALES_SRC = pl/koper.po
koper_LOCALES_TMP = pl/koper.pot

CLEANFILES = $(koper_LOCALES)

EXTRA_DIST = $(koper_LOCALES_SRC) $(koper_LOCALES)

# Program to create

bin_PROGRAMS = koper

koper_SOURCES = \
	koperapp.h \
	koperapp.cpp \
	koperframe.h \
	koperframe.cpp \
       	koperslot.h \
	koperslot.cpp \
	kopervfetch.h \
	kopervfetch.cpp \
	koperpopup.h \
	koperpopup.cpp

koper_DEPENDENCIES = $(LIBWXCOMMON) $(LIBSZARP) $(LIBSZARP2)
koper_LDADD = $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP) @WX_LIBS@ @XML_LIBS@ @LUA_LIBS@ @FTGL_LIBS@ @WXGL_LIBS@ @BOOST_LDFLAGS@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@

# Rule to create mo files
%.mo: %.po
	mkdir -p `dirname $@`
	@MSGFMT@ -o $@ $<

all: koper$(EXEEXT) $(koper_LOCALES)

# Install locales
install-data-local: $(koper_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(koper_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES

winsetup: ${WIN_APP_NAME}Setup.exe

${WIN_APP_NAME}Setup.exe: koper$(EXEEXT) $(koper_LOCALES)
	@srcdir@/../../script/nsis/winsetup.sh ${WIN_APP_NAME} koper$(EXEEXT) \
		-d resources/locales/pl_PL/LC_MESSAGES \
		$(koper_LOCALES) \
		-d resources/wx/anim \
		@srcdir@/../../resources/wx/anim/*png \
		-d resources/documentation/new/koper/html \
		@srcdir@/../../resources/documentation/new/koper/html/*.h* \
		-d resources/documentation/new/koper/html \
		@srcdir@/../../resources/documentation/new/koper/html/*.map 
        
update-po:
	@XGETTEXT@ -o $(koper_LOCALES_TMP) -L C++ --keyword=_ $(koper_SOURCES)
	@MSGMERGE@ -U $(koper_LOCALES_SRC) $(koper_LOCALES_TMP)
