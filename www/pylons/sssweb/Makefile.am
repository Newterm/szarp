
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6332 2009-01-20 17:27:14Z reksio $

SUBDIRS = 

MAINTAINERCLEANFILES = Makefile.in

LOCALES = @srcdir@/sssweb/i18n/pl/LC_MESSAGES/sssweb.mo
LOCALES_SRC = sssweb/i18n/pl/LC_MESSAGES/sssweb.po
LOCALES_TMP = sssweb/i18n/sssweb.pot

CLEANFILES = $(LOCALES) $(LOCALES_TMP)

EXTRA_DIST = $(LOCALES_SRC) $(LOCALES)

PACKAGE=szarp-sssweb
EGG=sssweb
PYVER=$(shell pyversions -vr)


# Rule to create mo files
$(LOCALES): $(LOCALES_SRC)
	python setup.py compile_catalog

all: $(LOCALES)

sswebdir=/usr/lib/python$(PYTHON_VERSION)/site-packages/sssweb
ssweb_PYTHON=$(shell find build -name '*.py')

clean:
	rm -rf build
	rm -f build-*
	python @srcdir@/setup.py clean
	find . -name *\.py[co] -exec rm -f {} \;
	rm -rf data/sessions/*
	rm -rf data/templates/*
	rm -rf sssweb.egg-info

install-data-local:
	python${PYVER} setup.py install --root $(DESTDIR) --single-version-externally-managed
	$(INSTALL) -d $(DESTDIR)/var/cache/pylons-sssweb
	$(INSTALL) -d $(DESTDIR)/etc/szarp
	$(INSTALL) -m 0644 production.ini $(DESTDIR)/etc/szarp/ssweb.ini
	$(INSTALL) -m 0644 ssweb-apache.conf $(DESTDIR)/etc/szarp/
	@if [ -e $(DESTDIR)/usr/local/lib/python$(PYTHON_VERSION)/dist-packages/sssweb ]; then \
			$(INSTALL) -d $(DESTDIR)/usr/lib/python$(PYTHON_VERSION)/site-packages; \
			cp -a $(DESTDIR)/usr/local/lib/python$(PYTHON_VERSION)/dist-packages/*  $(DESTDIR)/usr/lib/python$(PYTHON_VERSION)/site-packages; \
	fi
	cp -a $(DESTDIR)/usr/lib/python$(PYTHON_VERSION)/site-packages/sssweb-*.egg-info  $(DESTDIR)/usr/lib/python$(PYTHON_VERSION)/site-packages/sssweb.egg-info 
update-po:
	python setup.py extract_messages
	python setup.py update_catalog
	rm -f $(LOCALES_TMP)
