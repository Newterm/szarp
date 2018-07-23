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
  -DWNCK_I_KNOW_THIS_IS_UNSTABLE
)


##### viszio (executable) #######################

add_executable( viszio
  viszioApp.cpp viszioFetchFrame.cpp viszioTransparentFrame.cpp
    viszioParamadd.cpp viszioConfiguration.cpp viszioApp.h viszioFetchFrame.h
    viszioTransparentFrame.h viszioParamadd.h viszioConfiguration.h
  DESTINATION ${BIN_INSTALL_DIR}
)
