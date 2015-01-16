
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6789 2009-05-29 12:03:33Z pawel $

SUBDIRS = 

LEX=@LEX@
YACC=@YACC@

SOURCE_DIR=@srcdir@
IPK_DIR=$(SOURCE_DIR)/ipk
SZBASE_DIR=$(SOURCE_DIR)/szbase
DMN_DIR=$(SOURCE_DIR)/linedmn
PARSERS_DIR=$(SOURCE_DIR)/parsers
INCLUDE_DIR=$(SOURCE_DIR)/include
TOOLS_DIR=$(SOURCE_DIR)/tools
XSLT_DIR=$(SOURCE_DIR)/xslt

LDADD = @LEXLIB@ @CURL_LIBS@ @XSLT_LIBS@ @XML_LIBS@ @LUA_LIBS@ 
	@ZIP_FLAGS@ \
	libSzarp2.la ../libSzarp/libSzarp.la 

AM_CPPFLAGS = @XML_CFLAGS@ @XSLT_CFLAGS@ @CURL_CFLAGS@ @LUA_CFLAGS@ \
	   @BOOST_CPPFLAGS@ @ZIP_CFLAGS@ \
	-I$(INCLUDE_DIR)				\
	-I$(INCLUDE_DIR)/szbase				\
	-I$(SOURCE_DIR)/../libSzarp/include		\
	-I$(SZBASE_DIR)					\
	-DPREFIX=\"@prefix@\"

noinst_LTLIBRARIES = libSzarp2.la libSzarpLuaParser.la

EXTRA_PROGRAMS = szbtest lua_parser_test xmlreader_test
szbtest_SOURCES = $(SZBASE_DIR)/szbtest.cc

lua_parser_test_INCLUDES = $(INCLUDES)
lua_parser_test_LDADD = $(LDADD) ../libSzarp/libSzarp.la libSzarp2.la
lua_parser_test_SOURCES = $(SZBASE_DIR)/lua_parser_test.cc

xmlreader_test_INCLUDES = $(INCLUDES)
xmlreader_test_LDADD = $(LDADD) ../libSzarp/libSzarp.la libSzarp2.la $(BOOST_LDFLAGS) $(BOOST_SYSTEM_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_REGEX_LIB) $(BOOST_DATE_TIME_LIB)  ../libSzarp/liblog.o
xmlreader_test_SOURCES = $(IPK_DIR)/xmlreader_test.cc


test: szbtest
	./szbtest
 
noinst_HEADERS = \
                      $(INCLUDE_DIR)/dmncfg.h \
                      $(INCLUDE_DIR)/httpcl.h \
                      $(INCLUDE_DIR)/szarpbase64.h \
                      $(INCLUDE_DIR)/ipchandler.h \
                      $(INCLUDE_DIR)/insort.h \
                      $(INCLUDE_DIR)/szarp_config.h \
		      $(INCLUDE_DIR)/tokens.h \
		      $(INCLUDE_DIR)/convuri.h \
                      $(INCLUDE_DIR)/ppset.h \
                      $(INCLUDE_DIR)/xmlutils.h

if MINGW32_OPT
INCLUDE_PARSERS =
else
INCLUDE_PARSERS = $(PARSERS_FILES)
endif

if MINGW32_OPT
BASECONN =
else
BASECONN = \
			$(DMN_DIR)/baseconn.cc \
			$(DMN_DIR)/bserialport.cc \
			$(DMN_DIR)/serialport.cc \
			$(DMN_DIR)/serialadapter.cc \
			$(DMN_DIR)/tcpconn.cc \
			$(DMN_DIR)/httpclient.cc \
			$(DMN_DIR)/atcconn.cc
endif


libSzarp2_la_SOURCES = \
			$(IPK_DIR)/tdevice.cc \
			$(IPK_DIR)/tdraw.cc \
			$(IPK_DIR)/tparam.cc \
			$(IPK_DIR)/tradio.cc \
			$(IPK_DIR)/traport.cc \
			$(IPK_DIR)/tsendparam.cc \
			$(IPK_DIR)/tszarpconfig.cc \
			$(IPK_DIR)/tunit.cc \
			$(IPK_DIR)/tvalue.cc \
			$(IPK_DIR)/tboiler.cc \
			$(IPK_DIR)/tseason.cc \
			$(IPK_DIR)/tanalysis.cc \
			$(IPK_DIR)/tainterval.cc \
			$(IPK_DIR)/ipkcontainer.cc \
			$(IPK_DIR)/tdictionary.cc \
			$(IPK_DIR)/ttreenode.cc \
			$(IPK_DIR)/dmncfg.cc \
			$(IPK_DIR)/tdefined.cc \
			$(IPK_DIR)/xmlwrapper.cc \
			$(IPK_DIR)/tdrawdefinable.cc \
			$(IPK_DIR)/tboilers.cc \
			$(IPK_DIR)/tscript.cc \
			$(DMN_DIR)/ipchandler.cc \
			$(SOURCE_DIR)/misc/convuri.cc \
			$(SOURCE_DIR)/misc/crontime.h \
			$(SOURCE_DIR)/misc/crontime.cc \
			$(SOURCE_DIR)/misc/http.cc \
			$(SOURCE_DIR)/misc/insort.cc \
			$(SOURCE_DIR)/misc/tokens.cc \
			$(SOURCE_DIR)/misc/xmlutils.cc \
			$(SOURCE_DIR)/misc/base64.cc \
			$(DMN_DIR)/mbus.cc \
			$(DMN_DIR)/ppset.cc \
			$(DMN_DIR)/daemonutils.cc \
			$(BASECONN) \
			$(SOURCE_DIR)/misc/xscale.cc \
			\
			$(SOURCE_DIR)/szbextr/extractor.cc \
			$(SOURCE_DIR)/szbextr/extr.h \
			\
			$(INCLUDE_PARSERS) \
			$(noinst_HEADERS) \
			\
			$(SZBASE_DIR)/szbbase.cc $(SZBASE_DIR)/szbbase.h \
			$(SZBASE_DIR)/szbbuf.cc $(SZBASE_DIR)/szbbuf.h \
			$(SZBASE_DIR)/szbdate.cc $(SZBASE_DIR)/szbdate.h \
			$(SZBASE_DIR)/szbfile.cc $(SZBASE_DIR)/szbfile.h \
			$(SZBASE_DIR)/szbname.cc $(SZBASE_DIR)/szbname.h \
			$(SZBASE_DIR)/definabledatablock.cc $(SZBASE_DIR)/definabledatablock.h \
			$(SZBASE_DIR)/definablecalculate.cc $(SZBASE_DIR)/definablecalculate.h \
			$(SZBASE_DIR)/realdatablock.cc $(SZBASE_DIR)/realdatablock.h \
			$(SZBASE_DIR)/combineddatablock.cc $(SZBASE_DIR)/combineddatablock.h \
			$(SZBASE_DIR)/cacheabledatablock.cc $(SZBASE_DIR)/cacheabledatablock.h \
			$(SZBASE_DIR)/luadatablock.cc $(SZBASE_DIR)/luadatablock.h \
			$(SZBASE_DIR)/szbhash.cc $(SZBASE_DIR)/szbhash.h \
			$(SZBASE_DIR)/szbdatablock.cc $(SZBASE_DIR)/szbdatablock.h \
			$(SZBASE_DIR)/szbfilewatcher.cc $(INCLUDE_DIR)/szbase/szbfilewatcher.h \
			$(SZBASE_DIR)/szbcancelhandle.cc $(INCLUDE_DIR)/szbase/szbcancelhandle.h \
			$(SZBASE_DIR)/loptdatablock.cc $(SZBASE_DIR)/loptdatablock.h \
			$(SZBASE_DIR)/proberconnection.cc $(SZBASE_DIR)/proberconnection.h \
			$(SZBASE_DIR)/luacalculate.cc $(SZBASE_DIR)/luacalculate.h \
			$(SZBASE_DIR)/loptcalculate.cc $(SZBASE_DIR)/loptcalculate.h \
			$(SZBASE_DIR)/probeblock.cc \
			$(SZBASE_DIR)/szbsearch.h $(SZBASE_DIR)/szbsearch.cc \
			$(SZBASE_DIR)/szbblock.cc

libSzarp2_la_LIBADD = libSzarpLuaParser.la

libSzarpLuaParser_la_CXXFLAGS = $(AM_CXXFLAGS) -g0
libSzarpLuaParser_la_SOURCES = \
			$(SZBASE_DIR)/lua_parser_extra.cc $(SZBASE_DIR)/lua_parser_extra.h \
			$(SZBASE_DIR)/lua_parser.cc $(INCLUDE_DIR)/szbase/lua_syntax.h

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in \
                       $(EXTRA_HEADERS) \
		       tools/ipk_config 
		       
                      
bin_SCRIPTS= \
             tools/ipk_config \
             $(TOOLS_DIR)/ipk_add_template \
             $(TOOLS_DIR)/ipk_base_max \
             $(TOOLS_DIR)/ipk_check_dtd \
             $(TOOLS_DIR)/ipk_create_template \
             $(TOOLS_DIR)/ipk_get_base \
             $(TOOLS_DIR)/ipk_move_draws \
             $(TOOLS_DIR)/ipk_normal_draws \
             $(TOOLS_DIR)/ipk_remove_filenames \
             $(TOOLS_DIR)/ipk_set_base 

libSzarp2_a_XSLT = \
                   $(XSLT_DIR)/first_base.xsl \
                   $(XSLT_DIR)/get_base.xsl \
                   $(XSLT_DIR)/list_tobase.xsl \
                   $(XSLT_DIR)/merge.xsl \
                   $(XSLT_DIR)/move_draw.xsl \
                   $(XSLT_DIR)/normalize_draw.xsl \
                   $(XSLT_DIR)/remove_base.xsl \
                   $(XSLT_DIR)/remove_empty.xsl \
                   $(XSLT_DIR)/remove_filenames.xsl \
                   $(XSLT_DIR)/select_with_name.xsl \
                   $(XSLT_DIR)/set_base.xsl \
                   $(XSLT_DIR)/sort_base.xsl \
		   $(XSLT_DIR)/extr_csv.xsl \
		   $(XSLT_DIR)/extr_oo.xsl

EXTRA_DIST = \
                      $(libSzarp2_a_XSLT) \
                      $(SOURCE_DIR)/Doxyfile \
		      $(TOOLS_DIR)/ipk_config.in \
                      $(bin_SCRIPTS)

install-data-local: $(libSzarp2_a_XSLT)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/xslt
	$(INSTALL) $(libSzarp2_a_XSLT) $(DESTDIR)@prefix@/resources/xslt

# Explicit rules for parsers.                      

doxy:
	doxygen $(SOURCE_DIR)/Doxyfile

