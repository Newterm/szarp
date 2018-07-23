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


##### s7dmn (executable) ########################

add_executable( s7dmn
  s7dmn.cc szpmap.cc s7client.cc s7qmap.cc s7query.cc $(basedmn_SOURCES)
  DESTINATION ${BIN_INSTALL_DIR}
)


##### szpmap_test (executable) ##################

add_executable( szpmap_test
  szpmap_test.cc szpmap.cc
  DESTINATION ${BIN_INSTALL_DIR}
)
