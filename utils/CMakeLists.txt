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
  -DINSTALL_PREFIX='"/opt/szarp"'
)


##### agregator (executable) ##################

add_executable( agregator
  agregator.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### crypt (executable) ########################

add_executable( crypt
  crypt.c
  DESTINATION ${BIN_INSTALL_DIR}
)


##### lpparse (executable) ######################

add_executable( lpparse
  lpparse.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### precconv (executable) ###################

add_executable( precconv
  precconv.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### sproxy (executable) #######################

add_executable( sproxy
  sproxy.c
  DESTINATION ${BIN_INSTALL_DIR}
)


##### vrsh (executable) #########################

add_executable( vrsh
  vrsh.c
  DESTINATION ${BIN_INSTALL_DIR}
)


##### vrsh_d (executable) #######################

add_executable( vrsh_d
  vrsh.c
  DESTINATION ${BIN_INSTALL_DIR}
)


##### stampd (executable) #####################

add_executable( stampd
  stampd.c
  DESTINATION ${BIN_INSTALL_DIR}
)
