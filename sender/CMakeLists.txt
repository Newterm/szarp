file(GLOB SOURCES *.cc)

add_definitions(
  -DHAVE_CONFIG_H
  -DPREFIX="/opt/szarp"
)

add_executable(sender
  ${SOURCES}
)

target_include_directories(sender
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(sender
  Szarp
  Szarp2
  SzarpLuaParser
  xml2
  luajit-5.1
  boost_system
  boost_filesystem
  protobuf
  boost_date_time
  boost_thread
  boost_locale
  curl
  xslt
  zip
)
