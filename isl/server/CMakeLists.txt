add_definitions(
  -DPREFIX=\"@prefix@\"
)

link_libraries( ${OPENSSL_LIBRARIRES} )

##### server (static) ###########################

add_library(server SHARED
  access.cc auth_server.cc base64.cc
  config_load.cc pam.cc server.cc
  ssl_server.cc utf8.cc
)

target_include_directories(server
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    /usr/include/libxml2
)
