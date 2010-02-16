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

LDADD = @PTHREAD_CFLAGS@ @XML_LIBS@ @LUA_LIBS@ @BOOST_LDFLAGS@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@

# Programs to create

bin_PROGRAMS = \
              alstdmn \
	      calecdmn \
	      dbdmn \
              fp2kdmn \
              endmn \
	      execdmn \
	      ifcl5dmn \
              ipcinfo \
              lbdmn \
	      ratedmn \
	      lecmdmn \
              linedmn \
	      kamsdmn \
	      kwmsdmn \
	      mbrtudmn \
	      k601dmn \
	      mbtcpdmn \
	      nrsdmn \
              pafdmn \
              pafdmnr \
              parcook \
	      polludmn \
	      calcdmn \
              rsdmn \
	      tcpdmn \
	      testdmn \
              wagadmn \
	      mbusdmn \
	      wlkdmn \
	      @DLMSDMN@ \
	      @UNITEDMN@ \
	      sbusdmn \
	      @PSETD@ \
	      @DDEDMN@ \
	      aqtdmn \
              dpafdmn \
	      dprdmn \
	      eapnetdmn \
	      lecdmn \
	      mbdmn \
	      lmldmn \
	      tensdmn \
	      muksdmn \
	      npozytondmn \
	      melsdmn \
	      pozytondmn \
	      sre2dmn \
              zmddmn 

EXTRA_PROGRAMS = psetd ddedmn unitedmn dlmsdmn

dpafdmn_SOURCES = dpafdmn.cc
dpafdmn_LDADD = $(LIBSZARP)

lbdmn_SOURCES = lbdmn.cc
lbdmn_LDADD = $(LIBSZARP)

alstdmn_SOURCES = alstdmn.cc
alstdmn_LDADD = $(LIBSZARP)

calecdmn_SOURCES = calecdmn.cc
calecdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

dbdmn_SOURCES = dbdmn.cc
dbdmn_LDADD =  $(LIBSZARP2) $(LIBSZARP) $(LDADD)

dprdmn_SOURCES = dprdmn.cc
dprdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

eapnetdmn_SOURCES = eapnetdmn.cc eapnet-prop-plugin.h
eapnetdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

endmn_SOURCES = endmn.cc
endmn_LDADD = $(LIBSZARP)

execdmn_SOURCES = execdmn.cc
execdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

fp2kdmn_SOURCES = fp2kdmn.cc
fp2kdmn_LDADD = $(LIBSZARP)

ipcinfo_SOURCES = ipcinfo.cc
ipcinfo_LDADD = $(LIBSZARP)

kamsdmn_SOURCES = kamsdmn.cc kams-prop-plugin.h
kamsdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) \
		$(XML_LIBS) $(BOOST_LDFLAGS) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB) $(BOOST_DATE_TIME_LIB)

kwmsdmn_SOURCES = kwmsdmn.cc
kwmsdmn_LDADD = $(LIBSZARP)

linedmn_SOURCES = linedmn.cc
linedmn_LDADD = $(LIBSZARP)

lmldmn_SOURCES = lmldmn.cc
lmldmn_LDADD = $(LIBSZARP)

mbrtudmn_SOURCES = mbrtudmn.cc
mbrtudmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

k601dmn_SOURCES = k601dmn.cc k601-prop-plugin.h
k601dmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

ifcl5dmn_SOURCES = ifcl5dmn.cc
ifcl5dmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

mbtcpdmn_SOURCES = mbtcpdmn.cc
mbtcpdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

pafdmn_SOURCES = pafdmn.cc
pafdmn_LDADD = $(LIBSZARP)

pafdmnr_SOURCES = pafdmnr.cc
pafdmnr_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

polludmn_SOURCES = polludmn.cc
polludmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

sre2dmn_SOURCES = sre2dmn.cc
sre2dmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

calcdmn_SOURCES = calcdmn.cc
calcdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

ratedmn_SOURCES = ratedmn.cc
ratedmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

lecdmn_SOURCES = lecdmn.cc
lecdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

lecmdmn_SOURCES = lecdmn.cc
lecmdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

tensdmn_SOURCES = tensdmn.cc
tensdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

muksdmn_SOURCES = muksdmn.cc
muksdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

aqtdmn_SOURCES = aqtdmn.cc
aqtdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

pozytondmn_SOURCES = pozytondmn.cc pozyton-prop-plugin.h
pozytondmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

npozytondmn_SOURCES = npozytondmn.cc npozyton-prop-plugin.h
npozytondmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

rsdmn_SOURCES = nrsdmn.cc
rsdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

nrsdmn_SOURCES = nrsdmn.cc
nrsdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

tcpdmn_SOURCES = tcpdmn.cc
tcpdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

testdmn_SOURCES = testdmn.cc
testdmn_LDADD = $(LIBSZARP)

wagadmn_SOURCES = wagadmn.cc
wagadmn_LDADD = $(LIBSZARP)

zmddmn_SOURCES = zmddmn.cc
zmddmn_LDADD = $(LIBSZARP)

parcook_SOURCES = parcook.cc funtable.cc funtable.h
parcook_LDADD =  $(LIBSZARP2) $(LIBSZARP) $(LDADD)

psetd_SOURCES = psetd.cc
psetd_LDADD = $(LDAP_LIBS) $(SSL_LIBS) $(LIBSZARP2) $(LIBSZARP) $(LDADD)

mbusdmn_SOURCES = mbusdmn.cc
mbusdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD)

wlkdmn_SOURCES = wlkdmn.cc
wlkdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD) $(BOOST_LDFLAGS) $(BOOST_DATE_TIME_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_REGEX_LIB) $(BOOST_PROGRAM_OPTIONS_LIB)

dlmsdmn_SOURCES = dlmsdmn.cc
dlmsdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD) $(BOOST_LDFLAGS) $(LBER_LIBS)

melsdmn_SOURCES = melsdmn.cc mels-prop-plugin.h
melsdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD) $(BOOST_LDFLAGS)

unitedmn_SOURCES = unitedmn.cc
unitedmn_LDADD = $(LIBSZARP2) $(LIBSZARP) @EVENT_LIBS@ @XML_LIBS@ @PTHREAD_CFLAGS@ $(BOOST_DATE_TIME_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)

mbdmn_SOURCES = mbdmn.cc
mbdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) @EVENT_LIBS@ @XML_LIBS@ @PTHREAD_CFLAGS@ $(BOOST_DATE_TIME_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)

sbusdmn_SOURCES = sbusdmn.cc
sbusdmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD) $(BOOST_LDFLAGS)

ddedmn_SOURCES = ddedmn.cc
ddedmn_LDADD = $(LIBSZARP2) $(LIBSZARP) $(LDADD) $(BOOST_LDFLAGS) $(XMLRPC_EPI_LIBS) $(CURL_LIBS)
