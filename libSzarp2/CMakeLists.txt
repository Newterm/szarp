file(GLOB SOURCES sz4/*.cpp szarp_base_common/*.cc szbase/*.cc szbextr/*.cc ipk/*.cc linedmn/*.cc misc/*.cc utils/*.cc)
file(GLOB_RECURSE HEADERS *.h)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS protobuf/paramsvalues.proto)

add_library(SzarpLuaParser SHARED
  szarp_base_common/lua_parser_extra.cpp
  szarp_base_common/lua_parser.cpp
)

target_include_directories(SzarpLuaParser
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_SOURCE_DIR}/libSzarp/include
)


add_library(Szarp2 SHARED
  ${PROTO_SRCS} ${PROTO_HDRS}
  ${SOURCES} ${HEADERS}
)

target_include_directories(Szarp2
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
  PRIVATE
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Szarp2
  PUBLIC
    zmq
    event
)
