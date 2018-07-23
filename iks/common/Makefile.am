
# No subdirectories
SUBDIRS = 

# Additional files for deleting on 'make maintainer-clean'

lib_LIBRARIES = libiks-common.a

AM_CPPFLAGS = @BOOST_CPPFLAGS@ @XML_CFLAGS@ @LUA_CFLAGS@ \
		-DBOOST_ENABLE_ASSERT_HANDLER \
	   -I@srcdir@/../../libSzarp/include \
	   -I@srcdir@/../../libSzarp2/include \
	   -I@srcdir@/../../include \
	   -I@srcdir@/../../extern/wxscintilla/include \
	   -DPREFIX=\"@prefix@\"

noinst_HEADERS = data/probe_type.h utils/exception.h locations/error_codes.h

libiks_common_a_SOURCES = data/probe_type.cpp utils/ptree.cpp utils/assertion_hnd.cpp
