# Makefile.am for Filler 2 program.

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Program to create
filler2_LOCALES = @srcdir@/pl/filler2.mo
filler2_LOCALES_SRC = pl/filler2.po
filler2_LOCALES_TMP = pl/filler2.pot

filler2_script = @srcdir@/filler2

filler2dir = @prefix@/lib/python
filler2_PYTHON = \
		@srcdir@/filler2GUI.py \
		@srcdir@/filler2.py

filler2_SOURCES = filler2GUI.py \
				  filler2.py

CLEANFILES = $(filler2_LOCALES)

EXTRA_DIST = $(filler2_LOCALES_SRC) $(filler2_LOCALES)

# Rule to create mo files
$(filler2_LOCALES): $(filler2_LOCALES_SRC)
	@MSGFMT@ -o $@ $<

all: $(filler2_LOCALES)

# Install locales and luncher
install-data-local: $(filler2_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(filler2_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(filler2_script) $(DESTDIR)@prefix@/bin

update-po:
	@XGETTEXT@ -o $(filler2_LOCALES_TMP) -L C++ --keyword=_  $(filler2_SOURCES)
	@MSGMERGE@ -U $(filler2_LOCALES_SRC) $(filler2_LOCALES_TMP)
	rm -f $(filler2_LOCALES_TMP)

