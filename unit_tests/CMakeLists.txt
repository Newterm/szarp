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
)


##### unit_tests (executable) ###################

add_executable( unit_tests
  unit_tests.cpp sz4_block_unit_test.cpp parhublistener_test.cpp
    szb_param_monitor_unit_test.cpp sz4_file_search.cpp
    sz4_buffer_unit_test.cpp sz4_factory_test.cpp sz4_base_test.cpp
    sz4_definable_param_cache.cpp test_observer.h test_observer.cpp
    base_param_converter_unit_test.cpp sz4_lua_param_optimized.cpp
    sz4_lua_param.cpp sz4_combined_param.cpp sz4_definable_param.cpp
    sz4_wsum.cpp sz4_live_cache.cpp sz4_decode_test.cpp filelogger_test.cpp
    loghandler_test.cpp zmq_handler_test.cpp cmdlineparser_test.cpp
    argsmgr_test.cpp simple_mocks.h
  DESTINATION ${BIN_INSTALL_DIR}
)


##### sz4_extr_simple (executable) ##############

add_executable( sz4_extr_simple
  sz4_extr_simple.cpp
  DESTINATION ${BIN_INSTALL_DIR}
)
