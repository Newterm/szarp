############
# STANDARD #
############

cmake_minimum_required(VERSION 2.6)

add_custom_target(
	_wxscintilla ALL
	COMMAND make release
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/wxscintilla/build
)

