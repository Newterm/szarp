############
# STANDARD #
############

INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/SetDirs)

##############################
# PACKAGES AND CONFIGURATION #
##############################

FIND_PACKAGE(libSzarp REQUIRED)
INCLUDE_DIRECTORIES(${LIBSZARP_INCLUDE_DIRS})

FIND_PACKAGE(libSzarp2 REQUIRED)
INCLUDE_DIRECTORIES(${LIBSZARP2_INCLUDE_DIRS})

FIND_PACKAGE(wxcommon REQUIRED)

INCLUDE(FindLua51)
INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})

INCLUDE(FindLibXml2)
INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

SET(wxWidgets_USE_UNICODE ON)
FIND_PACKAGE(wxWidgets REQUIRED base core xrc net gl html aui)
INCLUDE(${wxWidgets_USE_FILE})

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/version.h)

###########
# SOURCES #
###########

PROJECT(Common)

INCLUDE_DIRECTORIES(
		../../include
		./
		${LIBSZARP_INCLUDE_DIR}
		${LIBSZARP2_INCLUDE_DIR}
		${LIBXML2_INCLUDE_DIR}
		${wxWidgets_CXX_FLAGS}
		${OPENSSL_INCLUDE_DIR}
		${VMIME_INCLUDE_DIR}
)

ADD_LIBRARY(wxcommon
		cconv.cc
                datechooser.cpp
                fetchparams.cc
                fonts.cpp
                geometry.cpp
                htmlabout.cpp
                htmlview.cpp
                parlist.cpp
                parselect.cpp
                serverdlg.cpp
                aboutdlg.cpp
                imagepanel.cpp
                szapp.cpp
                szhlpctrl.cpp
                szmutex.cpp
                ipc.cc
                cfgnames.cpp
		cfgdlg.cpp
		biowxsock.cpp
		pscgui.cpp
		authdiag.cpp
)

ADD_DEPENDENCIES(wxcommon version)
