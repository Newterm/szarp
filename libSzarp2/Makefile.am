
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6789 2009-05-29 12:03:33Z pawel $

SUBDIRS = 

LEX=@LEX@
YACC=@YACC@

SOURCE_DIR=@srcdir@
IPK_DIR=$(SOURCE_DIR)/ipk
SZARP_BASE_COMMON_DIR=$(SOURCE_DIR)/szarp_base_common
SZBASE_DIR=$(SOURCE_DIR)/szbase
SZ4_DIR=$(SOURCE_DIR)/sz4
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
	-DPREFIX=\"@prefix@\"				\
	-I$(SOURCE_DIR)/protobuf

noinst_LTLIBRARIES = libSzarp2.la libSzarpLuaParser.la

EXTRA_PROGRAMS = szbtest lua_parser_test xmlreader_test
szbtest_SOURCES = $(SZBASE_DIR)/szbtest.cc


$(SOURCE_DIR)/protobuf/paramsvalues.pb.h: $(SOURCE_DIR)/protobuf/paramsvalues.proto
	$(PROTOC) --cpp_out=$(SOURCE_DIR)/protobuf -I$(SOURCE_DIR)/protobuf $(SOURCE_DIR)/protobuf/paramsvalues.proto

$(SOURCE_DIR)/protobuf/paramsvalues.pb.cc: $(SOURCE_DIR)/protobuf/paramsvalues.proto
	$(PROTOC) --cpp_out=$(SOURCE_DIR)/protobuf -I$(SOURCE_DIR)/protobuf $(SOURCE_DIR)/protobuf/paramsvalues.proto

$(SOURCE_DIR)/include/protobuf/paramsvalues.pb.h: $(SOURCE_DIR)/protobuf/paramsvalues.pb.h
	mkdir -p $(SOURCE_DIR)/include/protobuf
	cp $< $@


lua_parser_test_INCLUDES = $(INCLUDES)
lua_parser_test_LDADD = $(LDADD) ../libSzarp/libSzarp.la libSzarp2.la
lua_parser_test_SOURCES = $(SZARP_BASE_COMMON_DIR)/lua_parser_test.cc

xmlreader_test_INCLUDES = $(INCLUDES)
xmlreader_test_LDADD = $(LDADD) ../libSzarp/libSzarp.la libSzarp2.la $(BOOST_LDFLAGS) $(BOOST_SYSTEM_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_REGEX_LIB) $(BOOST_DATE_TIME_LIB)  ../libSzarp/liblog.o
xmlreader_test_SOURCES = $(IPK_DIR)/xmlreader_test.cc


test: szbtest
	./szbtest
 
if MINGW32_OPT
INCLUDE_PROTOBUF =
else
INCLUDE_PROTOBUF = $(INCLUDE_DIR)/protobuf/paramsvalues.pb.h
endif

noinst_HEADERS = \
                      $(INCLUDE_DIR)/dmncfg.h \
                      $(INCLUDE_DIR)/httpcl.h \
                      $(INCLUDE_DIR)/ipchandler.h \
                      $(INCLUDE_DIR)/insort.h \
                      $(INCLUDE_DIR)/szarp_config.h \
		      $(INCLUDE_DIR)/tokens.h \
		      $(INCLUDE_DIR)/convuri.h \
                      $(INCLUDE_DIR)/ppset.h \
                      $(INCLUDE_DIR)/xmlutils.h \
                      $(INCLUDE_PROTOBUF)

if MINGW32_OPT
INCLUDE_PARSERS =
else
INCLUDE_PARSERS = $(PARSERS_FILES)
endif

if MINGW32_OPT
BASECONN =
else
BASECONN = \
			$(DMN_DIR)/daemonutils.cc \
			$(DMN_DIR)/baseconn.cc \
			$(DMN_DIR)/bserialport.cc \
			$(DMN_DIR)/serialport.cc \
			$(DMN_DIR)/serialadapter.cc \
			$(DMN_DIR)/tcpconn.cc \
			$(DMN_DIR)/httpclient.cc \
			$(DMN_DIR)/atcconn.cc \
			$(DMN_DIR)/evbase.cc \
			$(DMN_DIR)/tcpserver.cc
endif

if MINGW32_OPT
PROTOBUF =
PROTOBUF_ALL =
else
PROTOBUF = \
			$(SOURCE_DIR)/protobuf/paramsvalues.pb.cc \
			$(SOURCE_DIR)/include/protobuf/paramsvalues.pb.h
PROTOBUF_ALL = \
            $(PROTOBUF) \
			$(SOURCE_DIR)/protobuf/paramsvalues.pb.h
endif



libSzarp2_la_SOURCES = \
			$(PROTOBUF) \
			$(IPK_DIR)/tdevice.cc \
			$(IPK_DIR)/tdraw.cc \
			$(IPK_DIR)/tparam.cc \
			$(IPK_DIR)/tradio.cc \
			$(IPK_DIR)/traport.cc \
			$(IPK_DIR)/tsendparam.cc \
			$(IPK_DIR)/tszarpconfig.cc \
			$(IPK_DIR)/tunit.cc \
			$(IPK_DIR)/tvalue.cc \
			$(IPK_DIR)/tseason.cc \
			$(IPK_DIR)/ipkcontainer.cc \
			$(IPK_DIR)/tdictionary.cc \
			$(IPK_DIR)/ttreenode.cc \
			$(IPK_DIR)/dmncfg.cc \
			$(IPK_DIR)/tdefined.cc \
			$(IPK_DIR)/xmlwrapper.cc \
			$(IPK_DIR)/tdrawdefinable.cc \
			$(IPK_DIR)/tscript.cc \
			$(DMN_DIR)/ipchandler.cc \
			$(SOURCE_DIR)/utils/daemonize.cc \
			$(SOURCE_DIR)/misc/convuri.cc \
			$(SOURCE_DIR)/misc/crontime.h \
			$(SOURCE_DIR)/misc/crontime.cc \
			$(SOURCE_DIR)/misc/http.cc \
			$(SOURCE_DIR)/misc/insort.cc \
			$(SOURCE_DIR)/misc/tokens.cc \
			$(SOURCE_DIR)/misc/xmlutils.cc \
			$(DMN_DIR)/mbus.cc \
			$(DMN_DIR)/ppset.cc \
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
			$(SZBASE_DIR)/szb_definable_calculate.cc $(SZBASE_DIR)/szb_definable_calculate.h \
			$(SZBASE_DIR)/realdatablock.cc $(SZBASE_DIR)/realdatablock.h \
			$(SZBASE_DIR)/combineddatablock.cc $(SZBASE_DIR)/combineddatablock.h \
			$(SZBASE_DIR)/cacheabledatablock.cc $(SZBASE_DIR)/cacheabledatablock.h \
			$(SZBASE_DIR)/luadatablock.cc $(SZBASE_DIR)/luadatablock.h \
			$(SZBASE_DIR)/szbhash.cc $(SZBASE_DIR)/szbhash.h \
			$(SZBASE_DIR)/szbdatablock.cc $(SZBASE_DIR)/szbdatablock.h \
			$(SZBASE_DIR)/szbfilewatcher.cc $(INCLUDE_DIR)/szbase/szbfilewatcher.h \
			$(SZBASE_DIR)/szbcancelhandle.cc $(INCLUDE_DIR)/szbase/szbcancelhandle.h \
			$(SZBASE_DIR)/loptdatablock.cc $(SZBASE_DIR)/loptdatablock.h \
			$(SZARP_BASE_COMMON_DIR)/lua_param_optimizer.cc $(INCLUDE_DIR)/szarp_base_common/lua_param_optimizer.h \
			$(INCLUDE_DIR)/szarp_base_common/lua_param_optimizer_templ.h \
			$(SZARP_BASE_COMMON_DIR)/lua_strings_extract.cc	$(INCLUDE_DIR)/szarp_base_common/lua_strings_extract.h \
			$(SZARP_BASE_COMMON_DIR)/lua_utils.cc $(INCLUDE_DIR)/szarp_base_common/lua_utils.h \
			$(SZARP_BASE_COMMON_DIR)/time.cc $(INCLUDE_DIR)/szarp_base_common/time.h \
			$(SZARP_BASE_COMMON_DIR)/definable_calculate.cc $(INCLUDE_DIR)/szarp_base_common/definable_calculate.h \
			$(INCLUDE_DIR)/szarp_base_common/lua_syntax_fusion_adapt.h \
			$(SZBASE_DIR)/proberconnection.cc $(SZBASE_DIR)/proberconnection.h \
			$(SZBASE_DIR)/luacalculate.cc $(SZBASE_DIR)/luacalculate.h \
			$(SZBASE_DIR)/loptcalculate.cc $(SZBASE_DIR)/loptcalculate.h \
			$(SZBASE_DIR)/probeblock.cc \
			$(SZBASE_DIR)/szbsearch.h $(SZBASE_DIR)/szbsearch.cc \
			$(SZBASE_DIR)/szbblock.cc \
			$(SZARP_BASE_COMMON_DIR)/szbparammonitor.cc $(INCLUDE_DIR)/szarp_base_common/szbparammonitor.h \
			$(INCLUDE_DIR)/szarp_base_common/szbparamobserver.h \
			\
			$(INCLUDE_DIR)/sz4/block.h \
			$(INCLUDE_DIR)/sz4/load_file_locked.h \
			$(INCLUDE_DIR)/sz4/buffer.h \
			$(INCLUDE_DIR)/sz4/buffer_templ.h \
			$(INCLUDE_DIR)/sz4/real_param_entry.h \
			$(INCLUDE_DIR)/sz4/real_param_entry_templ.h \
			$(INCLUDE_DIR)/sz4/lua_optimized_param_entry.h \
			$(INCLUDE_DIR)/sz4/lua_optimized_param_entry_templ.h \
			$(INCLUDE_DIR)/sz4/lua_interpreter.h \
			$(INCLUDE_DIR)/sz4/lua_param_entry.h \
			$(INCLUDE_DIR)/sz4/rpn_param_entry.h \
			$(INCLUDE_DIR)/sz4/combined_param_entry.h \
			$(INCLUDE_DIR)/sz4/base.h \
			$(INCLUDE_DIR)/sz4/defs.h \
			$(INCLUDE_DIR)/sz4/util.h \
			$(INCLUDE_DIR)/sz4/block_cache..h \
			$(INCLUDE_DIR)/sz4/lua_first_last_time..h \
			$(INCLUDE_DIR)/sz4/decode_file.h \
			$(INCLUDE_DIR)/sz4/encode_file.h \
			$(SZ4_DIR)/load_file_locked.cpp \
			$(SZ4_DIR)/path.cpp \
			$(SZ4_DIR)/buffer.cpp \
			$(SZ4_DIR)/base.cpp \
			$(SZ4_DIR)/block.cpp \
			$(SZ4_DIR)/defs.cpp \
			$(SZ4_DIR)/lua_interpreter.cpp \
			$(SZ4_DIR)/sz4_time.cpp \
			$(SZ4_DIR)/util.cpp \
			$(SZ4_DIR)/block_cache.cpp \
			$(SZ4_DIR)/filelock.cpp \
			$(SZ4_DIR)/decode_file.cpp \
			$(SZ4_DIR)/encode_file.cpp \
			$(SZ4_DIR)/real_param_entry.cpp \
			$(SZ4_DIR)/live_cache.cpp

if MINGW32_OPT
else
libSzarp2_la_SOURCES += $(DMN_DIR)/zmqhandler.cc
endif


libSzarp2_la_LIBADD = libSzarpLuaParser.la

libSzarpLuaParser_la_CXXFLAGS = $(AM_CXXFLAGS) -g0
libSzarpLuaParser_la_SOURCES = \
			$(SZARP_BASE_COMMON_DIR)/lua_parser_extra.cc $(SZARP_BASE_COMMON_DIR)/lua_parser_extra.h \
			$(SZARP_BASE_COMMON_DIR)/lua_parser.cc $(INCLUDE_DIR)/szarp_base_common/lua_syntax.h

# Additional files for deleting on 'make maintainer-clean'
MAINTAINERCLEANFILES = Makefile.in \
                       $(EXTRA_HEADERS) \
                       tools/ipk_config \
                       $(PROTOBUF_ALL)

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

