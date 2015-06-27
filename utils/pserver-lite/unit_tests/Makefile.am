# Makefile.am for Unit Tests for pserverLITE program.

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

SOURCE_DIR=@srcdir@

# Compilations flags
AM_CPPFLAGS = \
	-I@srcdir@/..

LDADD = \
	@CPPUNIT_LIBS@

bin_PROGRAMS = pserver-lite-unit-tests

pserver_lite_unit_tests_SOURCES = \
		unit_tests.cpp \
		pserver_service_unit_test.cpp \
		../commands.cc \
		../pserver_service.cpp

