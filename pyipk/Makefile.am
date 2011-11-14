
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6332 2009-01-20 17:27:14Z reksio $

SUBDIRS = 

MAINTAINERCLEANFILES = Makefile.in

PACKAGE=pyipk
EGG=pyipk
PYVER=`pyversions -vr`

all:
	python${PYVER} setup.py build

noinst_PYTHON=`find build -name '*.py'`

clean:
	rm -rf build
	rm -f build-*
	python @srcdir@/setup.py clean
	find . -name *\.py[co] -exec rm -f {} \;

install:
if DEB_BUILD
	python${PYVER} setup.py install --root $(DESTDIR) --prefix /usr --install-layout=deb
else
	python${PYVER} setup.py install 
endif

