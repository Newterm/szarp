
# Pawe� Pa�ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6789 2009-05-29 12:03:33Z pawel $

SUBDIRS = 


AM_CPPFLAGS = @XML_CFLAGS@ @XSLT_CFLAGS@ @CURL_CFLAGS@ @LUA_CFLAGS@ \
	   @BOOST_CPPFLAGS@ @ZIP_CFLAGS@ $(PYTHON_CPPFLAGS) \
	-I@srcdir@/../../libSzarp2/include			\
	-I@srcdir@/../../libSzarp2/szbase			\
	-I@srcdir@/../../libSzarp/include			\
	-DPREFIX=\"@prefix@\"

lib_LTLIBRARIES = libpyszbase.la

libpyszbase_la_SOURCES = pyszbase.cc
libpyszbase_la_LDFLAGS = -version-info 0:0:0 -l$(BOOST_PYTHON_LIB) \
			$(PYTHON_LDFLAGS) $(PTHREAD_CFLAGS) \
			$(XML_LIBS) $(LUA_LIBS) $(BOOST_LDFLAGS) \
			$(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB) \
			$(BOOST_THREAD_LIB) $(BOOST_DATE_TIME_LIB) \
			$(PROTOBUF_LIBS) $(ZMQ_LIBS)\
			@BOOST_LOCALE_LIB@ @CURL_LIBS@ @XSLT_LIBS@ @ZIP_LIBS@ @EVENT_LIBS@
libpyszbase_la_LIBADD = ../../libSzarp/libSzarp.la ../../libSzarp2/libSzarp2.la

