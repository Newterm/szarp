
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6741 2009-05-19 07:29:57Z reksio $

if MINGW32_OPT
INCLUDE_SUBDIRS = ekstraktor3 szau
else
INCLUDE_SUBDIRS = ekstraktor3 confedit koper filler
endif

if HAVE_CURL
CURL_SUBDIRS = raporter3 kontroler3
endif

SUBDIRS = common draw3 $(INCLUDE_SUBDIRS) \
	wxhelp scc $(CURL_SUBDIRS) sync \
	isledit szast

winsetup:
	@srcdir@/../script/nsis/winsetup.sh -c \
		-d bin \
		-f draw3.exe draw3/draw3.exe \
		-f scc.exe scc/scc.exe \
		-f ssc.exe sync/ssc.exe \
		-f szau.exe szau/szau.exe \
		-f ekstraktor3.exe ekstraktor3/ekstraktor3.exe \
		-f raporter3.exe raporter3/raporter3.exe \
		-f isledit.exe isledit/isledit.exe \
		-f wxhelp.exe wxhelp/wxhelp.exe \
		-f isledit.exe isledit/isledit.exe \
		-f szast.exe szast/szast.exe \
		-d resources \
		@srcdir@/../resources/dictionary.xml \
		-f isledit.inx @srcdir@/../wx/isledit/isledit.inx.win \
		-f szarp.cfg @srcdir@/../resources/SZARP_CFG_TEMPLATE \
		-f szarp_in.cfg @srcdir@/../resources/SZARP_IN_CFG_TEMPLATE \
		-d resources/dtd \
		@srcdir@/../resources/dtd/params-list.rng \
		-d resources/wx/icons \
		@srcdir@/../resources/wx/icons/*.xpm \
		@srcdir@/../resources/wx/icons/*.png \
		-d resources/wx/images \
		@srcdir@/../resources/wx/images/*.png \
		@srcdir@/../resources/wx/images/*.bmp \
		-d resources/locales/pl_PL/LC_MESSAGES \
		-f draw3.mo @srcdir@/../wx/draw3/pl/draw3.mo \
		-f ekstraktor3.mo @srcdir@/../wx/ekstraktor3/ekstraktor3.mo \
		-f raporter3.mo @srcdir@/../wx/raporter3/raporter3.mo \
		-f scc.mo @srcdir@/../wx/scc/scc.mo \
		-f ssc.mo @srcdir@/../wx/sync/ssc.mo \
		-f szau.mo @srcdir@/../wx/szau/szau.mo \
		-f szast.mo @srcdir@/../wx/szast/szast.mo \
		-f common.mo @srcdir@/../wx/common/pl/common.mo \
		-f wx.mo @srcdir@/../wx/common/pl/wx.mo \
		-f isledit.po @srcdir@/../wx/isledit/isledit.mo \
		-d resources/locales/fr_FR/LC_MESSAGES \
		-f draw3.mo @srcdir@/../wx/draw3/fr/draw3.mo \
		-f common.mo @srcdir@/../wx/common/fr/common.mo \
		-f wx.mo @srcdir@/../wx/common/fr/wx.mo \
		-d resources/wx/html/ \
		-f raporterabout.html @srcdir@/../resources/wx/html/raporterabout.html \
		-f ekstraktorabout.html @srcdir@/../resources/wx/html/ekstraktorabout.html \
		-f sccabout.html @srcdir@/../resources/wx/html/sccabout.html \
		-d resources/documentation/new/common \
		@srcdir@/../resources/documentation/new/common/*.png \
		-d resources/documentation/new/ssc/html \
		@srcdir@/../resources/documentation/new/ssc/html/*.h* \
		-d resources/documentation/new/ssc/html \
		@srcdir@/../resources/documentation/new/ssc/html/*.map \
		-d resources/documentation/new/ssc/html/pics \
		@srcdir@/../resources/documentation/new/ssc/html/pics/*.* \
		-d resources/documentation/new/ekstraktor3/html \
		@srcdir@/../resources/documentation/new/ekstraktor3/html/*.h* \
		-d resources/documentation/new/ekstraktor3/html \
		@srcdir@/../resources/documentation/new/ekstraktor3/html/*.map \
		-d resources/documentation/new/ekstraktor3/html/pics \
		@srcdir@/../resources/documentation/new/ekstraktor3/html/pics/*.* \
		-d resources/documentation/new/draw3/html \
		@srcdir@/../resources/documentation/new/draw3/html/*.h* \
		-d resources/documentation/new/draw3/html \
		@srcdir@/../resources/documentation/new/draw3/html/*.map \
		-d resources/documentation/new/draw3/html/pics \
		@srcdir@/../resources/documentation/new/draw3/html/pics/*.* \
		-d resources/documentation/new/raporter/html \
		@srcdir@/../resources/documentation/new/raporter/html/*.h* \
		-d resources/documentation/new/raporter/html \
		@srcdir@/../resources/documentation/new/raporter/html/*.map \
		-d resources/documentation/new/raporter/html/pics \
		@srcdir@/../resources/documentation/new/raporter/html/pics/*.* \
		-d resources/documentation/new/howto/html \
		@srcdir@/../resources/documentation/new/howto/html/*.h* \
		-d resources/documentation/new/ssc/html \
		@srcdir@/../resources/documentation/new/howto/html/*.map \
		-d resources/documentation/new/eksploatacja/html \
		@srcdir@/../resources/documentation/new/eksploatacja/html/*.h* \
		-d resources/documentation/new/eksploatacja/html \
		@srcdir@/../resources/documentation/new/eksploatacja/html/*.map \
		-d resources/documentation/new/eksploatacja/html/pics \
		@srcdir@/../resources/documentation/new/eksploatacja/html/pics/*.* \
		-d resources/documentation/new/ipk/html \
		@srcdir@/../resources/documentation/new/ipk/html/*.h* \
		-d resources/documentation/new/ipk/html \
		@srcdir@/../resources/documentation/new/ipk/html/*.map \
		-d resources/documentation/new/ipk/html/pics \
		@srcdir@/../resources/documentation/new/ipk/html/pics/*.* \
		-d resources/icons \
		@srcdir@/../resources/wx/icons/*.ico \
		@srcdir@/../script/nsis/desktop.ini \
		-d resources/wx/anim \
		@srcdir@/../resources/wx/anim/*png \
		-d resources/documentation/new/scc/html \
		@srcdir@/../resources/documentation/new/scc/html/*.h* \
		-d resources/documentation/new/scc/html \
		@srcdir@/../resources/documentation/new/scc/html/*.map \
		-d resources/regulators \
		@srcdir@/../resources/regulators/*xml \
		-d resources/fonts \
		@srcdir@/../resources/fonts/*ttf

MAINTAINERCLEANFILES = Makefile.in
