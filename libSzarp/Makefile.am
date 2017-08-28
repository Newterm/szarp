
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

libparnt.yy.cc: libparnt.lex $(INCLUDE_DIR)/libpar_int.h $(INCLUDE_DIR)/libparnt.parse.h
	$(LEX) -o$@ $<

libSzarp_la_SOURCES = \
                      libparnt.yy.cc \
                      libparnt.cc \
                      libparnt.parse.cc \
                      liblog.cpp \
                      argsmgr.cpp \
                      cmdlineparser.cpp \
                      loghandlers.cpp \
                      date.cc \
                      msgerror.cc \
                      msgtools.cc \
                      daemon.cc \
                      execute.cc \
		      mbrtu.cc \
		      conversion.cc \
		      ipctools.cc \
		      mingw32_missing.cc
                      
noinst_HEADERS = \
                 $(INCLUDE_DIR)/libpar.h \
                 $(INCLUDE_DIR)/libpar_int.h \
                 $(INCLUDE_DIR)/libparnt.parse.h \
                 $(INCLUDE_DIR)/liblog.h \
                 $(INCLUDE_DIR)/argsmgr.h \
                 $(INCLUDE_DIR)/cmdlineparser.h \
                 $(INCLUDE_DIR)/loghandlers.h \
                 $(INCLUDE_DIR)/date.h \
                 $(INCLUDE_DIR)/msgerror.h \
                 $(INCLUDE_DIR)/msgtools.h \
                 $(INCLUDE_DIR)/daemon.h \
                 $(INCLUDE_DIR)/execute.h \
		 $(INCLUDE_DIR)/mbrtu.h \
		 $(INCLUDE_DIR)/modbus.h \
		 $(INCLUDE_DIR)/ipctools.h \
		 $(INCLUDE_DIR)/conversion.h \
		 $(INCLUDE_DIR)/mingw32_missing.h 
 
EXTRA_DIST = \
             libparnt.lex 

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = \
                 Makefile.in \
                 libparnt.yy.cc
                      

