# Makefile.am for Unit Tests for pserverLITE program.

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

SOURCE_DIR=@srcdir@

# Compilations flags
AM_CPPFLAGS = \
	-I@srcdir@/.. \
	-I@srcdir@/../../../libSzarp/include \
	-I@srcdir@/../../../libSzarp2/include

LDADD = \
	@CPPUNIT_LIBS@

bin_PROGRAMS = command-handlers-unit-tests

command_handlers_unit_tests_SOURCES = \
		unit_tests.cpp \
		command_handlers_unit_test.cpp \
		../commands.cc \
		../szcache.cpp

