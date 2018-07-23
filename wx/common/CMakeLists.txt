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


##### wxcommon (static) #########################

add_library( wxcommon STATIC
  @srcdir@/version.h @srcdir@/cconv.cc @srcdir@/cconv.h
    @srcdir@/datechooser.cpp @srcdir@/datechooser.h @srcdir@/dcolors.cc
    @srcdir@/dcolors.h @srcdir@/fetchparams.cc @srcdir@/fetchparams.h
    @srcdir@/fonts.cpp @srcdir@/fonts.h @srcdir@/geometry.cpp
    @srcdir@/geometry.h @srcdir@/htmlabout.cpp @srcdir@/htmlabout.h
    @srcdir@/htmlview.cpp @srcdir@/htmlview.h @srcdir@/parlist.cpp
    @srcdir@/parlist.h @srcdir@/parselect.cpp @srcdir@/parselect.h
    @srcdir@/serverdlg.cpp @srcdir@/serverdlg.h @srcdir@/aboutdlg.cpp
    @srcdir@/aboutdlg.h @srcdir@/imagepanel.cpp @srcdir@/imagepanel.h
    @srcdir@/szapp.cpp @srcdir@/szapp.h @srcdir@/szhlpctrl.cpp
    @srcdir@/szhlpctrl.h @srcdir@/szmutex.cpp @srcdir@/szmutex.h
    @srcdir@/ipc.cc @srcdir@/ipc.h @srcdir@/cfgnames.cpp @srcdir@/cfgnames.h
    @srcdir@/balloontaskbaritem.cpp @srcdir@/balloontaskbaritem.h
    @srcdir@/authdiag.cpp @srcdir@/authdiag.h @srcdir@/pscgui.cpp
    @srcdir@/pscgui.h @srcdir@/biowxsock.h @srcdir@/biowxsock.cpp
    @srcdir@/cfgdlg.h @srcdir@/cfgdlg.cpp @srcdir@/szframe.h
    @srcdir@/szframe.cpp @srcdir@/sztaskbaritem.h @srcdir@/sztaskbaritem.cpp
    @srcdir@/singleinstance.h @srcdir@/singleinstance.cc @srcdir@/md5.h
    @srcdir@/md5.cpp @srcdir@/getprobersaddresses.h
    @srcdir@/getprobersaddresses.cpp @srcdir@/GUI/resource.cpp
    @srcdir@/helpctrlex/helpctrlex.cpp @srcdir@/helpctrlex/helpctrlex.h
    @srcdir@/helpctrlex/helpdlgex.cpp @srcdir@/helpctrlex/helpdlgex.h
    @srcdir@/helpctrlex/helpfrmex.cpp @srcdir@/helpctrlex/helpfrmex.h
    @srcdir@/helpctrlex/helpwinex.cpp @srcdir@/helpctrlex/helpwinex.h
    @srcdir@/validate.cpp @srcdir/validate.h
  LINK Szarp2-shared Szarp-shared
)
