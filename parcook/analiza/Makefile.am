
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6215 2008-11-20 16:30:17Z reksio $

# No subdirectories
SUBDIRS = 

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Compilations flags
AM_CPPFLAGS = @XML_CFLAGS@ \
	   @LUA_CFLAGS@ \
	   @BOOST_CPPFLAGS@ \
	   -I@srcdir@/../../libSzarp/include \
	   -I@srcdir@/../../libSzarp2/include  \
           -I../../include -DPREFIX=\"@prefix@\"

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la
LIBSZARPFB = ../../libSzarpfb/libSzarpfb.a

LDADD = $(LIBSZARP2) $(LIBSZARP) @PTHREAD_CFLAGS@ @XML_LIBS@ @LUA_LIBS@ @BOOST_LDFLAGS@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ @BOOST_LOCALE_LIB@

# Programs to create

bin_PROGRAMS = \
               analiza

analiza_SOURCES = \
               analiza.cc analiza.h
