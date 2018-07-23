#set(PREFIX libSzarp)

add_definitions(
  -DHAVE_CONFIG_H
)

## libparnt
FLEX_TARGET(libparnt libparnt.lex ${CMAKE_CURRENT_BINARY_DIR}/libparnt.yy.cc)

## libSzarp
add_library(Szarp SHARED
  ${FLEX_libparnt_OUTPUTS} libparnt.cc libparnt.parse.cc
  liblog.cpp argsmgr.cpp cmdlineparser.cpp
  loghandlers.cpp date.cc msgerror.cc msgtools.cc
  daemon.cc execute.cc mbrtu.cc
  conversion.cc ipctools.cc
)

set_property(TARGET Szarp PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(Szarp
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(Szarp boost_program_options systemd quadmath m)
