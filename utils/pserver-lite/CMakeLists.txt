cmake_minimum_required(VERSION 3.5)

add_subdirectory( unit_tests )

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


##### pserver-lite (executable) #################

add_executable( pserver-lite
  
  DESTINATION ${BIN_INSTALL_DIR}
)


##### szcache_test (executable) #################

add_executable( szcache_test
  szcache.cpp szcache_test.cpp shm_connection.cc
  DESTINATION ${BIN_INSTALL_DIR}
)
