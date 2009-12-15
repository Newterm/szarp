
#
# $Id: Makefile.am 222 2009-03-18 23:25:18Z asmyk $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Compilations flags
INCLUDES = @WX_CXXFLAGS@ @XML_CFLAGS@ @LUA_CFLAGS@ @BOOST_CPPFLAGS@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
        -I@srcdir@/../../include -I@srcdir@/../common 

LIBSZARP=../../libSzarp/libSzarp.a
LIBSZARP2=../../libSzarp2/libSzarp2.a 
LIBWXCOMMON=../common/libwxcommon.a

# Program to create

bin_PROGRAMS = visio

visio_SOURCES = \
	visioApp.cpp visioFetchFrame.cpp visioTransparentFrame.cpp visioParamadd.cpp \
	visioApp.h visioFetchFrame.h visioTransparentFrame.h visioParamadd.h


visio_DEPENDENCIES = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP)
visio_LDADD = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP) @XML_LIBS@ @WX_LIBS@ @LUA_LIBS@ @CURL_LIBS@ @BOOST_LDFLAGS@ @BOOST_DATE_TIME_LIB@ @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@

visio_LOCALES = pl/visio.mo
visio_LOCALES_SRC = pl/visio.po
visio_LOCALES_TMP = pl/visio.pot

CLEANFILES = $(visio_LOCALES)
EXTRA_DIST = $(visio_LOCALES_SRC) 

# Rule to create mo files
%.mo: %.po
	mkdir -p `dirname $@`
	@MSGFMT@ -o $@ $<


all: visio$(EXEEXT) $(visio_LOCALES)

# Install locales
install-data-local: $(visio_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(visio_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES

update-po:
	@XGETTEXT@ -o $(visio_LOCALES_TMP) -L C++ --keyword=_ $(visio_SOURCES)
	@MSGMERGE@ -U $(visio_LOCALES_SRC) $(visio_LOCALES_TMP)
	rm -f $(visio_LOCALES_TMP)

