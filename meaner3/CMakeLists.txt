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


##### meaner3 (executable) ######################

add_executable( meaner3
  meaner3.cc meaner3.h tmeaner.cc tmeaner.h tparcook.cc tparcook.h
    tsaveparam.cc tsaveparam.h texecute.cc texecute.h texecsect.cc texecsect.h
    texecalarm.cc texecalarm.h tstatus.cc tstatus.h twriter.cc twriter.h
    classes.h
  DESTINATION ${BIN_INSTALL_DIR}
)


##### peakator (executable) #####################

add_executable( peakator
  utils/peakator.cc tsaveparam.cc tsaveparam.h tstatus.cc tstatus.h
  DESTINATION ${BIN_INSTALL_DIR}
)


##### prober (executable) #######################

add_executable( prober
  fileremover.cc tparcook.cc tsaveparam.cc twriter.cc tprober.cc tstatus.cc
    prober.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### szbedit (executable) ####################

add_executable( szbedit
  utils/szbedit.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### extrszb (executable) ####################

add_executable( extrszb
  utils/extraszb.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### szbindex (executable) #####################

add_executable( szbindex
  utils/szbindex.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### szbreader (executable) ####################

add_executable( szbreader
  utils/szbreader.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### szbwriter (executable) ####################

add_executable( szbwriter
  utils/szbwriter.cc utils/szbwriter_cache.cc utils/szbwriter_cache.h
    utils/tmmapparam.cc utils/tmmapparam.h tsaveparam.h tsaveparam.cc tstatus.h
    tstatus.cc
  DESTINATION ${BIN_INSTALL_DIR}
)


##### szbndremover (executable) #################

add_executable( szbndremover
  utils/szbndremover.cc tsaveparam.cc tsaveparam.h tstatus.cc tstatus.h
  DESTINATION ${BIN_INSTALL_DIR}
)


##### szbmod (executable) #######################

add_executable( szbmod
  utils/szbmod.cc tsaveparam.cc tsaveparam.h tstatus.cc tstatus.h
  DESTINATION ${BIN_INSTALL_DIR}
)


##### szbnull (executable) ######################

add_executable( szbnull
  utils/szbnull.cc
  DESTINATION ${BIN_INSTALL_DIR}
)
