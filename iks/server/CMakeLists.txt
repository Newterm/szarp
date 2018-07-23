cmake_minimum_required(VERSION 3.5)

add_subdirectory( data )
add_subdirectory( net )
add_subdirectory( locations )
add_subdirectory( utils )

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
  -DBOOST_ENABLE_ASSERT_HANDLER
)


##### iks-server (executable) ###################

add_executable( iks-server
  
  DESTINATION ${BIN_INSTALL_DIR}
)
