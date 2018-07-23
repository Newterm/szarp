# Makefile.am for Unit Tests for pserverLITE program.

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'

SOURCE_DIR=@srcdir@

# Compilations flags
AM_CPPFLAGS = \
	-DPREFIX=\"@prefix@\" \
	-I@srcdir@/.. \
	-I@srcdir@/../../../libSzarp/include \
	-I@srcdir@/../../../libSzarp2/include \
	-I@srcdir@/../../../libSzarp2/include/szbase \
	@XML_CFLAGS@ @LUA_CFLAGS@ $(BOOST_CPPFLAGS)

LIBSZARP=../../../libSzarp/libSzarp.la
LIBSZARP2=../../../libSzarp2/libSzarp2.la

LDADD = \
	$(LIBSZARP) $(LIBSZARP2) \
	@CPPUNIT_LIBS@ \
	@LUA_LIBS@ @XML_LIBS@ @BOOST_SYSTEM_LIB@ \
	@BOOST_DATE_TIME_LIB@ @BOOST_LOCALE_LIB@ \
	@BOOST_THREAD_LIB@ @BOOST_FILESYSTEM_LIB@ \
	@PTHREAD_CFLAGS@

TESTS = command-handlers-unit-tests

bin_PROGRAMS = command-handlers-unit-tests

command_handlers_unit_tests_SOURCES = \
		unit_tests.cpp \
		command_handlers_unit_test.cpp \
		../commands.cc \
		../szcache.cpp \
		../shm_connection.cc

