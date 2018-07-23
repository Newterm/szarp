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


##### scc (executable) ##########################

add_executable( scc
  @srcdir@/scc.cpp @srcdir@/sccapp.cpp @srcdir@/sccapp.h
    @srcdir@/sccreloader.cpp @srcdir@/sccreloader.h @srcdir@/sccframe.cpp
    @srcdir@/sccframe.h @srcdir@/sccipc.cpp @srcdir@/sccipc.h
    @srcdir@/sccmenu.cpp @srcdir@/sccmenu.h @srcdir@/scctunnelframe.cpp
    @srcdir@/scctunnelframe.h @srcdir@/szarpconfigs.cpp @srcdir@/szarpconfigs.h
    @srcdir@/szarpfonts.cpp @srcdir@/szarpfonts.h
    @srcdir@/scchideselectionframe.cpp @srcdir@/scchideselectionframe.h
  DESTINATION ${BIN_INSTALL_DIR}
)
