add_subdirectory(data)
add_subdirectory(net)
add_subdirectory(locations)
add_subdirectory(utils)

file(GLOB SOURCES *.cpp)

add_definitions(
  -DPREFIX=\"@prefix@\"
  -DBOOST_ENABLE_ASSERT_HANDLER
)

add_executable(iks-server
  ${SOURCES}
)

target_include_directories(iks-server
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${CMAKE_SOURCE_DIR}/iks/common
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(iks-server
  Szarp
  Szarp2
  SzarpLuaParser
  iks-common
  iks-server-data
  iks-server-net
  iks-server-location
  iks-server-utils
  ${Boost_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${LIBXSLT_LIBRARIES}
  ${LUA_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CURL_LIBRARIES}
  ${Zip_LIBRARIES}
  python2.7
)
