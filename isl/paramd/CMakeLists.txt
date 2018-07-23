add_definitions(
  -DHAVE_CONFIG_H
  -DPREFIX="/opt/szarp"
)


##### paramd (executable) #######################

add_executable( paramd
  ipk2xml.cc param_tree.cc
    param_value_functor.cc
    tree_processor.cc paramd.cc ptt2xml.cc
    svg_generator.cc
)

target_include_directories( paramd
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${CMAKE_SOURCE_DIR}/libSzarp/include
        ${CMAKE_SOURCE_DIR}/libSzarp2/include
        ${CMAKE_SOURCE_DIR}/isl/server/include
        /usr/include/libxml2
        /usr/include/lua5.1
)

target_link_libraries( paramd
  Szarp
  Szarp2
  server
  Threads::Threads
  xml2
  luajit-5.1
  pam
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${PROTOBUF_LIBRARY}
  boost_system
  boost_filesystem
  protobuf
  boost_date_time
  boost_thread
  boost_locale
  ssl
  crypto
  systemd
)
