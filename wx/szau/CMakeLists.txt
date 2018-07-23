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


##### szau (executable) #########################

add_executable( szau
  @srcdir@/szauapp.cpp @srcdir@/szauapp.h @srcdir@/downloader.cpp
    @srcdir@/downloader.h @srcdir@/updater.cpp @srcdir@/updater.h
    @srcdir@/szautaskbaritem.cpp @srcdir@/szautaskbaritem.h
  DESTINATION ${BIN_INSTALL_DIR}
)
