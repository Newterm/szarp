# Makefile.am for Filler 2 program.

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Program to create
filler2_LOCALES = @srcdir@/pl/filler2_pl.qm
filler2_LOCALES_SRC = pl/filler2_pl.ts

filler2_script = @srcdir@/filler2

filler2dir = @prefix@/lib/python

filler2_PYTHON = \
		@srcdir@/AboutDialog.py \
		@srcdir@/DatetimeDialog.py \
		@srcdir@/HistoryDialog.py \
		@srcdir@/ipkparser.py \
		@srcdir@/filler2-launcher.py \
		@srcdir@/filler2.py

filler2_UI = filler2.ui DatetimeDialog.ui AboutDialog.ui HistoryDialog.ui

filler2_GENPYTHON = filler2.py DatetimeDialog.py AboutDialog.py HistoryDialog.py

filler2_PROJECT = filler2.pro

CLEANFILES = $(filler2_GENPYTHON) $(filler2_LOCALES)

EXTRA_DIST = $(filler2_LOCALES_SRC) $(filler2_LOCALES)

# Rule to create qm files
$(filler2_LOCALES): $(filler2_LOCALES_SRC)
	lrelease-qt4 -nounfinished $? -qm $@

# Rule to compile Qt4 ui files
%.py: %.ui
	pyuic4 $< > $@

all: $(filler2_GENPYTHON) $(filler2_LOCALES)

# Install locales and luncher
install-data-local: $(filler2_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/qt4
	$(INSTALL) $(filler2_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/qt4
	$(INSTALL) $(filler2_script) $(DESTDIR)@prefix@/bin

update-ts:
	pylupdate4 $(filler2_PROJECT)

