file(GLOB SOURCES *.cpp)

add_definitions(
  -DHAVE_CONFIG_H
)

add_library(iks-server-utils SHARED
  ${SOURCES}
)

target_include_directories(iks-server-utils
  PUBLIC
    ${CMAKE_SOURCE_DIR}/iks/server/include
)
#
#target_link_libraries(iks-server-utils
#   Szarp
#   Szarp2
#   SzarpLuaParser
#   ${Boost_LIBRARIES}
#   ${CURL_LIBRARIES}
#   ${LIBXML2_LIBRARIES}
#   ${LUA_LIBRARIES}
#   ${PROTOBUF_LIBRARIES}
#   ${Zip_LIBRARIES}
#   ${LIBXSLT_LIBRARIES}
#)
