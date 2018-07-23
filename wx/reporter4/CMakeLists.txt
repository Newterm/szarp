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


##### reporter4 (executable) ####################

add_executable( reporter4
  repapp.cpp reporter_view.cpp reporter_view.h reporter_controller.hpp
    reporter_controller.h report_edit.cpp report_edit.h userreports.cpp
    userreports.h report.h report.cpp data_provider.cpp config_handler.cpp
    $(ICONS)
  DESTINATION ${BIN_INSTALL_DIR}
)
