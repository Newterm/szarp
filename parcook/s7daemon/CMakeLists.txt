file(GLOB SOURCES *.cc)

add_definitions(
  -DHAVE_CONFIG_H
  -DPREFIX="/opt/szarp"
)
add_executable(s7dmn
  ${SOURCES} #../${BASEDMN_SOURCES}
)

target_include_directories(s7dmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
  # ${PYTHON_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(s7dmn
  Szarp
  Szarp2
  SzarpLuaParser
  ${Boost_LIBRARIES}
  ${PYTHON_LIBRARIES}
  ${CURL_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${LUA_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${Zip_LIBRARIES}
  ${LIBXSLT_LIBRARIES}
  python2.7
)

