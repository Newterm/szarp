
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6199 2008-11-20 08:28:37Z reksio $

SUBDIRS = 

INCLUDE_DIR=include

LDADD=
AM_CPPFLAGS = -I./$(INCLUDE_DIR) @BOOST_CPPFLAGS@ @XML_CFLAGS@ -DYYLMAX=32768 -DSD_JOURNAL_SUPPRESS_LOCATION


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
                      msgerror.cc \
                      msgtools.cc \
                      daemon.cc \
                      execute.cc \
		      mbrtu.cc \
		      conversion.cc \
		      ipctools.cc \
		      mingw32_missing.cc
                      
if MINGW32_OPT
libSzarp_la_LIBADD = @BOOST_PROGRAM_OPTIONS_LIB@
else
libSzarp_la_LIBADD = @BOOST_PROGRAM_OPTIONS_LIB@ -lsystemd
endif

 
EXTRA_DIST =
             libparnt.lex 

# Additional files for deleting on 'make maintainer-clean'
                 Makefile.in
