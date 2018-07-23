# Makefile.am for pserverLITE program.

# No subdirectories
SUBDIRS = unit_tests

# Additional files for deleting on 'make maintainer-clean'

# Compilations flags
AM_CPPFLAGS = \
	-DPREFIX=\"@prefix@\" \
	-I@srcdir@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
	-I@srcdir@/../../libSzarp2/include/szbase \
	@XML_CFLAGS@ @LUA_CFLAGS@ $(BOOST_CPPFLAGS)

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la

LDADD = \
	$(LIBSZARP) $(LIBSZARP2) \
	@BOOST_PROGRAM_OPTIONS_LIB@ @EVENT_LIBS@ \
	@LUA_LIBS@ @XML_LIBS@ @BOOST_SYSTEM_LIB@ \
	@BOOST_DATE_TIME_LIB@ @BOOST_LOCALE_LIB@ \
	@BOOST_THREAD_LIB@ @BOOST_FILESYSTEM_LIB@ \
	@PTHREAD_CFLAGS@

TESTS = szcache_test

bin_PROGRAMS = pserver-lite szcache_test

pserver_lite_SOURCES = \
	main.cc pserver_service.cpp commands.cc szcache.cpp \
	shm_connection.cc

szcache_test_SOURCES = \
	szcache.cpp szcache_test.cpp shm_connection.cc
