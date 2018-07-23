
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6332 2009-01-20 17:27:14Z reksio $

SUBDIRS = 


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

clean:
	rm -rf build
	rm -f build-*
	python @srcdir@/setup.py clean
	find . -name *\.py[co] -exec rm -f {} \;
	rm -rf data/sessions/*
	rm -rf data/templates/*
	rm -rf sssweb.egg-info

install-data-local:
	$(INSTALL) -d $(DESTDIR)/etc/szarp
	$(INSTALL) -d $(DESTDIR)/var/cache/pylons-sssweb
	$(INSTALL) -m 0644 production.ini $(DESTDIR)/etc/szarp/ssweb.ini
	$(INSTALL) -m 0644 ssweb-apache.conf $(DESTDIR)/etc/szarp/
if DEB_BUILD
	python${PYVER} setup.py install --root $(DESTDIR) --prefix /usr --single-version-externally-managed
else
	python${PYVER} setup.py install 
endif

update-po:
	python setup.py extract_messages
	python setup.py update_catalog
	rm -f $(LOCALES_TMP)
