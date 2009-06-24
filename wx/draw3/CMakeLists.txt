############
# STANDARD #
############

INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})

INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/SetDirs)

###########
# SOURCES #
###########

PROJECT(DRAW3)

ADD_EXECUTABLE(draw3
		cfgmgr.cpp 
		coobs.cpp 
		database.cpp 
		dbinquirer.cpp
		defcfg.cpp 
		disptime.cpp 
		drawdnd.cpp 
		drawfrm.cpp 
		drawipc.cpp
		drawobs.cpp 
		drawpick.cpp 
		drawpnl.cpp
		drawprint.cpp 
		drawtb.cpp
		drawurl.cpp
		drawview.cpp 
		frmmgr.cpp 
		GUI/resource.cpp
		incsearch.cpp
		infowdg.cpp
		piewin.cpp
		progfrm.cpp 
		relwin.cpp 
		remarks.cpp 
		splashscreen.cpp
		statdiag.cpp 
		summwin.cpp 
		timeformat.cpp 
		xydiag.cpp
		xygraph.cpp
                dbmgr.cpp 
                draw3.cpp 
                drawapp.cpp 
                draw.cpp
                drawswdg.cpp
                seldraw.cpp
                selset.cpp
                timewdg.cpp
)

ADD_CUSTOM_COMMAND(OUTPUT
		GUI/resource.cpp
		COMMAND wxrc -c -o GUI/resource.cpp GUI/*xrc
)

ADD_DEFINITIONS(
		-DNO_GSTREAMER
)

INCLUDE_DIRECTORIES(
	./
)

##############################
# PACKAGES AND CONFIGURATION #
##############################

FIND_PACKAGE(libSzarp REQUIRED)
INCLUDE_DIRECTORIES(${LIBSZARP_INCLUDE_DIRS})
LINK_DIRECTORIES(${LIBSZARP_LIBRARY_DIRS})
TARGET_LINK_LIBRARIES(draw3 ${LIBSZARP_LIBRARIES})

FIND_PACKAGE(libSzarp2 REQUIRED)
INCLUDE_DIRECTORIES(${LIBSZARP2_INCLUDE_DIRS})
#LINK_DIRECTORIES(${LIBSZARP2_LIBRARY_DIRS})
TARGET_LINK_LIBRARIES(draw3 ${LIBSZARP2_LIBRARIES})

FIND_PACKAGE(wxcommon REQUIRED)
INCLUDE_DIRECTORIES(${WXCOMMON_INCLUDE_DIRS})
LINK_DIRECTORIES(${WXCOMMON_LIBRARY_DIRS})
TARGET_LINK_LIBRARIES(draw3 ${WXCOMMON_LIBRARIES})

FIND_PACKAGE(wxscintilla REQUIRED)
INCLUDE_DIRECTORIES(${WXSCINTILLA_INCLUDE_DIRS})
MESSAGE(DDDDUPA ${WXSCINTILLA_LIBRARY_DIRS})

LINK_DIRECTORIES(${WXSCINTILLA_LIBRARY_DIRS})
TARGET_LINK_LIBRARIES(draw3 ${WXSCINTILLA_LIBRARIES})

INCLUDE(FindLua51)
INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})

INCLUDE(FindLibXml2)
INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
ADD_DEFINITIONS(${LIBXML2_DEFINITIONS})
TARGET_LINK_LIBRARIES(draw3 ${LIBXML2_LIBRARIES})

SET(Boost_USE_STATIC_LIBS ON)
FIND_PACKAGE(Boost 1.34.1 COMPONENTS date_time filesystem threads )

SET(wxWidgets_USE_UNICODE ON)
FIND_PACKAGE(wxWidgets REQUIRED base core xrc net gl html aui)
INCLUDE(${wxWidgets_USE_FILE})
TARGET_LINK_LIBRARIES(draw3 ${wxWidgets_LIBRARIES})

FIND_PACKAGE(PkgConfig REQUIRED)

pkg_check_modules(GDK2 REQUIRED gdk-2.0)

INCLUDE_DIRECTORIES(${GDK2_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(draw3 ${GDK2_LIBRARIES})

pkg_check_modules(SQLITE3 REQUIRED sqlite3)

pkg_check_modules(LIBSSL REQUIRED libssl)

pkg_check_modules(VMIME REQUIRED vmime)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

ADD_DEPENDENCIES(draw3 RESOURCESCPP)

