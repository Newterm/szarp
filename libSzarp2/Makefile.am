
# Pawe³ Pa³ucha pawel@praterm.com.pl
#
# $Id: Makefile.am 6789 2009-05-29 12:03:33Z pawel $

SUBDIRS =

LEX=@LEX@
YACC=@YACC@

LDADD = @LEXLIB@ @CURL_LIBS@ @XSLT_LIBS@ @XML_LIBS@ @LUA_LIBS@
	@ZIP_FLAGS@ \
	libSzarp2.la ../libSzarp/libSzarp.la

AM_CPPFLAGS = @XML_CFLAGS@ @XSLT_CFLAGS@ @CURL_CFLAGS@ @LUA_CFLAGS@ \
	@BOOST_CPPFLAGS@ @ZIP_CFLAGS@ \
	-I@srcdir@/include \
	-I@srcdir@/include/szbase \
	-I@srcdir@/../libSzarp/include \
	-I@srcdir@/szbase \
	-DPREFIX=\"@prefix@\" \
	-I@srcdir@/protobuf

noinst_LTLIBRARIES = libSzarp2.la libSzarpLuaParser.la

EXTRA_PROGRAMS = szbtest lua_parser_test xmlreader_test
szbtest_SOURCES = @srcdir@/szbase/szbtest.cc

@srcdir@/protobuf/paramsvalues.pb.h: @srcdir@/protobuf/paramsvalues.proto
	$(PROTOC) --cpp_out=@srcdir@/protobuf -I@srcdir@/protobuf @srcdir@/protobuf/paramsvalues.proto

@srcdir@/protobuf/paramsvalues.pb.cc: @srcdir@/protobuf/paramsvalues.proto
	$(PROTOC) --cpp_out=@srcdir@/protobuf -I@srcdir@/protobuf @srcdir@/protobuf/paramsvalues.proto

@srcdir@/include/protobuf/paramsvalues.pb.h: @srcdir@/protobuf/paramsvalues.pb.h
	mkdir -p @srcdir@/include/protobuf
	cp $< $@

lua_parser_test_INCLUDES = $(INCLUDES)
lua_parser_test_LDADD = $(LDADD) ../libSzarp/libSzarp.la libSzarp2.la
lua_parser_test_SOURCES = @srcdir@/szarp_base_common/lua_parser_test.cc

xmlreader_test_INCLUDES = $(INCLUDES)
xmlreader_test_LDADD = $(LDADD) ../libSzarp/libSzarp.la libSzarp2.la \
	$(BOOST_LDFLAGS) $(BOOST_SYSTEM_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) \
	$(BOOST_REGEX_LIB) $(BOOST_DATE_TIME_LIB)  ../libSzarp/liblog.o
xmlreader_test_SOURCES = @srcdir@/ipk/xmlreader_test.cc

test: szbtest
	./szbtest

if MINGW32_OPT
INCLUDE_PROTOBUF =
else
INCLUDE_PROTOBUF = @srcdir@/include/protobuf/paramsvalues.pb.h
endif

noinst_HEADERS = \
	@srcdir@/include/dmncfg.h \
	@srcdir@/include/httpcl.h \
	@srcdir@/include/ipchandler.h \
	@srcdir@/include/insort.h \
	@srcdir@/include/szarp_config.h \
	@srcdir@/include/tokens.h \
	@srcdir@/include/convuri.h \
	@srcdir@/include/ppset.h \
	@srcdir@/include/xmlutils.h \
	@srcdir@/include/custom_assert.h \
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
	@srcdir@/linedmn/daemonutils.cc \
	@srcdir@/linedmn/baseconn.cc \
	@srcdir@/linedmn/bserialport.cc \
	@srcdir@/linedmn/serialport.cc \
	@srcdir@/linedmn/serialadapter.cc \
	@srcdir@/linedmn/tcpconn.cc \
	@srcdir@/linedmn/httpclient.cc \
	@srcdir@/linedmn/atcconn.cc \
	@srcdir@/linedmn/evbase.cc \
	@srcdir@/linedmn/tcpserver.cc
endif

if MINGW32_OPT
PROTOBUF =
PROTOBUF_ALL =
else
PROTOBUF = \
	@srcdir@/protobuf/paramsvalues.pb.cc \
	@srcdir@/include/protobuf/paramsvalues.pb.h
PROTOBUF_ALL = \
	$(PROTOBUF) \
	@srcdir@/protobuf/paramsvalues.pb.h
endif

libSzarp2_la_SOURCES = \
	$(PROTOBUF) \
	@srcdir@/ipk/tdevice.cc \
	@srcdir@/ipk/tdraw.cc \
	@srcdir@/ipk/tparam.cc \
	@srcdir@/ipk/param_formula_info.cpp \
	@srcdir@/ipk/tradio.cc \
	@srcdir@/ipk/traport.cc \
	@srcdir@/ipk/tsendparam.cc \
	@srcdir@/ipk/tszarpconfig.cc \
	@srcdir@/ipk/config_info.cpp \
	@srcdir@/ipk/tunit.cc \
	@srcdir@/ipk/tvalue.cc \
	@srcdir@/ipk/tseason.cc \
	@srcdir@/ipk/ipkcontainer.cc \
	@srcdir@/ipk/tdictionary.cc \
	@srcdir@/ipk/ttreenode.cc \
	@srcdir@/ipk/dmncfg.cc \
	@srcdir@/ipk/tdefined.cc \
	@srcdir@/ipk/xmlwrapper.cc \
	@srcdir@/ipk/tdrawdefinable.cc \
	@srcdir@/ipk/tscript.cc \
	@srcdir@/linedmn/ipchandler.cc \
	@srcdir@/utils/daemonize.cc \
	@srcdir@/misc/convuri.cc \
	@srcdir@/misc/crontime.h \
	@srcdir@/misc/crontime.cc \
	@srcdir@/misc/http.cc \
	@srcdir@/misc/insort.cc \
	@srcdir@/misc/tokens.cc \
	@srcdir@/misc/xmlutils.cc \
	@srcdir@/linedmn/mbus.cc \
	@srcdir@/linedmn/ppset.cc \
	$(BASECONN) \
	@srcdir@/misc/xscale.cc \
	@srcdir@/szbextr/extractor.cc \
	@srcdir@/szbextr/extr.h \
	$(INCLUDE_PARSERS) \
	$(noinst_HEADERS) \
	@srcdir@/szbase/szbbase.cc \
	@srcdir@/szbase/szbbase.h \
	@srcdir@/szbase/szbbuf.cc \
	@srcdir@/szbase/szbbuf.h \
	@srcdir@/szbase/szbdate.cc \
	@srcdir@/szbase/szbdate.h \
	@srcdir@/szbase/szbfile.cc \
	@srcdir@/szbase/szbfile.h \
	@srcdir@/szbase/szbname.cc \
	@srcdir@/szbase/szbname.h \
	@srcdir@/szbase/definabledatablock.cc \
	@srcdir@/szbase/definabledatablock.h \
	@srcdir@/szbase/szb_definable_calculate.cc \
	@srcdir@/szbase/szb_definable_calculate.h \
	@srcdir@/szbase/realdatablock.cc \
	@srcdir@/szbase/realdatablock.h \
	@srcdir@/szbase/combineddatablock.cc \
	@srcdir@/szbase/combineddatablock.h \
	@srcdir@/szbase/cacheabledatablock.cc \
	@srcdir@/szbase/cacheabledatablock.h \
	@srcdir@/szbase/luadatablock.cc \
	@srcdir@/szbase/luadatablock.h \
	@srcdir@/szbase/szbhash.cc \
	@srcdir@/szbase/szbhash.h \
	@srcdir@/szbase/szbdatablock.cc \
	@srcdir@/szbase/szbdatablock.h \
	@srcdir@/szbase/szbfilewatcher.cc \
	@srcdir@/include/szbase/szbfilewatcher.h \
	@srcdir@/szbase/szbcancelhandle.cc \
	@srcdir@/include/szbase/szbcancelhandle.h \
	@srcdir@/szbase/loptdatablock.cc \
	@srcdir@/szbase/loptdatablock.h \
	@srcdir@/szarp_base_common/lua_param_optimizer.cc \
	@srcdir@/include/szarp_base_common/lua_param_optimizer.h \
	@srcdir@/include/szarp_base_common/lua_param_optimizer_templ.h \
	@srcdir@/szarp_base_common/lua_strings_extract.cc \
	@srcdir@/include/szarp_base_common/lua_strings_extract.h \
	@srcdir@/szarp_base_common/lua_utils.cc \
  	@srcdir@/include/szarp_base_common/lua_utils.h \
	@srcdir@/szarp_base_common/time.cc \
	@srcdir@/include/szarp_base_common/time.h \
	@srcdir@/szarp_base_common/definable_calculate.cc \
	@srcdir@/include/szarp_base_common/definable_calculate.h \
	@srcdir@/include/szarp_base_common/lua_syntax_fusion_adapt.h \
	@srcdir@/szbase/proberconnection.cc \
	@srcdir@/szbase/proberconnection.h \
	@srcdir@/szbase/luacalculate.cc \
	@srcdir@/szbase/luacalculate.h \
	@srcdir@/szbase/loptcalculate.cc \
	@srcdir@/szbase/loptcalculate.h \
	@srcdir@/szbase/probeblock.cc \
	@srcdir@/szbase/szbsearch.h \
	@srcdir@/szbase/szbsearch.cc \
	@srcdir@/szbase/szbblock.cc \
	@srcdir@/szarp_base_common/szbparammonitor.cc \
	@srcdir@/include/szarp_base_common/szbparammonitor.h \
	@srcdir@/include/szarp_base_common/szbparamobserver.h \
	@srcdir@/include/sz4/block.h \
	@srcdir@/include/sz4/load_file_locked.h \
	@srcdir@/include/sz4/buffer.h \
	@srcdir@/include/sz4/buffer_templ.h \
	@srcdir@/include/sz4/real_param_entry.h \
	@srcdir@/include/sz4/real_param_entry_templ.h \
	@srcdir@/include/sz4/lua_optimized_param_entry.h \
	@srcdir@/include/sz4/lua_optimized_param_entry_templ.h \
	@srcdir@/include/sz4/lua_interpreter.h \
	@srcdir@/include/sz4/lua_param_entry.h \
	@srcdir@/include/sz4/rpn_param_entry.h \
	@srcdir@/include/sz4/combined_param_entry.h \
	@srcdir@/include/sz4/base.h \
	@srcdir@/include/sz4/defs.h \
	@srcdir@/include/sz4/util.h \
	@srcdir@/include/sz4/block_cache..h \
	@srcdir@/include/sz4/lua_first_last_time..h \
	@srcdir@/include/sz4/decode_file.h \
	@srcdir@/include/sz4/encode_file.h \
	@srcdir@/sz4/load_file_locked.cpp \
	@srcdir@/sz4/path.cpp \
	@srcdir@/sz4/buffer.cpp \
	@srcdir@/sz4/base.cpp \
	@srcdir@/sz4/block.cpp \
	@srcdir@/sz4/defs.cpp \
	@srcdir@/sz4/lua_interpreter.cpp \
	@srcdir@/sz4/sz4_time.cpp \
	@srcdir@/sz4/util.cpp \
	@srcdir@/sz4/block_cache.cpp \
	@srcdir@/sz4/filelock.cpp \
	@srcdir@/sz4/decode_file.cpp \
	@srcdir@/sz4/encode_file.cpp \
	@srcdir@/sz4/real_param_entry.cpp \
	@srcdir@/sz4/live_cache.cpp

if MINGW32_OPT
else
libSzarp2_la_SOURCES += @srcdir@/linedmn/zmqhandler.cc
endif

libSzarp2_la_LIBADD = libSzarpLuaParser.la

libSzarpLuaParser_la_CXXFLAGS = $(AM_CXXFLAGS) -g0
libSzarpLuaParser_la_SOURCES = \
	@srcdir@/szarp_base_common/lua_parser_extra.cc @srcdir@/szarp_base_common/lua_parser_extra.h \
	@srcdir@/szarp_base_common/lua_parser.cc @srcdir@/include/szarp_base_common/lua_syntax.h

# Additional files for deleting on 'make maintainer-clean'
	$(EXTRA_HEADERS) \
	tools/ipk_config \
	$(PROTOBUF_ALL)

if MINGW32_OPT
INC_TO_BIN =
else
INC_TO_BIN = @srcdir@/tools/ipk_config
endif

bin_SCRIPTS= \
	$(INC_TO_BIN) \
	@srcdir@/tools/ipk_add_template \
	@srcdir@/tools/ipk_base_max \
	@srcdir@/tools/ipk_check_dtd \
	@srcdir@/tools/ipk_create_template \
	@srcdir@/tools/ipk_get_base \
	@srcdir@/tools/ipk_move_draws \
	@srcdir@/tools/ipk_normal_draws \
	@srcdir@/tools/ipk_remove_filenames \
	@srcdir@/tools/ipk_set_base

libSzarp2_a_XSLT = \
	@srcdir@/xslt/first_base.xsl \
	@srcdir@/xslt/get_base.xsl \
	@srcdir@/xslt/list_tobase.xsl \
	@srcdir@/xslt/merge.xsl \
	@srcdir@/xslt/move_draw.xsl \
	@srcdir@/xslt/normalize_draw.xsl \
	@srcdir@/xslt/remove_base.xsl \
	@srcdir@/xslt/remove_empty.xsl \
	@srcdir@/xslt/remove_filenames.xsl \
	@srcdir@/xslt/select_with_name.xsl \
	@srcdir@/xslt/set_base.xsl \
	@srcdir@/xslt/sort_base.xsl \
	@srcdir@/xslt/extr_csv.xsl \
	@srcdir@/xslt/extr_oo.xsl

EXTRA_DIST = \
	$(libSzarp2_a_XSLT) \
	@srcdir@/Doxyfile \
	@srcdir@/include/ipk_config.in \
	$(bin_SCRIPTS)

install-data-local: $(libSzarp2_a_XSLT)
	$(INSTALL) -d $(DESTDIR)@prefix@/resources/xslt
	$(INSTALL) $(libSzarp2_a_XSLT) $(DESTDIR)@prefix@/resources/xslt

# Explicit rules for parsers.

doxy:
	doxygen @srcdir@/Doxyfile
