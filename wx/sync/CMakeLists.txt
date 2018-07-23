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


##### ssc (executable) ##########################

add_executable( ssc
  ssexception.h ssexception.cc sspackexchange.h sspackexchange.cc ssclient.h
    ssclient.cc ssutil.h ssutil.cc ssfsutil.h ssfsutil.cc ssstring.h hwkey.cc
    hwkey.h
  DESTINATION ${BIN_INSTALL_DIR}
)
