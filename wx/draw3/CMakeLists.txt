file(GLOB_RECURSE SOURCES *.cpp)

foreach(def ${wxWidgets_DEFINITIONS})
 add_definitions(
	 -D${def}
 )
endforeach()
add_definitions(
 -DSCI_LEXER -DLINK_LEXERS -DPREFIX=\"/opt/szarp/\"
)

WXWIDGETS_ADD_RESOURCES(SOURCES GUI/resources.xrc OPTIONS -c -o GUI/resource.cpp)

add_executable(draw3
  ${SOURCES}
)

target_include_directories(draw3
  PUBLIC
    ${CMAKE_SOURCE_DIR}/wx/common
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${CMAKE_SOURCE_DIR}/iks/client/include
    ${CMAKE_SOURCE_DIR}/extern/wxscintilla/include
    ${wxWidgets_INCLUDE_DIRS}
    ${GTK2_INCLUDE_DIRS}
		${LIBXML2_INCLUDE_DIR}
		${LUA_INCLUDE_DIR}
)

target_link_libraries(draw3
  Szarp
  Szarp2
	SzarpLuaParser
  wxscintilla
  iks-client
  wxcommon
	${Boost_LIBRARIES}
	${PYTHON_LIBRARIES}
	${CURL_LIBRARIES}
	${LIBXML2_LIBRARIES}
	${LUA_LIBRARIES}
	${PROTOBUF_LIBRARIES}
	${Zip_LIBRARIES}
	${LIBXSLT_LIBRARIES}
	python2.7
	${wxWidgets_LIBRARIES}
  ${GTK2_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${CARES_LIBRARIES}
  ${XMLRPCEPI_LIBRARIES}
  ${FTGL_LIBRARIES}
)
