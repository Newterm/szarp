# No subdirectories
SUBDIRS = data net locations utils

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Compilations flags
AM_CPPFLAGS = \
	-DPREFIX=\"@prefix@\" \
	-DBOOST_ENABLE_ASSERT_HANDLER \
	-I@srcdir@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
	@XML_CFLAGS@ @XSLT_CFLAGS@ @LUA_CFLAGS@

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la

LDADD = \
	locations/libiks-server-locations.a \
	data/libiks-server-data.a \
	utils/libiks-server-utils.a \
	net/libiks-server-net.a \
	@PTHREAD_CFLAGS@ $(LIBSZARP2) $(LIBSZARP) \
	@XSLT_LIBS@ @XML_LIBS@ @LUA_LIBS@ \
	@BOOST_LDFLAGS@ @BOOST_ASIO_LIB@ @BOOST_SYSTEM_LIB@ \
	@BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ \
	@BOOST_PROGRAM_OPTIONS_LIB@ @BOOST_LOCALE_LIB@

bin_PROGRAMS = iks-server

iks_server_SOURCES = \
	main.cpp \
	daemon.cpp \
	global_service.cpp

install-data-local:
	$(INSTALL) -d $(DESTDIR)@prefix@/iks
	$(INSTALL) iks-server.ini.sample $(DESTDIR)@prefix@/iks

