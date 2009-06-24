
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6199 2008-11-20 08:28:37Z reksio $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Compilations flags
INCLUDES = @WX_CXXFLAGS@ -I@srcdir@/../common -DPREFIX=\"@prefix@\" \
	-I@srcdir@/../../libSzarp/include

LIBWXCOMMON=../common/libwxcommon.a
LIBSZARP=../../libSzarp/libSzarp.a

# Program to create

bin_PROGRAMS = wxhelp

wxhelp_SOURCES = \
        whapp.cpp \
        whapp.h \
        wxhelp.cpp 


wxhelp_LDADD = $(LIBWXCOMMON) $(LIBSZARP) @WX_LIBS@ @BOOST_LDFLAGS@ @BOOST_THREAD_LIB@ @XML_LIBS@

