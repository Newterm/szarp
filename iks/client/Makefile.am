# No subdirectories
SUBDIRS = 

AM_CPPFLAGS = \
	-DPREFIX=\"@prefix@\" \
	-I@srcdir@ \
	-I@srcdir@/../common \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
	@XML_CFLAGS@ @XSLT_CFLAGS@ @LUA_CFLAGS@

LDADD = @PTHREAD_CFLAGS@ \
	@BOOST_LDFLAGS@ @BOOST_ASIO_LIB@ @BOOST_SYSTEM_LIB@ \
	@BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ \
	@BOOST_PROGRAM_OPTIONS_LIB@ @BOOST_LOCALE_LIB@

lib_LIBRARIES = libiks-client.a

noinst_HEADERS = \
	iks_connection.h
	tcp_client_socket.h \
	sz4_iks.h \
	sz4_iks_templ.h

libiks_client_a_SOURCES = \
	iks_connection.cpp \
	tcp_client_socket.cpp \
	sz4_iks.cpp
