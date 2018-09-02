file(GLOB_RECURSE SOURCES *.cpp)

add_library(iks-server-location SHARED
  ${SOURCES}
)

target_include_directories(iks-server-location
  PUBLIC
    ${CMAKE_SOURCE_DIR}/iks/server/include
    ${CMAKE_SOURCE_DIR}/iks/common/
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
)

target_link_libraries(iks-server-location
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
