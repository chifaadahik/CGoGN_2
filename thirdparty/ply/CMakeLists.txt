set(CGOGN_THIRDPARTY_PLY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Ply include directory")

project(ply
	LANGUAGES C
)

cgogn_reset_warning_level()
set(HEADER_FILES
	ply.h
)

set(SOURCE_FILES
	ply.c
)


add_library(${PROJECT_NAME} STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")

install(FILES "ply.h" DESTINATION "include/cgogn/thirdparty")

cgogn_create_package("${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" "include/cgogn/thirdparty")
