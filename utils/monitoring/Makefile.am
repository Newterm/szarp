# Makefile.am for list_szarp_test_params, check_szarp_daemons and check_paramd programs.

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Source directory
SOURCE_DIR=@srcdir@


# Compilations flags
AM_CPPFLAGS = @XML_CFLAGS@ @LUA_CFLAGS@ \
	-I$(SOURCE_DIR)/../../libSzarp2/include \
	-I$(SOURCE_DIR)/../../libSzarp/include

bin_PROGRAMS = list_szarp_test_params \
		check_szarp_daemons \
		check_paramd

LIBSZARP2 = ../../libSzarp2/libSzarp2.la
LIBSZARP = ../../libSzarp/libSzarp.la

LDADD = $(LIBSZARP2) $(LIBSZARP) @PTHREAD_CFLAGS@ @XML_LIBS@ @LUA_LIBS@ @PROTOBUF_LIBS@ $(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB) $(BOOST_DATE_TIME_LIB) $(BOOST_LOCALE_LIB) $(CURL_LIBS)


list_szarp_test_params_SOURCES = list_szarp_test_params.cpp
list_szarp_test_params_LDADD = $(LDADD)

check_szarp_daemons_SOURCES = check_szarp_daemons.cpp
check_szarp_daemons_LDADD = $(LDADD)

check_paramd_SOURCES = check_paramd.cpp
check_paramd_LDADD = $(LDADD)
