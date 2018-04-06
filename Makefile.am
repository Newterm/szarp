# Makefile.am for SZARP

# Pawe³ Pa³ucha <pawel@praterm.pl>
#
# $Id: Makefile.am 6583 2009-04-03 15:13:43Z pawel $

SUBDIRS = libSzarp libSzarp2 python sender parcook cfgdealer isl meaner3 iks extern wx utils unit_tests www resources script qt4
# SUBDIRS = @SUBDIRS@

# ACLOCAL_AMFLAGS = -I m4
# MAINTAINERCLEANFILES = Makefile.in configure aclocal.m4 config.h.in \
#                        depcomp install-sh missing mkinstalldirs ylwrap \
#                        config.guess config.sub stamp-h.in py-compile 

# Traditional SZARP target
cleanconfig: maintainer-clean
	debian/rules clean
	rm -rf doxy

# Create documentation
doc documentation:
	${MAKE} -C resources/documentation/new doc

# Make version info
version: 
	@install -d $(DESTDIR)@prefix@/resources
	@echo "Creating version info..."
	@ @srcdir@/script/version-info @srcdir@/debian/changelog > $(DESTDIR)@prefix@/resources/version.info

install_scripts:
	${MAKE} -C script install_scripts

logdir:
	$(INSTALL) -d $(DESTDIR)@prefix@/logs

install-data-hook: version logdir

doxy:
	cd @srcdir@ && doxygen

winsetup:
	${MAKE} -C wx winsetup

EXTRA_DIST = \
	     Doxyfile

.PHONY: doxy version
