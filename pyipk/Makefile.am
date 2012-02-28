
# Jakub Kotur <qba@newterm.pl>
#
# $Id: Makefile.am 6332 2012-02-23 15:15:14

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

# fake pyipk files, to build lenny. This must be updated every time
# while something is added to debian/packs.files in pyipk section
if DEB_BUILD
	@touch $(DESTDIR)///opt/szarp/bin/ipkcmd
	@touch $(DESTDIR)///opt/szarp/bin/ipkqt
	@mkdir -p $(DESTDIR)///opt/szarp/lib/plugins
	@mkdir -p $(DESTDIR)///opt/szarp/lib/python${PYTHON_VERSION}/dist-packages/libipk
	@mkdir -p $(DESTDIR)///opt/szarp/lib/python${PYTHON_VERSION}/dist-packages/qtipk
endif
	@echo 'Python <= 2.5 not supported'

endif

