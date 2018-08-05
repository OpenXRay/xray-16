project(xrLC_Light)

list(APPEND DIRS
	"."
	)

add_dir("${DIRS}")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../.. 
	${CMAKE_CURRENT_SOURCE_DIR}/../../../sdk/include
	)

list(APPEND ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../Shader_xrLC.h")
list(APPEND ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../../xrEngine/xrLoadSurface.cpp")
#list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./LevelCompilerLoggerWindow.cpp")
#list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./LevelCompilerLoggerWindow.hpp")

add_definitions(-D_USRDLL -DLEVEL_COMPILER -D_USE_MATH_DEFINES -DXRLC_LIGHT_EXPORTS -DFORCE_NO_EXCEPTIONS -DNO_XR_VDECLARATOR)
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore xrCDB zlib xrLCUtil)
