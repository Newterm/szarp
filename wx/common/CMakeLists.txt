file(GLOB_RECURSE SOURCES *.cc *.cpp)
file(GLOB_RECURSE HEADERS *.h)

foreach(def ${wxWidgets_DEFINITIONS})
  add_definitions(
    -D${def}
  )
endforeach()
add_definitions(
  -DSCI_LEXER -DLINK_LEXERS -DPREFIX=\"/opt/szarp/\"
)

add_library(wxcommon SHARED
  ${SOURCES} ${HEADERS}
)

target_include_directories(wxcommon
  PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
    ${wxWidgets_INCLUDE_DIRS}
    ${GTK2_INCLUDE_DIRS}
)

target_link_libraries(wxcommon
  PUBLIC
    Szarp
    Szarp2
    ${OpenSSL_LIBRARIES}
)
