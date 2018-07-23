# SZARP Scada
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6824 2009-06-05 13:51:11Z pawel $

SUBDIRS =

if WANT_SNAP7DMN
SUBDIRS += s7daemon
endif

# Source directory
SOURCE_DIR=@srcdir@

# Additional files for deleting on 'make maintainer-clean'

# Compilations flags
AM_CPPFLAGS = @XML_CFLAGS@ @LUA_CFLAGS@ \
           -I$(SOURCE_DIR)/../libSzarp/include		\
           -I$(SOURCE_DIR)/../libSzarp2/include		\
           -I$(SOURCE_DIR)/../include			\
	   $(ZMQ_CFLAGS)				\
	   $(PROTOBUF_CFLAGS)				\
	   $(BOOST_CPPFLAGS)				\
	   $(XMLRPC_EPI_CFLAGS)				\
	   `python-config --cflags | sed -e 's/-Wstrict-prototypes//g'` \
	   -DPREFIX=\"@prefix@\"

LIBSZARP2 = ../libSzarp2/libSzarp2.la
LIBSZARP = ../libSzarp/libSzarp.la

LDADD = $(LIBSZARP2) $(LIBSZARP) @EVENT_LIBS@ @PTHREAD_CFLAGS@ @XML_LIBS@ @LUA_LIBS@ $(BOOST_LDFLAGS) $(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB) $(BOOST_DATE_TIME_LIB) $(BOOST_REGEX_LIB) $(BOOST_LOCALE_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) @ZMQ_LIBS@ @PROTOBUF_LIBS@

# Aditional libraries for daemons using dlopen
DL = -ldl

# Programs to create

bin_PROGRAMS = \
	      aqtdmn \
	      calcdmn \
	      dbdmn \
	      @DDEDMN@ \
	      dpafdmn \
		  dprdmn \
	      execdmn \
	      ipcinfo \
	      k601dmn \
	      kamsdmn \
	      kwmsdmn \
	      lbdmn \
	      mbdmn \
	      borutadmn \
	      borutadmn_z \
	      mbrtudmn \
	      mbusdmn \
	      nrsdmn \
	      pafdmn \
	      pafdmnr \
	      parcook \
	      polludmn \
	      ratedmn \
	      rsdmn \
	      sampledmn \
	      sbusdmn \
	      iecdmn \
	      prodmn \
	      tcpdmn \
	      testdmn \
	      wlkdmn \
	      pythondmn \
	      defdmn

if WANT_SNAP7DMN
bin_PROGRAMS += snap7dmn
endif


EXTRA_PROGRAMS = ddedmn


basedmn_SOURCES = base_daemon.cc base_daemon.h

dpafdmn_SOURCES = dpafdmn.cc

lbdmn_SOURCES = lbdmn.cc

dbdmn_SOURCES = dbdmn.cc

dprdmn_SOURCES = dprdmn.cc

execdmn_SOURCES = execdmn.cc

ipcinfo_SOURCES = ipcinfo.cc

kamsdmn_SOURCES = kamsdmn.cc cfgdealer_handler.cpp
kamsdmn_LDADD = $(LDADD) $(BOOST_LDFLAGS) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB) $(BOOST_DATE_TIME_LIB) $(DL)

kwmsdmn_SOURCES = kwmsdmn.cc

mbrtudmn_SOURCES = mbrtudmn.cc

k601dmn_SOURCES = k601dmn.cc cfgdealer_handler.cpp k601-prop-plugin.h
k601dmn_LDADD = $(LDADD) $(DL)

pafdmn_SOURCES = pafdmn.cc

pafdmnr_SOURCES = pafdmnr.cc

polludmn_SOURCES = polludmn.cc

calcdmn_SOURCES = calcdmn.cc

ratedmn_SOURCES = ratedmn.cc

aqtdmn_SOURCES = aqtdmn.cc

rsdmn_SOURCES = nrsdmn.cc

nrsdmn_SOURCES = nrsdmn.cc

tcpdmn_SOURCES = tcpdmn.cc

testdmn_SOURCES = testdmn.cc

parcook_SOURCES = parcook.cc funtable.cc funtable.h

mbusdmn_SOURCES = mbusdmn.cc

wlkdmn_SOURCES = wlkdmn.cc
wlkdmn_LDADD = $(LDADD) $(BOOST_LDFLAGS) $(BOOST_DATE_TIME_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_REGEX_LIB)

mbdmn_SOURCES = mbdmn.cc
mbdmn_LDADD = $(LDADD) @EVENT_LIBS@ @XML_LIBS@ @PTHREAD_CFLAGS@ $(BOOST_DATE_TIME_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)

borutadmn_SOURCES = borutadmn.cc borutadmn.h boruta_zet.cc boruta_modbus.cc boruta_fp210.cc boruta_wmtp.cc boruta_lumel.cc boruta_fc.cc  cfgdealer_handler.cpp
borutadmn_LDADD = $(LDADD) @EVENT_LIBS@ @XML_LIBS@ @PTHREAD_CFLAGS@ $(BOOST_DATE_TIME_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)

borutadmn_z_SOURCES = borutadmn_z.cc borutadmn_z.h  boruta_modbus_z.cc boruta_fc_z.cc
borutadmn_z_LDADD = $(LDADD) @EVENT_LIBS@ @XML_LIBS@ @PTHREAD_CFLAGS@ $(BOOST_DATE_TIME_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)

sampledmn_SOURCES = sampledmn.cc $(basedmn_SOURCES)

sbusdmn_SOURCES = sbusdmn.cc cfgdealer_handler.cpp

ddedmn_SOURCES = ddedmn.cc
ddedmn_LDADD = $(LDADD) $(BOOST_LDFLAGS) $(XMLRPC_EPI_LIBS) $(CURL_LIBS) -lstdc++

iecdmn_SOURCES = iecdmn.cc
iecdmn_LDADD = $(LDADD)

prodmn_SOURCES = prodmn.cc
prodmn_LDADD = $(LDADD) @EVENT_LIBS@ @XML_LIBS@ @PTHREAD_CFLAGS@ $(BOOST_DATE_TIME_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)

pythondmn_SOURCES = pythondmn.cc cfgdealer_handler.cpp
pythondmn_LDADD = -l$(BOOST_PYTHON_LIB) @PTHREAD_CFLAGS@ $(BOOST_DATE_TIME_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB) $(BOOST_PYTHON_MODULE) $(PYTHON_LDFLAGS) $(LDADD)

snap7dmn_SOURCES = snap7dmn.cc $(basedmn_SOURCES)
snap7dmn_LDADD = $(LDADD) -lsnap7

defdmn_SOURCES = defdmn.h defdmn.cc defdmn.tpp funtable.cc funtable.h
defdmn_LDADD = $(LDADD)
