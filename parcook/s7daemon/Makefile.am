# Makefile.am for s7dmn program.

# No subdirectories
SUBDIRS = 

# Compilations flags
AM_CPPFLAGS = @CPPUNIT_CFLAGS@ @XML_CFLAGS@ @LUA_CFLAGS@ $(BOOST_CPPFLAGS) \
	-DPREFIX=\"@prefix@\" \
	-I@srcdir@ \
	-I@srcdir@/../../include \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
	-I@srcdir@/../../libSzarp2/include/szbase

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la

LDADD = $(LIBSZARP2) $(LIBSZARP) @CPPUNIT_LIBS@ @EVENT_LIBS@ @PTHREAD_CFLAGS@ @XML_LIBS@ @LUA_LIBS@ $(BOOST_LDFLAGS) $(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB) $(BOOST_DATE_TIME_LIB) $(BOOST_REGEX_LIB) $(BOOST_LOCALE_LIB) @ZMQ_LIBS@ @PROTOBUF_LIBS@

bin_PROGRAMS = s7dmn szpmap_test

basedmn_SOURCES = ../base_daemon.cc ../base_daemon.h

s7dmn_SOURCES = s7dmn.cc szpmap.cc s7client.cc s7qmap.cc s7query.cc $(basedmn_SOURCES)
s7dmn_LDADD = $(LDADD) -lsnap7

szpmap_test_SOURCES = szpmap_test.cc szpmap.cc

