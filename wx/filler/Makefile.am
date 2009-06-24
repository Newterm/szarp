
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 3622 2006-12-27 11:37:11Z maciek $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Program to create

filler_LOCALES = @srcdir@/filler.mo
filler_LOCALES_SRC = pl/filler.po
filler_LOCALES_TMP = pl/filler.pot

filler_script = @srcdir@/filler\
		@srcdir@/ipk2filler.py

fillerdir = @prefix@/lib/python
filler_PYTHON = \
		@srcdir@/cmdLineParser.py \
		@srcdir@/DateDialog.py \
		@srcdir@/fillerGUI.py \
		@srcdir@/filler.py 

CLEANFILES = $(filler_LOCALES) 

EXTRA_DIST = $(filler_LOCALES_SRC) $(filler_LOCALES)

# Rule to create mo files
$(filler_LOCALES): $(filler_LOCALES_SRC)
	@MSGFMT@ -o $@ $<

all: $(filler_LOCALES)

# Install locales and luncher
install-data-local: $(filler_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(filler_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(filler_script) $(DESTDIR)@prefix@/bin

update-po:
	@XGETTEXT@ -o $(filler_LOCALES_TMP) -L C++ --keyword=_  $(filler_SOURCES)
	@MSGMERGE@ -U $(filler_LOCALES_SRC) $(filler_LOCALES_TMP)
	rm -f $(filler_LOCALES_TMP)
