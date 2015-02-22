# No subdirectories
SUBDIRS = 

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Compilations flags
AM_CPPFLAGS = \
	-DPREFIX=\"@prefix@\" \
	-I@srcdir@/../ \
	-I@srcdir@/../../../libSzarp/include

LIBSZARP=../../../libSzarp/libSzarp.la

LDADD = @PTHREAD_CFLAGS@ $(LIBSZARP) \
	@BOOST_LDFLAGS@ @BOOST_ASIO_LIB@ @BOOST_SYSTEM_LIB@ \
	@BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ \
	@BOOST_PROGRAM_OPTIONS_LIB@ @BOOST_LOCALE_LIB@

lib_LIBRARIES = libprobes-server-net.a

libprobes_server_net_a_SOURCES = \
	tcp_server.cpp

