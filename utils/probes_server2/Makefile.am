# No subdirectories
SUBDIRS = net

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Compilations flags
AM_CPPFLAGS = \
	-DPREFIX=\"@prefix@\" \
	-I@srcdir@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
	-I@srcdir@/../../libSzarp2/include/szbase

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la

LDADD = \
	net/libprobes-server-net.a \
	@PTHREAD_CFLAGS@ $(LIBSZARP) $(LIBSZARP2) \
	@BOOST_LDFLAGS@ @BOOST_ASIO_LIB@ @BOOST_SYSTEM_LIB@ \
	@BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ \
	@BOOST_PROGRAM_OPTIONS_LIB@ @BOOST_LOCALE_LIB@

bin_PROGRAMS = probes_server2

probes_server2_SOURCES = \
	szbcache.cpp \
	daemon.cpp \
	handler.cpp \
	global_service.cpp \
	thread_pool.cpp \
	probes_srv.cpp \
	main.cpp


