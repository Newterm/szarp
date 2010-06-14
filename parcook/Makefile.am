# SZARP Scada
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6824 2009-06-05 13:51:11Z pawel $

SUBDIRS = analiza

# Source directory
SOURCE_DIR=@srcdir@

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Compilations flags
INCLUDES = @XML_CFLAGS@ @LUA_CFLAGS@ \
           -I$(SOURCE_DIR)/../libSzarp/include		\
           -I$(SOURCE_DIR)/../libSzarp2/include		\
           -I$(SOURCE_DIR)/../include			\
	   $(BOOST_CPPFLAGS)				\
	   $(XMLRPC_EPI_CFLAGS)				\
	   -DPREFIX=\"@prefix@\"

LIBSZARP2 = ../libSzarp2/libSzarp2.a
LIBSZARP = ../libSzarp/libSzarp.a

LDADD = $(LIBSZARP2) $(LIBSZARP) @PTHREAD_CFLAGS@ @XML_LIBS@ @LUA_LIBS@ @BOOST_LDFLAGS@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@

# Programs to create

bin_PROGRAMS = \
              alstdmn \
	      aqtdmn \
	      calcdmn \
	      calecdmn \
	      dbdmn \
	      @DDEDMN@ \
	      @DLMSDMN@ \
              dpafdmn \
	      dprdmn \
	      eapnetdmn \
              endmn \
	      execdmn \
              fp2kdmn \
	      ifcl5dmn \
              ipcinfo \
	      k601dmn \
	      kamsdmn \
	      kwmsdmn \
              lbdmn \
	      lecdmn \
	      lecmdmn \
              linedmn \
	      lmldmn \
	      mbdmn \
	      mbrtudmn \
	      mbtcpdmn \
	      mbusdmn \
	      melsdmn \
	      muksdmn \
	      npozytondmn \
	      nrsdmn \
              pafdmn \
              pafdmnr \
              parcook \
	      polludmn \
	      pozytondmn \
	      @PSETD@ \
	      ratedmn \
              rsdmn \
	      sbusdmn \
	      setdmn \
	      iecdmn \
	      prodmn \
	      sre2dmn \
	      tcpdmn \
	      tensdmn \
	      testdmn \
	      @UNITEDMN@ \
	      wlkdmn \
              zmddmn 

EXTRA_PROGRAMS = psetd ddedmn unitedmn dlmsdmn

dpafdmn_SOURCES = dpafdmn.cc

lbdmn_SOURCES = lbdmn.cc

alstdmn_SOURCES = alstdmn.cc

calecdmn_SOURCES = calecdmn.cc

dbdmn_SOURCES = dbdmn.cc

dprdmn_SOURCES = dprdmn.cc

eapnetdmn_SOURCES = eapnetdmn.cc eapnet-prop-plugin.h

endmn_SOURCES = endmn.cc

execdmn_SOURCES = execdmn.cc

fp2kdmn_SOURCES = fp2kdmn.cc

ipcinfo_SOURCES = ipcinfo.cc

kamsdmn_SOURCES = kamsdmn.cc kams-prop-plugin.h
kamsdmn_LDADD = $(LDADD) $(BOOST_LDFLAGS) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB) $(BOOST_DATE_TIME_LIB)

kwmsdmn_SOURCES = kwmsdmn.cc

linedmn_SOURCES = linedmn.cc

lmldmn_SOURCES = lmldmn.cc

mbrtudmn_SOURCES = mbrtudmn.cc

k601dmn_SOURCES = k601dmn.cc k601-prop-plugin.h

ifcl5dmn_SOURCES = ifcl5dmn.cc

mbtcpdmn_SOURCES = mbtcpdmn.cc

pafdmn_SOURCES = pafdmn.cc

pafdmnr_SOURCES = pafdmnr.cc

polludmn_SOURCES = polludmn.cc

sre2dmn_SOURCES = sre2dmn.cc

calcdmn_SOURCES = calcdmn.cc

ratedmn_SOURCES = ratedmn.cc

lecdmn_SOURCES = lecdmn.cc

lecmdmn_SOURCES = lecmdmn.cc

tensdmn_SOURCES = tensdmn.cc

muksdmn_SOURCES = muksdmn.cc

aqtdmn_SOURCES = aqtdmn.cc

pozytondmn_SOURCES = pozytondmn.cc pozyton-prop-plugin.h

npozytondmn_SOURCES = npozytondmn.cc npozyton-prop-plugin.h

rsdmn_SOURCES = nrsdmn.cc

nrsdmn_SOURCES = nrsdmn.cc

tcpdmn_SOURCES = tcpdmn.cc

testdmn_SOURCES = testdmn.cc

zmddmn_SOURCES = zmddmn.cc

parcook_SOURCES = parcook.cc funtable.cc funtable.h

psetd_SOURCES = psetd.cc
psetd_LDADD = $(LDAP_LIBS) $(SSL_LIBS) $(LDADD)

mbusdmn_SOURCES = mbusdmn.cc

wlkdmn_SOURCES = wlkdmn.cc
wlkdmn_LDADD = $(LDADD) $(BOOST_LDFLAGS) $(BOOST_DATE_TIME_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_REGEX_LIB) $(BOOST_PROGRAM_OPTIONS_LIB)

dlmsdmn_SOURCES = dlmsdmn.cc
dlmsdmn_LDADD = $(LDADD) $(BOOST_LDFLAGS) $(LBER_LIBS)

melsdmn_SOURCES = melsdmn.cc mels-prop-plugin.h
melsdmn_LDADD = $(LDADD) $(BOOST_LDFLAGS)

unitedmn_SOURCES = unitedmn.cc
unitedmn_LDADD = $(LDADD) @EVENT_LIBS@ @XML_LIBS@ @PTHREAD_CFLAGS@ $(BOOST_DATE_TIME_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)

mbdmn_SOURCES = mbdmn.cc
mbdmn_LDADD = $(LDADD) @EVENT_LIBS@ @XML_LIBS@ @PTHREAD_CFLAGS@ $(BOOST_DATE_TIME_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)

sbusdmn_SOURCES = sbusdmn.cc

ddedmn_LDADD = $(LDADD) $(BOOST_LDFLAGS) $(XMLRPC_EPI_LIBS) $(CURL_LIBS)

setdmn_LDADD = $(LDADD) @EVENT_LIBS@

iecdmn_SOURCES = iecdmn.cc
iecdmn_LDADD = $(LDADD)

prodmn_SOURCES = prodmn.cc
prodmn_LDADD = $(LDADD) @EVENT_LIBS@ @XML_LIBS@ @PTHREAD_CFLAGS@ $(BOOST_DATE_TIME_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)
