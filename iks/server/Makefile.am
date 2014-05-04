# No subdirectories
SUBDIRS = 

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Compilations flags
AM_CPPFLAGS = \
	-I@srcdir@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include

LIBSZARP=../../libSzarp/libSzarp.la
LIBSZARP2=../../libSzarp2/libSzarp2.la

LDADD = @PTHREAD_CFLAGS@ $(LIBSZARP2) $(LIBSZARP) \
	@BOOST_LDFLAGS@ @BOOST_ASIO_LIB@ @BOOST_SYSTEM_LIB@ \
	@BOOST_SIGNALS_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ \
	@BOOST_PROGRAM_OPTIONS_LIB@

bin_PROGRAMS = iks-server

iks_server_SOURCES = \
	locations/welcome/welcome.cpp \
	locations/location.cpp \
	locations/manager.cpp \
	net/tcp_client.cpp \
	net/tcp_server.cpp \
	main.cpp \
	global_service.cpp

