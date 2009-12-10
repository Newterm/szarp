
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
	TaskbarExampleApp.cpp TaskbarExampleMain.cpp TransparentFrame.cpp visioParamadd.cpp \
	TaskbarExampleApp.h TaskbarExampleMain.h TransparentFrame.h visioParamadd.h


visio_DEPENDENCIES = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP)
visio_LDADD = $(RESOURCES) $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP) @XML_LIBS@ @WX_LIBS@ @LUA_LIBS@ @CURL_LIBS@ @BOOST_LDFLAGS@ @BOOST_DATE_TIME_LIB@ @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@

CLEANFILES = $(visio_LOCALES)
EXTRA_DIST = $(visio_LOCALES_SRC) 

all: visio$(EXEEXT)

# Install locales
install-data-local: $(visio_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(visio_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES


