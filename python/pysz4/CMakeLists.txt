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


##### pysz4 (shared) ############################

add_library( pysz4 SHARED
  pysz4.cc
  VERSION 0.0.0
  LINK Szarp-shared Szarp2-shared
  DESTINATION ${LIB_INSTALL_DIR}
)
