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


##### raporter3 (executable) ####################

add_executable( raporter3
  rap.cc raporter.cc raporter.h raporterperiod.cc raporterperiod.h
    raporteredit.cc raporteredit.h filedump.cc raporteredit.h userreports.cc
    userreports.h ns.h $(ICONS)
  DESTINATION ${BIN_INSTALL_DIR}
)
