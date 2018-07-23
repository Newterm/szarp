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


##### ekstraktor3 (executable) ##################

add_executable( ekstraktor3
  ekstraktor3.cpp ids.h EkstraktorWidget.cpp EkstraktorWidget.h
    EkstraktorMainWindow.cpp EkstraktorMainWindow.h extraction.cpp extraction.h
  DESTINATION ${BIN_INSTALL_DIR}
)
