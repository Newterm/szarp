add_definitions(
  -DPREFIX="/opt/szarp"
)

add_executable(meaner3
  meaner3.cc tmeaner.cc tparcook.cc tsaveparam.cc
  texecute.cc texecsect.cc texecalarm.cc tstatus.cc
  twriter.cc
)

target_include_directories(meaner3
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(meaner3
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

add_executable(peakator
  utils/peakator.cc tsaveparam.cc tstatus.cc
)

target_include_directories(peakator
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(peakator
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

add_executable(prober
  fileremover.cc tparcook.cc tsaveparam.cc twriter.cc
  tprober.cc tstatus.cc prober.cc
)

target_include_directories(prober
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(prober
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

add_executable(szbedit
  utils/szbedit.cc
)

target_include_directories(szbedit
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(szbedit
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
   newt
)

add_executable(extrszb
  utils/extrszb.cc
)

target_include_directories(extrszb
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(extrszb
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

add_executable(szbindex
  utils/szbindex.cc
)

target_include_directories(szbindex
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(szbindex
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

add_executable(szbreader
  utils/szbreader.cc
)

target_include_directories(szbreader
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(szbreader
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

add_executable(szbwriter
  utils/szbwriter.cc utils/szbwriter_cache.cc utils/tmmapparam.cc
  tsaveparam.cc tstatus.cc
)

target_include_directories(szbwriter
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(szbwriter
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

add_executable(szbndremover
  utils/szbndremover.cc tsaveparam.cc tstatus.cc
)

target_include_directories(szbndremover
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(szbndremover
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

add_executable(szbmod
  utils/szbmod.cc tsaveparam.cc tstatus.cc
)

target_include_directories(szbmod
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(szbmod
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

add_executable(szbnull
  utils/szbnull.cc
)

target_include_directories(szbnull
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(szbnull
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
