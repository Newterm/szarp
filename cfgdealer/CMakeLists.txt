#todo test!

add_executable(cfgdealer
  cfgdealer.cpp
)

target_include_directories(cfgdealer
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(cfgdealer
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
   python2.7
)
