
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6199 2008-11-20 08:28:37Z reksio $

SUBDIRS = 

SOURCE_DIR=@srcdir@
INCLUDE_DIR=$(SOURCE_DIR)/include

LDADD=
AM_CPPFLAGS = -I$(INCLUDE_DIR) @BOOST_CPPFLAGS@ @XML_CFLAGS@


LEX=@LEX@

noinst_LTLIBRARIES = libSzarp.la

EXTRA_PROGRAMS = async_log_test

async_log_test_SOURCES = async_log_test.c
async_log_test_LDADD = $(LDADD) -levent libSzarp.la 


libparnt.yy.cc: libparnt.lex $(INCLUDE_DIR)/libpar_int.h $(INCLUDE_DIR)/libparnt.parse.h
	$(LEX) -o$@ $<

libSzarp_la_SOURCES = \
                      libparnt.yy.cc \
                      libparnt.cc \
                      libparnt.parse.cc \
                      liblog.cc \
                      date.cc \
                      msgerror.cc \
                      msgtools.cc \
                      daemon.cc \
                      execute.cc \
		      mbrtu.cc \
		      conversion.cc \
		      ipctools.cc \
		      async_syslog.c \
		      liblog_impl_classic.cc \
		      liblog_impl_syslog.cc \
		      liblog_impl_asyslog.cc \
		      mingw32_missing.cc
                      
noinst_HEADERS = \
                 $(INCLUDE_DIR)/libpar.h \
                 $(INCLUDE_DIR)/libpar_int.h \
                 $(INCLUDE_DIR)/libparnt.parse.h \
                 $(INCLUDE_DIR)/liblog.h \
                 $(INCLUDE_DIR)/date.h \
                 $(INCLUDE_DIR)/msgerror.h \
                 $(INCLUDE_DIR)/msgtools.h \
                 $(INCLUDE_DIR)/daemon.h \
                 $(INCLUDE_DIR)/execute.h \
		 $(INCLUDE_DIR)/mbrtu.h \
		 $(INCLUDE_DIR)/modbus.h \
		 $(INCLUDE_DIR)/ipctools.h \
		 $(INCLUDE_DIR)/conversion.h \
		 $(INCLUDE_DIR)/async_syslog.h \
		 $(INCLUDE_DIR)/liblog_impl_classic.h \
		 $(INCLUDE_DIR)/liblog_impl_syslog.h \
		 $(INCLUDE_DIR)/liblog_impl_asyslog.h \
		 $(INCLUDE_DIR)/mingw32_missing.h 
 
EXTRA_DIST = \
             libparnt.lex 

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = \
                 Makefile.in \
                 libparnt.yy.c 
                      

