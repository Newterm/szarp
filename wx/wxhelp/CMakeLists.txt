file(GLOB SOURCES *.cpp)

foreach(def ${wxWidgets_DEFINITIONS})
 add_definitions(
	 -D${def}
 )
endforeach()

add_executable(wxhelp
  ${SOURCES}
)

target_include_directories(wxhelp
  PRIVATE
    ${CMAKE_SOURCE_DIR}/wx/common
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${wxWidgets_INCLUDE_DIRS}
)

target_link_libraries(wxhelp
  PRIVATE
    Szarp
    Szarp2
    SzarpLuaParser
    wxcommon
    #wxscintilla
    #iks-client
    #${Boost_LIBRARIES}
    #${PYTHON_LIBRARIES}
    #${CURL_LIBRARIES}
    #${LIBXML2_LIBRARIES}
    #${LUA_LIBRARIES}
    #${PROTOBUF_LIBRARIES}
    #${Zip_LIBRARIES}
    #${LIBXSLT_LIBRARIES}
    #python2.7
    #${wxWidgets_LIBRARIES}
    #${GTK2_LIBRARIES}
    #${OPENGL_LIBRARIES}
    #${SQLITE3_LIBRARIES}
    #${OPENSSL_LIBRARIES}
    #${CARES_LIBRARIES}
    #${XMLRPCEPI_LIBRARIES}
    #${FTGL_LIBRARIES}
)
