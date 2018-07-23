#
# (C) Pawe³ Pa³ucha 2002
# paramd (ParamDaemon) Makefile
#
# $Id: Makefile.am 6199 2008-11-20 08:28:37Z reksio $

SUBDIRS =

AM_CPPFLAGS = -I@srcdir@/../../include \
	   -I@srcdir@/../../libSzarp/include   \
	   -I@srcdir@/../../libSzarp2/include   \
	   -I@srcdir@/../server @XML_CFLAGS@ @PTHREAD_CFLAGS@ @LUA_CFLAGS@ \
	   -DPREFIX=\"@prefix@\"

LDADD=../server/libserver.la \
      ../../libSzarp/libSzarp.la \
      ../../libSzarp2/libSzarp2.la  \
      @SSL_LIBS@ @PAM_LIBS@ @XML_LIBS@ @PTHREAD_CFLAGS@ @PTHREAD_LIBS@ @LUA_LIBS@ @BOOST_SYSTEM_LIB@ @BOOST_DATE_TIME_LIB@ @BOOST_THREAD_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_LOCALE_LIB@

SHORTHOST=$(shell hostname -s)

SSLCONFIG=@srcdir@/config.ssl
KEYFILE=server.pem
CERTFILE=server.ca
SSLINSTALL_PATH=@prefix@/resources
PAMFILE=@srcdir@/paramd.pam

bin_PROGRAMS = paramd

paramd_SOURCES = \
	ipk2xml.cc \
	ipk2xml.h \
	param_tree.cc \
	param_tree.h \
	param_tree_dynamic_data.h \
	param_value_functor.cc \
	param_value_functor.h \
	tree_processor.h \
	tree_processor.cc \
	paramd.cc \
	ptt2xml.cc \
	ptt2xml.h  \
	svg_generator.h \
	svg_generator.cc \
	probes_types.h

EXTRA_DIST = \
	$(PAMFILE) \
	$(SSLCONFIG)

# This used to be something like that:
#
#@openssl req -x509 -newkey rsa:1024 -keyout $(KEYFILE) \
#-out $(CERTFILE) -days 365 -config $(SSLCONFIG).tmp
#
# But it doesn't work anymore.

ssl:
	@echo "Generating RSA private key and certificate..."
	@sed s/localhost/$(HOSTNAME)/ $(SSLCONFIG) > $(SSLCONFIG).tmp
	@openssl req -x509 -newkey rsa:1024 -keyout $(KEYFILE) \
	-out $(CERTFILE) -days 365 
	@rm -f $(SSLCONFIG).bak
	@echo "Installing certificate and key file in $(SSLINSTALL_PATH)"
	@$(INSTALL) $(CERTFILE) $(SSLINSTALL_PATH)/$(CERTFILE)
	@$(INSTALL) $(KEYFILE) $(SSLINSTALL_PATH)/$(KEYFILE)
	@echo "Installing PAM access rules"

install-data-hook: pam

pam:
	@if [ ! -e $(DESTDIR)/etc/pam.d ]; then \
		$(INSTALL) -d $(DESTDIR)/etc/pam.d; \
	fi
	@$(INSTALL) $(PAMFILE) $(DESTDIR)/etc/pam.d/paramd

# Additional files for deleting on 'make maintainer-clean'
                 Makefile.in 

