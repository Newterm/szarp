file(GLOB SOURCES sz4/*.cpp szarp_base_common/*.cc szbase/*.cc szbextr/*.cc ipk/*.cc linedmn/*.cc misc/*.cc utils/*.cc)

add_definitions(
  -DHAVE_CONFIG_H
)

## protobuf
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS protobuf/paramsvalues.proto)

## libSzarpLuaParser
add_library( SzarpLuaParser SHARED
  szarp_base_common/lua_parser_extra.cpp
  szarp_base_common/lua_parser.cpp
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
    ${SOURCES}
)

target_include_directories( Szarp2
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    /usr/include/libxml2
    /usr/include/lua5.1
)

target_link_libraries( Szarp2
  zmq
  event
)
