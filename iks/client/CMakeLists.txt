file(GLOB SOURCES *.cpp)

add_library(iks-client SHARED
  ${SOURCES}
)

target_include_directories(iks-client
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/iks/common
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(iks-client
  iks-common
)
