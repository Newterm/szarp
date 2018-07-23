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
  -DINSTALL_PREFIX='"@prefix@"'
)


##### @IPKEDIT@ (executable) ####################

add_executable( @IPKEDIT@
  
  DESTINATION ${BIN_INSTALL_DIR}
)


##### other data ################################
