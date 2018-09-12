# Makefile.am for check_paramd program.

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Source directory
SOURCE_DIR=@srcdir@


# Compilations flags
AM_CPPFLAGS = @XML_CFLAGS@ @XSLT_CFLAGS@ @CURL_CFLAGS@ @LUA_CFLAGS@ \
	@BOOST_CPPFLAGS@ @ZIP_CFLAGS@ \
	-I$(SOURCE_DIR)/../../libSzarp2/include \
	-I$(SOURCE_DIR)/../../libSzarp/include

bin_PROGRAMS = check_paramd

LIBSZARP2 = ../../libSzarp2/libSzarp2.la
LIBSZARP = ../../libSzarp/libSzarp.la

LDADD = $(LIBSZARP2) $(LIBSZARP) @EVENT_LIBS@ @PTHREAD_CFLAGS@ @XML_LIBS@ @LUA_LIBS@ $(BOOST_LDFLAGS) $(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB) $(BOOST_DATE_TIME_LIB) $(BOOST_REGEX_LIB) $(BOOST_LOCALE_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) @ZMQ_LIBS@ @PROTOBUF_LIBS@ $(CURL_LIBS)


check_paramd_SOURCES = check_paramd.cpp
check_paramd_LDADD = $(LDADD)
