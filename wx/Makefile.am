
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6741 2009-05-19 07:29:57Z reksio $

if MINGW32_OPT
INCLUDE_SUBDIRS = ekstraktor3 szau
else
INCLUDE_SUBDIRS = reporter4 ekstraktor3 confedit filler
endif

if HAVE_CURL
CURL_SUBDIRS = raporter3 kontroler3
if HAVE_WNCK
WNCK_SUBDIRS = viszio
endif # WNCK
endif # CURL


SUBDIRS = common draw3 $(INCLUDE_SUBDIRS) \
	wxhelp scc $(CURL_SUBDIRS) \
	isledit $(WNCK_SUBDIRS)

MAINTAINERCLEANFILES = Makefile.in
