# No subdirectories
SUBDIRS=

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

INCLUDES = @CPPUNIT_CFLAGS@ -I../libSzarp2/include -I../libSzarp/include @XML_CFLAGS@ @XSLT_CFLAGS@ @CURL_CFLAGS@ @LUA_CFLAGS@ \
	   @BOOST_CPPFLAGS@ @ZIP_CFLAGS@

LIBS = ../libSzarp2/libSzarp2.la ../libSzarp/libSzarp.la @CPPUNIT_LIBS@ @XML_LIBS@ @LUA_LIBS@ @BOOST_LDFLAGS@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ @CURL_LIBS@ @XSLT_LIBS@ @ZIP_LIBS@

bin_PROGRAMS = unit_tests

unit_tests_SOURCES = \
		unit_tests.cpp \
		sz4_block_unit_test.cpp \
		szb_param_monitor_unit_test.cpp \
		sz4_file_search.cpp \
		sz4_buffer_unit_test.cpp \
		sz4_base_test.cpp \
		sz4_definable_param_cache.cpp \
		test_observer.h \
		test_observer.cpp \
		base_param_converter_unit_test.cpp
