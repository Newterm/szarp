# No subdirectories
SUBDIRS=

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

INCLUDES = @CPPUNIT_CFLAGS@ -I../libSzarp2/include -I../libSzarp/include 

LIBS = ../libSzarp2/libSzarp2.la ../libSzarp/libSzarp.la @CPPUNIT_LIBS@

bin_PROGRAMS = unit_tests

unit_tests_SOURCES = \
		unit_tests.cpp \
		sz4_block_unit_test.cpp
