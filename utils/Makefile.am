# SZARP SCADA
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6846 2009-06-23 15:40:30Z pawel $

SUBDIRS = nagios

# Source directory
SOURCE_DIR=@srcdir@

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Compilations flags
AM_CPPFLAGS = \
		@XML_CFLAGS@ \
		@LUA_CFLAGS@ \
		@SSL_CFLAGS@ \
		@PTHREAD_CFLAGS@ \
		-I$(SOURCE_DIR)/../libSzarp/include \
		-I$(SOURCE_DIR)/../libSzarp2/include \
		-I$(SOURCE_DIR)/../include \
		-DPREFIX=\"@prefix@\" \
		-DINSTALL_PREFIX='"/opt/szarp"'

LIBSZARP=../libSzarp/libSzarp.a 
LIBSZARP2=../libSzarp2/libSzarp2.a ../libSzarp/libSzarp.a @BOOST_DATE_TIME_LIB@  @BOOST_THREAD_LIB@

LDADD = @XML_LIBS@ $(LIBSZARP) @LUA_LIBS@

# Programs to create
#if MINGW32_OPT
#bin_PROGRAMS=sendhexw @XGREP2@
#else
if !MINGW32_OPT
bin_PROGRAMS = \
		@AGREGATOR@ \
		@ANALIZA2IPK@ \
		crypt \
		@IPK2SZARP@ \
		@IPKREP@ \
		@IPKMAX@ \
		lpparse \
		@PRECCONV@ \
		sproxy \
		vrsh \
		vrsh_d \
		analiza_create \
		@XGREP2@  \
		@STAMPD@
	      
               
EXTRA_PROGRAMS = \
		analiza2ipk \
		agregator \
		ipk2szarp \
		ipkrep \
		ipkmax \
		precconv \
		analiza_create \
		xgrep2 \
		stampd


EXTRA_DIST = \
              $(bin_SCRIPTS) \
              $(noinst_SCRIPTS)

noinst_SCRIPTS = \
              isdn-up-in.sh \
              isdn-up-out.sh 
                 
bin_SCRIPTS = \
	      conf-get.sh \
	      conf-import.py \
	      conf-put.sh \
	      fdcp \
	      get_weather.sh \
	      i2smo \
              ipcclean \
              isdn-down.sh \
              isdn_pooler.sh \
              modem_pooler.sh \
              raprec \
              rsync_wrap.pl \
	      sortlog \
              ssh_tunel.sh \
              szarp_scripter \
	      szarpinfo \
              szppp-off \
              szppp-on \
              szredialer \
	      szrsync \
	      szbpush \
              remarks_server/remarks_server.py

raporterdir = @prefix@/bin/
raporter_PYTHON = raporter.py

endif

agregator_SOURCES = agregator.cc
agregator_LDADD = $(LIBSZARP2) $(LIBSZARP) @PTHREAD_CFLAGS@ @XML_LIBS@ @BOOST_LDFLAGS@ @BOOST_FILESYSTEM_LIB@ @SSL_LIBS@

ipk2szarp_SOURCES = ipk2szarp.cc
ipk2szarp_LDADD = @PTHREAD_CFLAGS@ @XML_LIBS@ $(LIBSZARP2) $(LIBSZARP2) @BOOST_FILESYSTEM_LIB@

lpparse_SOURCES = lpparse.cc
lpparse_LDADD = @PTHREAD_CFLAGS@ @XML_LIBS@ $(LIBSZARP)

ipkrep_SOURCES = ipkrep.cc
ipkrep_LDADD = @PTHREAD_CFLAGS@ @XML_LIBS@ $(LIBSZARP2) @BOOST_FILESYSTEM_LIB@

precconv_SOURCES = precconv.cc
precconv_LDADD = $(LIBSZARP2) $(LIBSZARP) @PTHREAD_CFLAGS@ @XML_LIBS@ @BOOST_FILESYSTEM_LIB@

analiza2ipk_SOURCES = analiza2ipk.cc
analiza2ipk_LDADD = @PTHREAD_CFLAGS@ @XML_LIBS@ $(LIBSZARP2) $(LIBSZARP) @BOOST_FILESYSTEM_LIB@

ipkmax_SOURCES = ipkmax.cc
ipkmax_LDADD = @PTHREAD_CFLAGS@ @XML_LIBS@ $(LIBSZARP) $(LIBSZARP2) @BOOST_FILESYSTEM_LIB@

crypt_LDADD = -lcrypt

vrsh_SOURCES = vrsh.c
vrsh_LDADD = $(LIBSZARP2) $(LIBSZARP)
vrsh_d_SOURCES = vrsh.c
vrsh_d_CPPFLAGS = -DDEBUG $(AM_CPPFLAGS)
vrsh_d_LDADD = $(LIBSZARP2) $(LIBSZARP)

analiza_create_SOURCES = analiza_create.c
analiza_create_CPPFLAGS = 
analiza_create_LDADD = 

xgrep2_SOURCES = xgrep2.cc
xgrep2_LDADD = $(LIBSZARP2) $(LIBSZARP)

#unifier_SOURCES = unifier.cc
#unifier_LDADD = @XML_LIBS@ $(LIBSZARP2) $(LIBSZARP)

stampd_SOURCES = stampd.c
stampd_LDADD = @EVENT_LIBS@ $(LIBSZARP)

if DEB_BUILD
install-data-local: xgrep2
	$(INSTALL) -d $(DESTDIR)/var/www/wpn/wpn-cgi
	$(INSTALL) xgrep2 $(DESTDIR)/var/www/wpn/wpn-cgi/xgrep2.cgi
	chmod +x $(DESTDIR)/var/www/wpn/wpn-cgi/xgrep2.cgi
endif

