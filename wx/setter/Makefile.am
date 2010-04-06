
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 3622 2006-12-27 11:37:11Z maciek $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Program to create

setter_LOCALES = @srcdir@/setter.mo
setter_LOCALES_SRC = pl/setter.po
setter_LOCALES_TMP = pl/setter.pot

bin_SCRIPTS = setter

CLEANFILES = $(setter_LOCALES) 

EXTRA_DIST = $(setter_LOCALES_SRC) $(setter_LOCALES)

# Rule to create mo files
$(setter_LOCALES): $(setter_LOCALES_SRC)
	@MSGFMT@ -o $@ $<

all: $(setter_LOCALES)

# Install locales and luncher
install-data-local: $(setter_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(setter_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(setter_script) $(DESTDIR)@prefix@/bin

update-po:
	@XGETTEXT@ -o $(setter_LOCALES_TMP) -L Python --keyword=_  setter
	@MSGMERGE@ -U $(setter_LOCALES_SRC) $(setter_LOCALES_TMP)
	rm -f $(setter_LOCALES_TMP)
