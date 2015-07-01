# Makefile.am for pserverLITE program.

# No subdirectories
SUBDIRS =

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
	$(LIBSZARP) \
	@BOOST_PROGRAM_OPTIONS_LIB@

bin_PROGRAMS = pserver-lite

pserver_lite_SOURCES = \
	main.cc

