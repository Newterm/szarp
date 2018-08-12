set(BASEDMN_SOURCES base_daemon.cc)

add_definitions(
  -DHAVE_CONFIG_H
  -DPREFIX="/opt/szarp"
)

#add_executable(aqtdmn
#  aqtdmn.cc
#)
#
#add_executable(calcdmn
#  calcdmn.cc
#)
#
#add_executable(dbdmn
#  dbdmn.cc
#)
#
#add_executable(ddedmn
#  ddedmn.cc
#)
#
#add_executable(dpafdmn
#  dpafdmn.cc
#)
#
#add_executable(dprdmn
#  dprdmn.cc
#)
#
#add_executable(execdmn
#  execdmn.cc
#)
#
#add_executable(ipcinfo
#  ipcinfo.cc
#)
#
#add_executable(k601dmn
#  k601dmn.cc cfgdealer_handler.cpp
#)
#
#add_executable(kamsdmn
#  kamsdmn.cc cfgdealer_handler.cpp
#)
#
#add_executable(mbdmn
#  mbdmn.cc
#)
#
#add_executable(borutadmn
#  borutadmn.cc boruta_zet.cc boruta_modbus.cc boruta_fp210.cc
#  boruta_wmtp.cc boruta_lumel.cc boruta_fc.cc cfgdealer_handler.cpp
#)
#
#add_executable(borutadmn_z
#  borutadmn_z.cc boruta_modbus_z.cc boruta_fc_z.cc
#)
#
#add_executable(mbrtudmn
#  mbrtudmn.cc
#)
#
#add_executable(mbusdmn
#  mbusdmn.cc
#)
#
#add_executable(nrsdmn
#  nrsdmn.cc
#)
#
#add_executable(pafdmn
#  pafdmn.cc
#)
#
#add_executable(pafdmnr
#  pafdmnr.cc
#)
#
#add_executable(parcook
#  parcook.cc funtable.cc
#)
#
#add_executable(polludmn
#  polludmn.cc
#)
#
#add_executable(ratedmn
#  ratedmn.cc
#)
#
#add_executable(rsdmn
#  nrsdmn.cc
#)
#
#add_executable(sampledmn
#  sampledmn.cc ${BASEDMN_SOURCES}
#)
#
#add_executable(sbusdmn
#  sbusdmn.cc cfgdealer_handler.cpp
#)
#
#add_executable(iecdmn
#  iecdmn.cc
#)
#
#add_executable(prodmn
#  prodmn.cc
#)
#
#add_executable(tcpdmn
#  tcpdmn.cc
#)
#
#add_executable(testdmn
#  testdmn.cc
#)
#
#add_executable(wlkdmn
#  wlkdmn.cc
#)
#
#target_include_directories(wlkdmn
#  PUBLIC
#  #${CMAKE_SOURCE_DIR}/libSzarp/include
#    ${CMAKE_SOURCE_DIR}/libSzarp2/include
#    # ${LIBXML2_INCLUDE_DIR}
#    #${PYTHON_INCLUDE_DIRS}
#    #${LUA_INCLUDE_DIR}
#)

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
  #server
  #Threads::Threads
  xml2
  luajit-5.1
  #pam
  ${Boost_LIBRARIES}
  ${PYTHON_LIBRARIES}
  boost_python3
  protobuf
  #ssl
  #crypto
  #systemd
  curl
  xslt
  zip
)

#add_executable(defdmn
#  defdmn.cc defdmn.tpp funtable.cc
#)
#
#add_executable(s7dmn
#  snap7dmn.cc ${BASEDMN_SOURCES}
#)
#
#target_include_directories(s7dmn
#  PUBLIC
#  #${CMAKE_SOURCE_DIR}/libSzarp/include
#  #  ${CMAKE_SOURCE_DIR}/libSzarp2/include
#  #  ${CMAKE_SOURCE_DIR}/isl/server/include
#    ${LIBXML2_INCLUDE_DIR}
#    #    ${PYTHON_INCLUDE_DIRS}
#    # ${LUA_INCLUDE_DIR}
#)
