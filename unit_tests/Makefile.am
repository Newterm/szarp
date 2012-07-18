# No subdirectories
SUBDIRS=

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

INCLUDES = @CPPUNIT_CFLAGS@ -I../libSzarp2/include -I../libSzarp/include 

LIBS = ../libSzarp2/libSzarp2.la ../libSzarp/libSzarp.la @CPPUNIT_LIBS@ @BOOST_LDFLAGS@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ @BOOST_REGEX_LIB@ 

bin_PROGRAMS = unit_tests

unit_tests_SOURCES = \
		unit_tests.cpp \
		sz4_block_unit_test.cpp \
		szb_param_monitor_unit_test.cpp
