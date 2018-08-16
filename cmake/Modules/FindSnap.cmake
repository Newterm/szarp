# Searches for an installation of the snap library. On success, it sets the following variables:
#
#   SNAP_FOUND              Set to true to indicate the snap library was found
#   SNAP_INCLUDE_DIRS       The directory containing the header file snap/snap.h
#   SNAP_LIBRARIES          The libraries needed to use the snap library
#
# To specify an additional directory to search, set SNAP_ROOT.

SET(SNAP_INCLUDE_DOC "The directory containing the header file snap7.h")
FIND_PATH(SNAP_INCLUDE_DIRS NAMES snap7.h PATHS ${SNAP_ROOT} ${SNAP_ROOT}/include DOC ${SNAP_INCLUDE_DOC} NO_DEFAULT_PATH)
IF(NOT SNAP_INCLUDE_DIRS)  # now look in system locations
  FIND_PATH(SNAP_INCLUDE_DIRS NAMES snap7.h DOC ${SNAP_INCLUDE_DOC})
ENDIF(NOT SNAP_INCLUDE_DIRS)

SET(SNAP_FOUND FALSE)

IF(SNAP_INCLUDE_DIRS)
  SET(SNAP_LIBRARY_DIRS ${SNAP_INCLUDE_DIRS})

  IF("${SNAP_LIBRARY_DIRS}" MATCHES "/include$")
    # Strip off the trailing "/include" in the path.
    GET_FILENAME_COMPONENT(SNAP_LIBRARY_DIRS ${SNAP_LIBRARY_DIRS} PATH)
  ENDIF("${SNAP_LIBRARY_DIRS}" MATCHES "/include$")

  IF(EXISTS "${SNAP_LIBRARY_DIRS}/lib")
    SET(SNAP_LIBRARY_DIRS ${SNAP_LIBRARY_DIRS}/lib)
  ENDIF(EXISTS "${SNAP_LIBRARY_DIRS}/lib")

  # Find SNAP libraries
  FIND_LIBRARY(SNAP_RELEASE_LIBRARY NAMES snap7 libsnap7
               PATH_SUFFIXES Release ${CMAKE_LIBRARY_ARCHITECTURE} ${CMAKE_LIBRARY_ARCHITECTURE}/Release
               PATHS ${SNAP_LIBRARY_DIRS} NO_DEFAULT_PATH)

  SET(SNAP_LIBRARIES )
  IF(SNAP_RELEASE_LIBRARY)
    SET(SNAP_LIBRARIES ${SNAP_RELEASE_LIBRARY})
  ENDIF(SNAP_RELEASE_LIBRARY)

  IF(SNAP_LIBRARIES)
    SET(SNAP_FOUND TRUE)
  ENDIF(SNAP_LIBRARIES)
ENDIF(SNAP_INCLUDE_DIRS)

IF(SNAP_FOUND)
  IF(NOT SNAP_FIND_QUIETLY)
    MESSAGE(STATUS "Found SNAP: headers at ${SNAP_INCLUDE_DIRS}, libraries at ${SNAP_LIBRARY_DIRS}")
  ENDIF(NOT SNAP_FIND_QUIETLY)
ELSE(SNAP_FOUND)
  IF(SNAP_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "SNAP library not found")
  ENDIF(SNAP_FIND_REQUIRED)
ENDIF(SNAP_FOUND)
