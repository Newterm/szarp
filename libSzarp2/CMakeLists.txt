############
# STANDARD #
############

INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/SetDirs)

###########
# SOURCES #
###########

PROJECT(libSzarp2)

ADD_DEFINITIONS(
	-DHAVE_GMTIME_R
	-DHAVE_LOCALTIME_R
	-DHAVE_TIMEGM
)

INCLUDE_DIRECTORIES(
	include/
	include/szbase
	./
)

ADD_LIBRARY(Szarp2 STATIC
	ipk/dmncfg.cc
	ipk/tdevice.cc
	ipk/tdraw.cc
	ipk/tparam.cc
	ipk/tradio.cc
	ipk/traport.cc
	ipk/tsendparam.cc
	ipk/tszarpconfig.cc
	ipk/tunit.cc
	ipk/tvalue.cc
	ipk/tboiler.cc
	ipk/tseason.cc
	ipk/tanalysis.cc
	ipk/tainterval.cc
	ipk/ipkcontainer.cc
	linedmn/ipchandler.cc
	linedmn/mbus.cc
	misc/convuri.c
	misc/crontime.cc
	misc/http.cc
	misc/insort.c
	misc/latin2.c
	misc/mingw32_missing.c
	misc/tokens.c
	misc/xmlutils.c
	misc/xscale.cc

	szbextr/extractor.cc

	parsers/ekrn_parser.c
	
	lex.rap.cc
	raporter.tab.cc
	lex.ptp.c
	ptt_act.tab.c
	lex.pcfg.c
	parcook_cfg.tab.c
	lex.lcfg.c
	line_cfg.tab.c
	lex.scfg.c
	sender_cfg.tab.c
	lex.def.cc
	definable.tab.cc 

	szbase/szbbase.cc
	szbase/szbbuf.cc
	szbase/szbdate.cc
	szbase/szbfile.cc
	szbase/szbname.cc
	szbase/definabledatablock.cc
	szbase/realdatablock.cc
	szbase/combineddatablock.cc
	szbase/cacheabledatablock.cc
	szbase/luadatablock.cc
	szbase/szbhash.cc
	szbase/szbdatablock.cc
)

##############################
# PACKAGES AND CONFIGURATION #
##############################

FIND_PACKAGE(libSzarp REQUIRED)
INCLUDE_DIRECTORIES(${LIBSZARP_INCLUDE_DIRS})

INCLUDE(FindLua51)
INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})

INCLUDE(FindLibXml2)
INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

FIND_PACKAGE(Flex  REQUIRED)

FIND_PACKAGE(Bison  REQUIRED)

ADD_CUSTOM_COMMAND(OUTPUT
		lex.rap.cc
		COMMAND ${FLEX_EXECUTABLE} ARGS -X -Prap -+ -o lex.rap.cc parsers/raporter.lex
		DEPENDS parsers/raporter.lex raporter.tab.h
)

ADD_CUSTOM_COMMAND(OUTPUT
		raporter.tab.cc raporter.tab.h
		COMMAND ${BISON_EXECUTABLE} ARGS -p rap --defines=raporter.tab.h -oraporter.tab.cc parsers/raporter.y
		DEPENDS parsers/raporter.y
)

ADD_CUSTOM_COMMAND(OUTPUT
		lex.ptp.c
		COMMAND ${FLEX_EXECUTABLE} ARGS -Pptp -olex.ptp.c parsers/ptt_act.lex
		DEPENDS parsers/ptt_act.lex ptt_act.tab.h
)

ADD_CUSTOM_COMMAND(OUTPUT
		ptt_act.tab.c ptt_act.tab.h
		COMMAND ${BISON_EXECUTABLE} ARGS -p ptp -d -optt_act.tab.c parsers/ptt_act.y
		DEPENDS parsers/ptt_act.y
)
	
ADD_CUSTOM_COMMAND(OUTPUT
		lex.pcfg.c
		COMMAND ${FLEX_EXECUTABLE} ARGS -Ppcfg -olex.pcfg.c parsers/parcook_cfg.lex
		DEPENDS parsers/parcook_cfg.lex parcook_cfg.tab.h
)

ADD_CUSTOM_COMMAND(OUTPUT
		parcook_cfg.tab.c parcook_cfg.tab.h
		COMMAND ${BISON_EXECUTABLE} ARGS -p pcfg -d -oparcook_cfg.tab.c parsers/parcook_cfg.y
		DEPENDS parsers/parcook_cfg.y
)

ADD_CUSTOM_COMMAND(OUTPUT
		lex.lcfg.c
		COMMAND ${FLEX_EXECUTABLE} ARGS -Plcfg -olex.lcfg.c parsers/line_cfg.lex
		DEPENDS parsers/line_cfg.lex line_cfg.tab.h
)

ADD_CUSTOM_COMMAND(OUTPUT
		line_cfg.tab.c line_cfg.tab.h
		COMMAND ${BISON_EXECUTABLE} ARGS -p lcfg -d -oline_cfg.tab.c parsers/line_cfg.y
		DEPENDS parsers/line_cfg.y
)

ADD_CUSTOM_COMMAND(OUTPUT
		lex.scfg.c
		COMMAND ${FLEX_EXECUTABLE} ARGS -Pscfg -olex.scfg.c parsers/sender_cfg.lex
		DEPENDS parsers/sender_cfg.lex sender_cfg.tab.h
)

ADD_CUSTOM_COMMAND(OUTPUT
		sender_cfg.tab.c sender_cfg.tab.h
		COMMAND ${BISON_EXECUTABLE} ARGS -p scfg -d -osender_cfg.tab.c parsers/sender_cfg.y
		DEPENDS parsers/sender_cfg.y
)

ADD_CUSTOM_COMMAND(OUTPUT
		lex.def.cc
		COMMAND ${FLEX_EXECUTABLE} ARGS -Pdef -+ -olex.def.cc parsers/definable.lex
		DEPENDS parsers/definable.lex definable.tab.h
)

ADD_CUSTOM_COMMAND(OUTPUT
		definable.tab.cc definable.tab.h
		COMMAND ${BISON_EXECUTABLE} ARGS -p def --defines=definable.tab.h -odefinable.tab.cc parsers/definable.y
		DEPENDS parsers/definable.y
)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

