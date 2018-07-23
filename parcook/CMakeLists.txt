cmake_minimum_required(VERSION 3.5)


include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

add_definitions(
  -DPREFIX=\"@prefix@\"
)


##### aqtdmn (executable) #######################

add_executable( aqtdmn
  aqtdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### calcdmn (executable) ######################

add_executable( calcdmn
  calcdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### dbdmn (executable) ########################

add_executable( dbdmn
  dbdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### @DDEDMN@ (executable) #####################

add_executable( ddedmn
  ddedmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### dpafdmn (executable) ######################

add_executable( dpafdmn
  dpafdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### dprdmn (executable) #######################

add_executable( dprdmn
  dprdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### execdmn (executable) ######################

add_executable( execdmn
  execdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### ipcinfo (executable) ######################

add_executable( ipcinfo
  ipcinfo.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### k601dmn (executable) ######################

add_executable( k601dmn
  k601dmn.cc cfgdealer_handler.cpp k601-prop-plugin.h
  DESTINATION ${BIN_INSTALL_DIR}
)


##### kamsdmn (executable) ######################

add_executable( kamsdmn
  kamsdmn.cc cfgdealer_handler.cpp
  DESTINATION ${BIN_INSTALL_DIR}
)


##### kwmsdmn (executable) ######################

add_executable( kwmsdmn
  kwmsdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### lbdmn (executable) ########################

add_executable( lbdmn
  lbdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### mbdmn (executable) ########################

add_executable( mbdmn
  mbdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### borutadmn (executable) ####################

add_executable( borutadmn
  borutadmn.cc borutadmn.h boruta_zet.cc boruta_modbus.cc boruta_fp210.cc
    boruta_wmtp.cc boruta_lumel.cc boruta_fc.cc cfgdealer_handler.cpp
  DESTINATION ${BIN_INSTALL_DIR}
)


##### borutadmn_z (executable) ##################

add_executable( borutadmn_z
  borutadmn_z.cc borutadmn_z.h boruta_modbus_z.cc boruta_fc_z.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### mbrtudmn (executable) #####################

add_executable( mbrtudmn
  mbrtudmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### mbusdmn (executable) ######################

add_executable( mbusdmn
  mbusdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### nrsdmn (executable) #######################

add_executable( nrsdmn
  nrsdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### pafdmn (executable) #######################

add_executable( pafdmn
  pafdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### pafdmnr (executable) ######################

add_executable( pafdmnr
  pafdmnr.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### parcook (executable) ######################

add_executable( parcook
  parcook.cc funtable.cc funtable.h
  DESTINATION ${BIN_INSTALL_DIR}
)


##### polludmn (executable) #####################

add_executable( polludmn
  polludmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### ratedmn (executable) ######################

add_executable( ratedmn
  ratedmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### rsdmn (executable) ########################

add_executable( rsdmn
  nrsdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### sampledmn (executable) ####################

add_executable( sampledmn
  sampledmn.cc $(basedmn_SOURCES)
  DESTINATION ${BIN_INSTALL_DIR}
)


##### sbusdmn (executable) ######################

add_executable( sbusdmn
  sbusdmn.cc cfgdealer_handler.cpp
  DESTINATION ${BIN_INSTALL_DIR}
)


##### iecdmn (executable) #######################

add_executable( iecdmn
  iecdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### prodmn (executable) #######################

add_executable( prodmn
  prodmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### tcpdmn (executable) #######################

add_executable( tcpdmn
  tcpdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### testdmn (executable) ######################

add_executable( testdmn
  testdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### wlkdmn (executable) #######################

add_executable( wlkdmn
  wlkdmn.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### pythondmn (executable) ####################

add_executable( pythondmn
  pythondmn.cc cfgdealer_handler.cpp
  DESTINATION ${BIN_INSTALL_DIR}
)


##### defdmn (executable) #######################

add_executable( defdmn
  defdmn.h defdmn.cc defdmn.tpp funtable.cc funtable.h
  DESTINATION ${BIN_INSTALL_DIR}
)
