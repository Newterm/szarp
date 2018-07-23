
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6789 2009-05-29 12:03:33Z pawel $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'

# Compilations flags
AM_CPPFLAGS = -I../include \
	-I@srcdir@/../include \
	-I@srcdir@/../libSzarp/include \
	-I@srcdir@/../libSzarp2/include \
	@XML_CFLAGS@ @XSLT_CFLAGS@ @LUA_CFLAGS@ \
	-DPREFIX=\"@prefix@\"

LIBSZARP=../libSzarp/libSzarp.la
LIBSZARP2=../libSzarp2/libSzarp2.la

LDADD = @PTHREAD_CFLAGS@ $(LIBSZARP2) $(LIBSZARP) @XSLT_LIBS@ @XML_LIBS@ \
	@LUA_LIBS@ @BOOST_LDFLAGS@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ \
	@BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ @BOOST_LOCALE_LIB@ @PROTOBUF_LIBS@

# Programs to create

bin_PROGRAMS = \
	meaner3 \
	peakator \
	prober \
	@SZBEDIT@ \
	@EXTRSZB@ \
	szbindex \
	szbreader \
	szbwriter \
	szbndremover \
	szbmod \
	szbnull

if !NO_LUA
bin_PROGRAMS += checker
endif

EXTRA_PROGRAMS = \
	szbedit \
	extrszb

meaner3_SOURCES = \
	meaner3.cc meaner3.h \
	tmeaner.cc tmeaner.h \
	tparcook.cc tparcook.h \
	tsaveparam.cc tsaveparam.h \
	texecute.cc texecute.h \
	texecsect.cc texecsect.h \
	texecalarm.cc texecalarm.h \
	tstatus.cc tstatus.h \
	twriter.cc twriter.h \
	classes.h

peakator_SOURCES = \
	utils/peakator.cc \
	tsaveparam.cc tsaveparam.h \
	tstatus.cc tstatus.h

prober_SOURCES = \
	fileremover.cc \
	tparcook.cc \
	tsaveparam.cc \
	twriter.cc \
	tprober.cc \
	tstatus.cc \
	prober.cc

szbreader_SOURCES = \
	utils/szbreader.cc

checker_SOURCES = \
	utils/checker.cc

szbwriter_SOURCES = \
	utils/szbwriter.cc utils/szbwriter_cache.cc utils/szbwriter_cache.h \
	utils/tmmapparam.cc utils/tmmapparam.h tsaveparam.h tsaveparam.cc tstatus.h tstatus.cc

szbedit_SOURCES = utils/szbedit.cc
szbedit_LDADD = $(LDADD) @NEWT_LIBS@

szbindex_SOURCES = utils/szbindex.cc

szbnull_SOURCES = utils/szbnull.cc

szbndremover_SOURCES = utils/szbndremover.cc \
	tsaveparam.cc tsaveparam.h \
	tstatus.cc tstatus.h

szbmod_SOURCES = utils/szbmod.cc \
	tsaveparam.cc tsaveparam.h \
	tstatus.cc tstatus.h

extrszb_SOURCES= utils/extrszb.cc
extrszb_LDADD = $(LIBSZARP) $(LIBSZARP2) \
	@PTHREAD_CFLAGS@ @XML_LIBS@ @XSLT_LIBS@ @LUA_LIBS@ @BOOST_LDFLAGS@ \
	@BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ \
	@BOOST_LOCALE_LIB@ @ZIP_LIBS@ @PROTOBUF_LIBS@ @ZMQ_LIBS@
