add_definitions(
  -DHAVE_CONFIG_H
)

add_library(pysz4 SHARED
  pysz4.cc
)

target_include_directories(pysz4
  PUBLIC
    ${CMAKE_SOURCE_DIR}/libSzarp2/include
    ${CMAKE_SOURCE_DIR}/libSzarp/include
    ${PYTHON_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)
