
#
# $Id: Makefile.am 222 2009-03-18 23:25:18Z asmyk $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Compilations flags
INCLUDES = @WX_CXXFLAGS@ @XML_CFLAGS@ @CURL_CFLAGS@ @LUA_CFLAGS@ @BOOST_CPPFLAGS@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
        -I@srcdir@/../../include \
	-I@srcdir@/../common \
	-DPREFIX=\"@prefix@\"

LIBSZARP=../../libSzarp/libSzarp.a
LIBSZARP2=../../libSzarp2/libSzarp2.a 
LIBWXCOMMON=../common/libwxcommon.a

# Program to create

bin_PROGRAMS = viszio

WIN_APP_NAME=viszio

viszio_SOURCES = \
	viszioApp.cpp viszioFetchFrame.cpp viszioTransparentFrame.cpp viszioParamadd.cpp \
	viszioApp.h viszioFetchFrame.h viszioTransparentFrame.h viszioParamadd.h


viszio_DEPENDENCIES = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP) $(LIBSZARP2)
viszio_LDADD = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP) @XML_LIBS@ @WX_LIBS@ @LUA_LIBS@ @CURL_LIBS@ @BOOST_LDFLAGS@ @BOOST_DATE_TIME_LIB@ @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@

viszio_LOCALES = pl/viszio.mo
viszio_LOCALES_SRC = pl/viszio.po
viszio_LOCALES_TMP = pl/viszio.pot

CLEANFILES = $(viszio_LOCALES)
EXTRA_DIST = $(viszio_LOCALES_SRC) 

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

