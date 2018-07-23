
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6199 2008-11-20 08:28:37Z reksio $

SUBDIRS = 

LDADD=../../libSzarp/libSzarp.la @SSL_LIBS@ @PAM_LIBS@ @XML_LIBS@
AM_CPPFLAGS = \
	-I@srcdir@/../../include		\
	-I@srcdir@/../../libSzarp/include	\
	-I@srcdir@/../../libSzarp2/include	\
	@XML_CFLAGS@ -DPREFIX=\"@prefix@\"
	
noinst_LTLIBRARIES = libserver.la

libserver_la_SOURCES = \
	access.cc \
	access.h \
	auth_server.cc \
	auth_server.h \
	base64.cc \
	base64.h \
	config_load.cc \
	config_load.h \
	pam.cc \
	pam.h \
	server.cc \
	server.h \
	uri.h \
	ssl_server.cc \
	ssl_server.h \
	utf8.cc \
	utf8.h 

# Additional files for deleting on 'make maintainer-clean'
                 Makefile.in 
                      

