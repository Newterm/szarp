set(BASEDMN_SOURCES base_daemon.cc)

add_definitions(
  -DHAVE_CONFIG_H
  -DPREFIX="/opt/szarp"
)

add_executable(aqtdmn
  aqtdmn.cc
)

target_include_directories(aqtdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(aqtdmn
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

add_executable(calcdmn
  calcdmn.cc
)

target_include_directories(calcdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(calcdmn
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

add_executable(dbdmn
  dbdmn.cc
)

target_include_directories(dbdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(dbdmn
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

add_executable(ddedmn
  ddedmn.cc
)

target_include_directories(ddedmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(ddedmn
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
   xmlrpc-epi
)

add_executable(dpafdmn
  dpafdmn.cc
)

target_include_directories(dpafdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(dpafdmn
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

add_executable(dprdmn
  dprdmn.cc
)

target_include_directories(dprdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(dprdmn
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

add_executable(execdmn
  execdmn.cc
)

target_include_directories(execdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(execdmn
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

add_executable(ipcinfo
  ipcinfo.cc
)

target_include_directories(ipcinfo
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(ipcinfo
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

add_executable(k601dmn
  k601dmn.cc cfgdealer_handler.cpp
)

target_include_directories(k601dmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(k601dmn
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
   dl
)

add_executable(kamsdmn
  kamsdmn.cc cfgdealer_handler.cpp
)

target_include_directories(kamsdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(kamsdmn
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

add_executable(mbdmn
  mbdmn.cc
)

target_include_directories(mbdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(mbdmn
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

add_executable(borutadmn
  borutadmn.cc boruta_zet.cc boruta_modbus.cc boruta_fp210.cc
  boruta_wmtp.cc boruta_lumel.cc boruta_fc.cc cfgdealer_handler.cpp
)

target_include_directories(borutadmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(borutadmn
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

add_executable(borutadmn_z
  borutadmn_z.cc boruta_modbus_z.cc boruta_fc_z.cc cfgdealer_handler.cpp
)

target_include_directories(borutadmn_z
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(borutadmn_z
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

add_executable(mbrtudmn
  mbrtudmn.cc
)

target_include_directories(mbrtudmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(mbrtudmn
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

add_executable(mbusdmn
  mbusdmn.cc
)

target_include_directories(mbusdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(mbusdmn
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

add_executable(nrsdmn
  nrsdmn.cc
)

target_include_directories(nrsdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(nrsdmn
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

add_executable(pafdmn
  pafdmn.cc
)

target_include_directories(pafdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(pafdmn
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

add_executable(pafdmnr
  pafdmnr.cc
)

target_include_directories(pafdmnr
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(pafdmnr
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

add_executable(parcook
  parcook.cc funtable.cc
)

target_include_directories(parcook
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(parcook
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

add_executable(polludmn
  polludmn.cc
)

target_include_directories(polludmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(polludmn
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

add_executable(ratedmn
  ratedmn.cc
)

target_include_directories(ratedmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(ratedmn
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

add_executable(rsdmn
  nrsdmn.cc
)

target_include_directories(rsdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(rsdmn
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

add_executable(sampledmn
  sampledmn.cc base_daemon.cc
)

target_include_directories(sampledmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(sampledmn
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

add_executable(sbusdmn
  sbusdmn.cc cfgdealer_handler.cpp
)

target_include_directories(sbusdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(sbusdmn
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

add_executable(iecdmn
  iecdmn.cc
)

target_include_directories(iecdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(iecdmn
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

add_executable(prodmn
  prodmn.cc
)

target_include_directories(prodmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(prodmn
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

add_executable(tcpdmn
  tcpdmn.cc
)

target_include_directories(tcpdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(tcpdmn
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

add_executable(testdmn
  testdmn.cc
)

target_include_directories(testdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(testdmn
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

add_executable(wlkdmn
  wlkdmn.cc
)

target_include_directories(wlkdmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(wlkdmn
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

add_executable(pythondmn
  pythondmn.cc cfgdealer_handler.cpp
)

target_include_directories(pythondmn
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(pythondmn
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

#add_executable(defdmn
#  defdmn.cc defdmn.tpp funtable.cc
#)
#

add_subdirectory(s7daemon)
