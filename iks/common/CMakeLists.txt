file(GLOB SOURCES data/*.cpp utils/*.cpp)

add_library(iks-common SHARED
  ${SOURCES}
)

target_include_directories(iks-common
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

#target_link_library(iks-common)
