
cmake_minimum_required(VERSION 2.6)

SET(PREFIX "\"/opt/szarp\"")
SET(PACKAGE_NAME "\"szarp\"")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

ADD_SUBDIRECTORY(CMakeModules)

ADD_SUBDIRECTORY(libSzarp)

ADD_SUBDIRECTORY(libSzarp2)

ADD_SUBDIRECTORY(extern)

ADD_SUBDIRECTORY(wx/common)

ADD_SUBDIRECTORY(wx/draw3)

