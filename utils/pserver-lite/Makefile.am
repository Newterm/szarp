# Makefile.am for pserverLITE program.

# No subdirectories
SUBDIRS = unit_tests

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Compilations flags
AM_CPPFLAGS = \
	-DPREFIX=\"@prefix@\" \
	-I@srcdir@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
	-I@srcdir@/../../libSzarp2/include/szbase

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la

LDADD = \
	$(LIBSZARP) $(LIBSZARP2) \
	@BOOST_PROGRAM_OPTIONS_LIB@ @EVENT_LIBS@

bin_PROGRAMS = pserver-lite szcache_test

pserver_lite_SOURCES = \
	main.cc pserver_service.cpp commands.cc

szcache_test_SOURCES = \
	szcache.cpp szcache_test.cpp
