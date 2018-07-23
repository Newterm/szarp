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


##### kontroler3 (executable) ###################

add_executable( kontroler3
  kontr.cc kontr.h kontroler.cc kontroler.h kontradd.cc kontradd.h parcalc.cc
    parcalc.h kontropt.cc kontropt.h kontralarm.h kontralarm.cc
  DESTINATION ${BIN_INSTALL_DIR}
)
