# No subdirectories
SUBDIRS = 

# Additional files for deleting on 'make maintainer-clean'

# Compilations flags
AM_CPPFLAGS = \
	-DPREFIX=\"@prefix@\" \
	-DBOOST_ENABLE_ASSERT_HANDLER \
	-I@srcdir@/../ \
	-I@srcdir@/../../../libSzarp/include \
	-I@srcdir@/../../../libSzarp2/include \
	@XML_CFLAGS@ @XSLT_CFLAGS@ @LUA_CFLAGS@

LIBSZARP=../../../libSzarp/libSzarp.la
LIBSZARP2=../../../libSzarp2/libSzarp2.la

LDADD = @PTHREAD_CFLAGS@ $(LIBSZARP2) $(LIBSZARP) \
	@XSLT_LIBS@ @XML_LIBS@ @LUA_LIBS@ \
	@BOOST_LDFLAGS@ @BOOST_ASIO_LIB@ @BOOST_SYSTEM_LIB@ \
	@BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ \
	@BOOST_PROGRAM_OPTIONS_LIB@ @BOOST_LOCALE_LIB@

lib_LIBRARIES = libiks-server-utils.a

libiks_server_utils_a_SOURCES = \
	config_.cpp \
	colors.cpp

