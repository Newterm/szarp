# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'

# Compilations flags
AM_CPPFLAGS = \
	-I@srcdir@/../libSzarp/include \
	-I@srcdir@/../libSzarp2/include

LIBSZARP=../libSzarp/libSzarp.la @BOOST_PROGRAM_OPTIONS_LIB@

LDADD = @PTHREAD_CFLAGS@ $(LIBSZARP2) $(LIBSZARP) \
	@BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ \
	@BOOST_PROGRAM_OPTIONS_LIB@ @ZMQ_LIBS@ @BOOST_LOCALE_LIB@

bin_PROGRAMS = cfgdealer cfgdealer_test

cfgdealer_SOURCES = \
	cfgdealer.cpp

cfgdealer_test_SOURCES = \
	cfgdealer_test.cpp
