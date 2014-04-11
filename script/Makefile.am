
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 4671 2007-11-13 13:28:15Z kg $

SUBDIRS = 

MAINTAINERCLEANFILES = Makefile.in

PACKAGED_SCRIPT = \
	@srcdir@/get_sysinfo \
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
	@srcdir@/sysinfo.in \
	@srcdir@/szarp-server.cron \
	@srcdir@/windist.sh \
	@srcdir@/szarp-sss.cron 


if DEB_BUILD
bin_SCRIPTS = $(PACKAGED_SCRIPT) \
	      sysinfo

install-data-local:
	$(INSTALL) -d $(DESTDIR)/var/szarp
	$(INSTALL) -d $(DESTDIR)/var/log/szarp/
	$(INSTALL) -d $(DESTDIR)/etc/cron.d
	$(INSTALL) szarp-server.cron $(DESTDIR)/etc/cron.d/szarp-server
	$(INSTALL) szarp-sss.cron $(DESTDIR)/etc/cron.d/szarp-sss
	$(INSTALL) -d $(DESTDIR)/usr/bin
	$(INSTALL) -d $(DESTDIR)/etc/init.d
endif
