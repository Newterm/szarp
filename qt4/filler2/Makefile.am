# Makefile.am for Filler 2 program.

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Program to create
filler2_LOCALES = @srcdir@/locales/filler2_pl.qm
filler2_LOCALES_SRC = locales/filler2_pl.ts

filler2_script = @srcdir@/filler2
szb2filler_script = @srcdir@/szb2filler

filler2dir = @prefix@/lib/python/filler2

filler2_PYTHON = \
		@srcdir@/ui/__init__.py \
		@srcdir@/ui/AboutDialog.py \
		@srcdir@/ui/DatetimeDialog.py \
		@srcdir@/ui/HistoryDialog.py \
		@srcdir@/ui/ValueDialogs.py \
		@srcdir@/ui/Filler2.py \
		@srcdir@/ipk/__init__.py \
		@srcdir@/ipk/ipkparser.py \
		@srcdir@/filler2-launcher.py

filler2_UI = ui/Filler2.ui ui/DatetimeDialog.ui ui/AboutDialog.ui ui/HistoryDialog.ui

filler2_GENPYTHON = ui/Filler2.py ui/DatetimeDialog.py ui/AboutDialog.py ui/HistoryDialog.py

filler2_PYC = ui/*.pyc ipk/*.pyc

filler2_PROJECT = filler2.pro

CLEANFILES = $(filler2_GENPYTHON) $(filler2_LOCALES) $(filler2_PYC)

EXTRA_DIST = $(filler2_LOCALES_SRC) $(filler2_LOCALES)

# Rule to create qm files
$(filler2_LOCALES): $(filler2_LOCALES_SRC)
	lrelease-qt4 -nounfinished $? -qm $@

# Rule to compile Qt4 ui files
%.py: %.ui
	pyuic4 $< > $@

all: $(filler2_GENPYTHON) $(filler2_LOCALES)

# Install locales and launcher
install-data-local: $(filler2_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/qt4
	$(INSTALL) $(filler2_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/qt4
	$(INSTALL) $(filler2_script) $(DESTDIR)@prefix@/bin
	$(INSTALL) $(szb2filler_script) $(DESTDIR)@prefix@/bin

update-ts:
	pylupdate4 $(filler2_PROJECT)

