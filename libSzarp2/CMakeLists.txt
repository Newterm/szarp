add_definitions(
  -DHAVE_CONFIG_H
)

## protobuf
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS protobuf/paramsvalues.proto)

## libSzarpLuaParser
add_library( SzarpLuaParser SHARED
  szarp_base_common/lua_parser_extra.cc
    szarp_base_common/lua_parser.cc
)

target_include_directories( SzarpLuaParser
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
)


## libSzarp2
add_library( Szarp2 SHARED
    ${PROTO_SRCS} ${PROTO_HDRS}
    ipk/tdevice.cc ipk/tdraw.cc
    ipk/tparam.cc ipk/param_formula_info.cpp
    ipk/tradio.cc ipk/traport.cc ipk/tsendparam.cc
    ipk/tszarpconfig.cc ipk/config_info.cpp
    ipk/tunit.cc ipk/tvalue.cc ipk/tseason.cc
    ipk/ipkcontainer.cc ipk/tdictionary.cc
    ipk/ttreenode.cc ipk/dmncfg.cc ipk/tdefined.cc
    ipk/xmlwrapper.cc ipk/tdrawdefinable.cc
    ipk/tscript.cc linedmn/ipchandler.cc
    utils/daemonize.cc misc/convuri.cc
    misc/crontime.cc misc/http.cc
    misc/insort.cc misc/tokens.cc misc/xmlutils.cc
    linedmn/mbus.cc linedmn/ppset.cc
    misc/xscale.cc szbextr/extractor.cc
    szbase/szbbase.cc
    szbase/szbbuf.cc szbase/szbdate.cc
    szbase/szbfile.cc
    szbase/szbname.cc
    szbase/definabledatablock.cc
    szbase/szb_definable_calculate.cc
    szbase/realdatablock.cc
    szbase/combineddatablock.cc
    szbase/cacheabledatablock.cc
    szbase/luadatablock.cc
    szbase/szbhash.cc
    szbase/szbdatablock.cc
    szbase/szbfilewatcher.cc
    szbase/szbcancelhandle.cc
    szbase/loptdatablock.cc
    szarp_base_common/lua_param_optimizer.cc
    szarp_base_common/lua_strings_extract.cc
    szarp_base_common/lua_utils.cc
    szarp_base_common/time.cc
    szarp_base_common/definable_calculate.cc
    szbase/proberconnection.cc
    szbase/luacalculate.cc
    szbase/loptcalculate.cc
    szbase/probeblock.cc
    szbase/szbsearch.cc szbase/szbblock.cc
    szarp_base_common/szbparammonitor.cc
    sz4/load_file_locked.cpp sz4/path.cpp
    sz4/buffer.cpp sz4/base.cpp sz4/block.cpp
    sz4/defs.cpp sz4/lua_interpreter.cpp
    sz4/sz4_time.cpp sz4/util.cpp
    sz4/block_cache.cpp sz4/filelock.cpp
    sz4/decode_file.cpp sz4/encode_file.cpp
    sz4/real_param_entry.cpp sz4/live_cache.cpp
)

set_property(TARGET Szarp2 PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories( Szarp2
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    /usr/include/libxml2
    /usr/include/lua5.1
)
