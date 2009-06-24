############
# STANDARD #
############

INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/SetDirs)

##############################
# PACKAGES AND CONFIGURATION #
##############################

FIND_PACKAGE(Flex  REQUIRED)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

###########
# SOURCES #
###########

PROJECT(libSzarp)

INCLUDE_DIRECTORIES(
	include
	./
)


ADD_CUSTOM_COMMAND(OUTPUT
		libparnt.yy.c
		COMMAND ${FLEX_EXECUTABLE} ARGS -olibparnt.yy.c libparnt.lex
		DEPENDS libparnt.lex include/libpar_int.h include/libparnt.parse.h
)

ADD_LIBRARY(Szarp
	libparnt.yy.c
	libparnt.c
	libparnt.parse.c
	liblog.c
	xs_help.c
	xs_menu.c
	xs_pix.c
	xs_quit.c
	xs_str.c
	icon.c
	date.c
	msgerror.c
	msgtools.c
	daemon.c
	execute.c
	help.c
	passwd.c
	mbrtu.c
)
