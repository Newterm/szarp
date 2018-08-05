file(GLOB SOURCES *.cc)

add_definitions(
  -DPREFIX=\"@prefix@\"
)

link_libraries( ${OPENSSL_LIBRARIRES} )

add_library(server SHARED
  ${SOURCES}
)

target_include_directories(server
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    /usr/include/libxml2
)

target_link_libraries(server
  ${OPENSSL_LIBRARIES}
)
