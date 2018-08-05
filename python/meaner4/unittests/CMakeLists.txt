file(GLOB PYSOURCE *.py)

add_custom_target(unittests
  COMMAND ${CMAKE_COMMAND} -E env "PYTHONPATH=${CMAKE_SOURCE_DIR}/python/protobuf:${CMAKE_CURRENT_SOURCE_DIR}/.." ${NOSETESTS_PATH}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Running meaner4 unittests."
)

set(COPY_TARGET unittests_copy)

add_custom_target(${COPY_TARGET})

add_custom_command(TARGET ${COPY_TARGET}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${COPY_DIR}
)

foreach(pysource ${PYSOURCE})
  add_custom_command(TARGET ${COPY_TARGET}
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/${pysource} ${COPY_DIR}
  )
endforeach()
