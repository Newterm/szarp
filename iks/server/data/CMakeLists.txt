file(GLOB SOURCES *.cpp)

add_definitions(
  -DHAVE_CONFIG_H
)

add_library(iks-server-data SHARED
  ${SOURCES}
)

target_include_directories(iks-server-data
  PUBLIC
    ${CMAKE_SOURCE_DIR}/iks/server/include
    ${CMAKE_SOURCE_DIR}/iks/common/
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
)

target_link_libraries(iks-server-data
   Szarp
   Szarp2
   SzarpLuaParser
   ${Boost_LIBRARIES}
   ${CURL_LIBRARIES}
   ${LIBXML2_LIBRARIES}
   ${LUA_LIBRARIES}
   ${PROTOBUF_LIBRARIES}
   ${Zip_LIBRARIES}
   ${LIBXSLT_LIBRARIES}
)
