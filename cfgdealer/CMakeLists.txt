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
)


##### cfgdealer (executable) ####################

add_executable( cfgdealer
  cfgdealer.cpp
  DESTINATION ${BIN_INSTALL_DIR}
)


##### cfgdealer_test (executable) ###############

add_executable( cfgdealer_test
  cfgdealer_test.cpp
  DESTINATION ${BIN_INSTALL_DIR}
)
