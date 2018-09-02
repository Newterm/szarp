file(GLOB_RECURSE SOURCES *.cpp *.cxx)

foreach(def ${wxWidgets_DEFINITIONS})
  add_definitions(
    -D${def}
  )
endforeach()
add_definitions(
  -DSCI_LEXER -DGTK -DLINK_LEXERS
)

add_library(wxscintilla SHARED
  ${SOURCES}
)

target_include_directories(wxscintilla
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${wxWidgets_INCLUDE_DIRS}
)
