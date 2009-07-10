# Makefile.am for SZARP

# Pawe³ Pa³ucha <pawel@praterm.pl>
#
# $Id: Makefile.am 6583 2009-04-03 15:13:43Z pawel $

SUBDIRS = @SUBDIRS@

MAINTAINERCLEANFILES = Makefile.in configure aclocal.m4 config.h.in \
                       depcomp install-sh missing mkinstalldirs ylwrap \
                       config.guess config.sub stamp-h.in py-compile 

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
	@grep "\$$Id" @srcdir@/ChangeLog | cut '--delimiter= ' --fields=3,4,5 \
		> $(DESTDIR)@prefix@/resources/version.info

install_scripts:
	${MAKE} -C script install_scripts

logdir:
	$(INSTALL) -d $(DESTDIR)@prefix@/logs

if !DEB_BUILD
install-data-hook: version logdir sysinfo
else DEB_BUILD
install-data-hook: version logdir
endif

sysinfo:
	$(SHELL) script/sysinfo

doxy:
	cd @srcdir@ && doxygen

winsetup:
	${MAKE} -C wx winsetup

EXTRA_DIST = \
	     Doxyfile

