
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 4671 2007-11-13 13:28:15Z kg $

SUBDIRS = 


PACKAGED_SCRIPT = \
	@srcdir@/insum.sh \
	@srcdir@/install_vim \
	@srcdir@/sshkey \
	@srcdir@/mobile/ipk2mobile.pl \
	@srcdir@/mobile/switch_controller.pl \
	@srcdir@/config_deb \
	@srcdir@/samson2/convert_names \
	@srcdir@/samson2/get_sam \
	@srcdir@/samson2/process_csv \
	@srcdir@/update_debs 

EXTRA_DIST = \
	$(PACKAGED_SCRIPT) \
	@srcdir@/debian_install \
	@srcdir@/sgml2hlpctrl.pl \
	@srcdir@/windist.sh


if DEB_BUILD
bin_SCRIPTS = $(PACKAGED_SCRIPT)

install-data-local:
	$(INSTALL) -d $(DESTDIR)/var/szarp
	$(INSTALL) -d $(DESTDIR)/var/log/szarp/
	$(INSTALL) -d $(DESTDIR)/usr/bin
	$(INSTALL) -d $(DESTDIR)/etc/init.d
endif
