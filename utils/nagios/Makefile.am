# SZARP SCADA
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6836 2009-06-12 22:10:00Z kg $

SUBDIRS = 

# Source directory
SOURCE_DIR=@srcdir@

# Additional files for deleting on 'make maintainer-clean'

if !MINGW32_OPT
# Python modules - also .pyc and .pyo versions are installed
nagiosdir = @prefix@/lib/
nagios_PYTHON = szarpnagios.py

bin_SCRIPTS = sn-fam.py update-nagios.py

install-data-local:
	$(INSTALL) -d $(DESTDIR)/etc/nagios3/conf.d
	$(INSTALL) -m 0644 szarp-templates.cfg szarp-admins.cfg $(DESTDIR)/etc/nagios3/conf.d
	$(INSTALL) -d $(DESTDIR)/etc/szarp
	$(INSTALL) -m 0644 szarp-nagios.cfg $(DESTDIR)/etc/szarp
	$(INSTALL) -d $(DESTDIR)/usr/share/nagios3/htdocs
	$(INSTALL) -m 0644 *.html $(DESTDIR)/usr/share/nagios3/htdocs

endif
