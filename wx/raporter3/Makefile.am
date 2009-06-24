
#
# $Id: Makefile.am 6491 2009-03-18 23:25:18Z reksio $

# No subdirectories
SUBDIRS =

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in

# Compilations flags
INCLUDES = @WX_CXXFLAGS@ @XML_CFLAGS@ @CURL_CFLAGS@ @LUA_CFLAGS@ @BOOST_CPPFLAGS@ \
	-I@srcdir@/../../libSzarp/include \
	-I@srcdir@/../../libSzarp2/include \
        -I@srcdir@/../../include \
	-I@srcdir@/../common \
	-DPREFIX=\"@prefix@\"

LIBSZARP=../../libSzarp/libSzarp.a
LIBSZARP2=../../libSzarp2/libSzarp2.a 
LIBWXCOMMON=../common/libwxcommon.a

# Program to create

bin_PROGRAMS = raporter3

WIN_APP_NAME=Raporter3

ICONS=\
	bmp_grey.xpm \
	bmp_start.xpm \
	bmp_green1.xpm \
	bmp_green2.xpm \
	bmp_yellow1.xpm \
	bmp_yellow2.xpm \
	bmp_red1.xpm \
	bmp_red2.xpm

raporter3_SOURCES = \
	rap.cc raporter.cc raporter.h \
	raporterbuf.cc raporterbuf.h \
	raporterperiod.cc raporterperiod.h \
	raporteredit.cc raporteredit.h \
	filedump.cc raporteredit.h \
	$(ICONS)

raporter3_DEPENDENCIES = $(LIBWXCOMMON) $(LIBSZARP) $(LIBSZARP2)
raporter3_LDADD = $(LIBWXCOMMON) $(LIBSZARP2) $(LIBSZARP) \
	@XML_LIBS@ @WX_LIBS@ @CURL_LIBS@ @LUA_LIBS@ @BOOST_LDFLAGS@ @BOOST_DATE_TIME_LIB@ @BOOST_THREAD_LIB@ @BOOST_FILESYSTEM_LIB@

raporter3_LOCALES = @srcdir@/raporter3.mo
raporter3_LOCALES_SRC = pl/raporter3.po
raporter3_LOCALES_TMP = pl/raporter3.pot

#.xrc.cc:
#	wxrc -v -c $< -o $@

CLEANFILES = $(raporter3_LOCALES) ${WIN_APP_NAME}Setup.exe

EXTRA_DIST = $(raporter3_LOCALES_SRC) $(raporter3_LOCALES)

# Rule to create mo files
$(raporter3_LOCALES): $(raporter3_LOCALES_SRC)
	@MSGFMT@ -o $@ $<

all: raporter3$(EXEEXT) $(raporter3_LOCALES)

# Install locales
install-data-local: $(raporter3_LOCALES)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES
	$(INSTALL) $(raporter3_LOCALES) \
                $(DESTDIR)@prefix@/resources/locales/pl_PL/LC_MESSAGES

update-po:
	@XGETTEXT@ -o $(raporter3_LOCALES_TMP) -L C++ --keyword=_ $(raporter3_SOURCES)
	@MSGMERGE@ -U $(raporter3_LOCALES_SRC) $(raporter3_LOCALES_TMP)
	rm -f $(raporter3_LOCALES_TMP)

# Target for creating Windows version zip-package.
windist: raporter3$(EXEEXT) $(raporter3_LOCALES)
	@srcdir@/../../script/windist.sh raporter3 @srcdir@ 
	
winsetup: ${WIN_APP_NAME}Setup.exe

${WIN_APP_NAME}Setup.exe: raporter3$(EXEEXT) $(raporter3_LOCALES)
	@srcdir@/../../script/nsis/winsetup.sh ${WIN_APP_NAME} raporter3$(EXEEXT) \
		-d resources/documentation/new/raporter/html \
		@srcdir@/../../resources/documentation/new/raporter/html/*.h* \
		-d resources/documentation/new/raporter/html/pics \
		@srcdir@/../../resources/documentation/new/raporter/html/*.* \
		-d resources/locales/pl_PL/LC_MESSAGES \
		$(raporter3_LOCALES) \
		-d resources/wx/html/raporterabou.html \
		@srcdir@/../../resources/wx/html/raporterabout.html
	@echo Make sure docs are generated!!!
		

