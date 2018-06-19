project(xrQSlim)

list(APPEND DIRS
	"src"
	)

add_dir("${DIRS}")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../.. 
	${CMAKE_CURRENT_SOURCE_DIR}/../../../sdk/include
	)

#list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./LevelCompilerLoggerWindow.cpp")
#ist(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./LevelCompilerLoggerWindow.hpp")

add_definitions(-DXR_QSLIM_EXPORTS)
add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})
