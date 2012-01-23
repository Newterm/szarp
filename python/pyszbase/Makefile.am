
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6789 2009-05-29 12:03:33Z pawel $

SUBDIRS = 


INCLUDES = @XML_CFLAGS@ @XSLT_CFLAGS@ @CURL_CFLAGS@ @LUA_CFLAGS@ \
	   @BOOST_CPPFLAGS@ @ZIP_CFLAGS@ -I@PYTHON_INCLUDE_DIR@ \
	-I../../libSzarp2/include			\
	-I../../libSzarp2/szbase			\
	-I../../libSzarp/include			\
	-DPREFIX=\"@prefix@\"

lib_LTLIBRARIES = libpyszbase.la

libpyszbase_la_SOURCES = pyszbase.cc
libpyszbase_la_LDFLAGS = -version-info 0:0:0 @BOOST_PYTHON_LIB@ -l@PYTHON_LIB@ @PTHREAD_CFLAGS@ @XML_LIBS@ @LUA_LIBS@ @BOOST_LDFLAGS@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ @CURL_LIBS@ @XSLT_LIBS@ @ZIP_LIBS@
libpyszbase_la_LIBADD = ../../libSzarp/libSzarp.la ../../libSzarp2/libSzarp2.la

