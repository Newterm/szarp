# No subdirectories
SUBDIRS=

# Additional files for deleting on 'make maintainer-clean'

SOURCE_DIR=@srcdir@

AM_CPPFLAGS = @CPPUNIT_CFLAGS@ -I$(SOURCE_DIR)/../libSzarp2/include \
	-I$(SOURCE_DIR)/../libSzarp/include @XML_CFLAGS@ @XSLT_CFLAGS@ @CURL_CFLAGS@ \
	@LUA_CFLAGS@ @BOOST_CPPFLAGS@ @ZIP_CFLAGS@

LIBS = ../libSzarp2/libSzarp2.la ../libSzarp/libSzarp.la \
	@CPPUNIT_LIBS@ @XML_LIBS@ @LUA_LIBS@ @BOOST_LDFLAGS@ \
	@BOOST_LOCALE_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ \
	@BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ @CURL_LIBS@ \
	@XSLT_LIBS@ @ZIP_LIBS@ @PROTOBUF_LIBS@ @ZMQ_LIBS@

bin_PROGRAMS = unit_tests sz4_extr_simple

TESTS = unit_tests

unit_tests_SOURCES = \
	unit_tests.cpp \
	sz4_block_unit_test.cpp \
	parhublistener_test.cpp \
	szb_param_monitor_unit_test.cpp \
	sz4_file_search.cpp \
	sz4_buffer_unit_test.cpp \
	sz4_factory_test.cpp \
	sz4_base_test.cpp \
	sz4_definable_param_cache.cpp \
	test_observer.h \
	test_observer.cpp \
	base_param_converter_unit_test.cpp \
	sz4_lua_param_optimized.cpp \
	sz4_lua_param.cpp \
	sz4_combined_param.cpp \
	sz4_definable_param.cpp \
	sz4_wsum.cpp \
	sz4_live_cache.cpp \
	sz4_decode_test.cpp \
	filelogger_test.cpp \
	loghandler_test.cpp \
	zmq_handler_test.cpp \
	cmdlineparser_test.cpp \
	argsmgr_test.cpp \
	simple_mocks.h

sz4_extr_simple_SOURCES = sz4_extr_simple.cpp
