# No subdirectories
SUBDIRS = 

LDADD = @PTHREAD_CFLAGS@ \
	@BOOST_LDFLAGS@ @BOOST_ASIO_LIB@ @BOOST_SYSTEM_LIB@ \
	@BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ \
	@BOOST_PROGRAM_OPTIONS_LIB@ @BOOST_LOCALE_LIB@

lib_LIBRARIES = libiks-client.a

noinst_HEADERS = \
	iks_client.h
	tcp_client_socket.h

libiks_client_a_SOURCES = \
	iks_client.cpp \
	tcp_client_socket.cpp
