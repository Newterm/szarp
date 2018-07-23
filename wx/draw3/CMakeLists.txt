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


##### draw3 (executable) ########################

add_executable( draw3
  classes.h draw3.cpp dbmgr.cpp dbmgr.h database.h database.cpp base_handler.h
    base_handler.cpp disptime.cpp disptime.h drawpnl.cpp drawpnl.h drawswdg.cpp
    drawswdg.h drawsctrl.h drawsctrl.cpp drawapp.cpp drawapp.h draw.cpp draw.h
    coobs.h coobs.cpp cfgmgr.cpp cfgmgr.h ids.h infowdg.cpp infowdg.h
    drawview.h drawview.cpp seldraw.cpp seldraw.h selset.cpp selset.h
    timewdg.cpp timewdg.h drawtb.cpp drawtb.h incsearch.cpp incsearch.h
    drawobs.h drawobs.cpp summwin.h summwin.cpp drawfrm.h drawfrm.cpp frmmgr.h
    frmmgr.cpp timeformat.h timeformat.cpp defcfg.h defcfg.cpp drawpick.h
    drawpick.cpp drawprint.h drawprint.cpp piewin.cpp piewin.h relwin.cpp
    relwin.h xydiag.cpp xydiag.h xygraph.cpp xygraph.h dbinquirer.cpp
    dbinquirer.h statdiag.h statdiag.cpp drawdnd.h drawdnd.cpp progfrm.h
    progfrm.cpp remarks.h remarks.cpp remarksdialog.h remarksdialog.cpp
    remarksfetcher.h remarksfetcher.cpp splashscreen.cpp splashscreen.h
    drawipc.cpp drawipc.h drawurl.cpp drawurl.h errfrm.h errfrm.cpp
    paramslist.cpp paramslist.h paredit.cpp paredit.h codeeditor.cpp
    codeeditor.h drawpsc.h drawpsc.cpp wxgraphs.h wxgraphs.cpp glgraphs.h
    glgraphs.cpp gcdcgraphs.h gcdcgraphs.cpp graphsutils.h graphsutils.cpp
    drawtime.h drawtime.cpp vercheck.h vercheck.cpp xyzgraph.h xyzgraph.cpp
    triangulation.h triangulation.cpp probadddiag.h probadddiag.cpp
    drawtreedialog.h drawtreedialog.cpp delqitem.h delqitem.cpp seteditctrl.h
    parameditctrl.h ikssetup.h ikssetup.cpp GUI/resource.cpp
  DESTINATION ${BIN_INSTALL_DIR}
)
