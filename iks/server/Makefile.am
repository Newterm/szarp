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
	@XML_CFLAGS@ @XSLT_CFLAGS@ @LUA_CFLAGS@

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la

LDADD = @PTHREAD_CFLAGS@ $(LIBSZARP2) $(LIBSZARP) \
	@XSLT_LIBS@ @XML_LIBS@ @LUA_LIBS@ \
	@BOOST_LDFLAGS@ @BOOST_ASIO_LIB@ @BOOST_SYSTEM_LIB@ \
	@BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ \
	@BOOST_PROGRAM_OPTIONS_LIB@

bin_PROGRAMS = iks-server

iks_server_SOURCES = \
	locations/remotes_updater/remotes_updater.cpp \
	locations/welcome/welcome.cpp \
	locations/szbase/szbase.cpp \
	locations/proxy/proxy.cpp \
	locations/location.cpp \
	locations/protocol_location.cpp \
	locations/manager.cpp \
	utils/ptree.cpp \
	utils/config_.cpp \
	utils/colors.cpp \
	data/config.cpp \
	data/param.cpp \
	data/params.cpp \
	data/params_updater.cpp \
	data/probe_type.cpp \
	data/set.cpp \
	data/sets.cpp \
	data/vars.cpp \
	data/vars_cache.cpp \
	data/szbase_wrapper.cpp \
	net/tcp_client.cpp \
	net/tcp_server.cpp \
	main.cpp \
	global_service.cpp

