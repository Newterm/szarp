# $Id: Makefile.am 901 2003-09-20 00:39:37Z pawel $

SUBDIRS=

MAINTAINERCLEANFILES = Makefile.in

FILE=icons.h

all: $(FILE)

$(FILE):
	@echo '#ifndef _ICON_ICON_' > $(FILE)
	@echo '#define _ICON_ICON_' >> $(FILE)
	@for i in `ls *.icon`; do \
         echo '#include "'$$i'"' >> $(FILE); done
	@echo '#endif' >> $(FILE)

CLEANFILES = $(FILE)

EXTRA_DIST = \
         draw32.icon \
         draw48.icon \
         ext48.icon \
         extr32.icon \
         extr48.icon \
         for48.icon \
         kon32.icon \
         kon48.icon \
         lab48.icon \
         rap32.icon \
         tdb48.icon \
         testdbase.icon 

