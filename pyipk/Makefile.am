
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6332 2009-01-20 17:27:14Z reksio $

SUBDIRS = 

MAINTAINERCLEANFILES = Makefile.in

PACKAGE=pyipk
EGG=pyipk

all:
if PYTHON_26
	python setup.py build
else
	@echo 'Python <= 2.5 not supported'
endif

noinst_PYTHON=`find build -name '*.py'`

clean:
if PYTHON_26
	rm -rf build
	rm -f build-*
	python @srcdir@/setup.py clean
	find . -name *\.py[co] -exec rm -f {} \;
else
	@echo 'Python <= 2.5 not supported'
endif


install:
if PYTHON_26

if DEB_BUILD
	python setup.py install --root $(DESTDIR) --prefix /opt/szarp/ --install-layout=deb
else
	python setup.py install --prefix /opt/szarp/ 
endif

else
	@echo 'Python <= 2.5 not supported'
endif

