
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6199 2008-11-20 08:28:37Z reksio $

SUBDIRS = 

SOURCE_DIR=@srcdir@
INCLUDE_DIR=$(SOURCE_DIR)/include

LDADD=
INCLUDES=-I$(INCLUDE_DIR) @BOOST_CPPFLAGS@ @XML_CFLAGS@


LEX=@LEX@

noinst_LIBRARIES = libSzarp.a

libparnt.yy.cc: libparnt.lex $(INCLUDE_DIR)/libpar_int.h $(INCLUDE_DIR)/libparnt.parse.h
	$(LEX) -o$@ $<

libSzarp_a_SOURCES = \
                      libparnt.yy.cc \
                      libparnt.cc \
                      libparnt.parse.cc \
                      liblog.cc \
                      date.cc \
                      msgerror.cc \
                      msgtools.cc \
                      daemon.cc \
                      execute.cc \
                      help.cc \
                      passwd.cc \
		      mbrtu.cc \
		      conversion.cc \
		      ipctools.cc \
		      mingw32_missing.cc
                      
noinst_HEADERS = \
                 $(INCLUDE_DIR)/libpar.h \
                 $(INCLUDE_DIR)/libpar_int.h \
                 $(INCLUDE_DIR)/libparnt.parse.h \
                 $(INCLUDE_DIR)/liblog.h \
                 $(INCLUDE_DIR)/date.h \
                 $(INCLUDE_DIR)/libXs.h \
                 $(INCLUDE_DIR)/help.h \
                 $(INCLUDE_DIR)/msgerror.h \
                 $(INCLUDE_DIR)/msgtools.h \
                 $(INCLUDE_DIR)/daemon.h \
                 $(INCLUDE_DIR)/execute.h \
                 $(INCLUDE_DIR)/passwd.h \
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
                 libparnt.yy.c 
                      

