file(GLOB SOURCES *.cc)

add_definitions(
  -DPREFIX=\"@prefix@\"
)

add_library(pyszbase SHARED
  ${SOURCES}
)

target_include_directories(pyszbase
  PUBLIC
    ${PYTHON_INCLUDE_DIRS}
)

target_link_libraries(pyszbase
  Szarp
  Szarp2
)
