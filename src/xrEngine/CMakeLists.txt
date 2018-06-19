project(xrEngine)

list(APPEND DIRS
	"."
	)

add_dir("${DIRS}")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/OpenAutomate/inc
	${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/FreeImage/Dist/x64
	${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include
    /usr/include/SDL2
	)

list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./editor_environment_manager_properties.cpp")
#list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./xr_cda.h")

add_definitions(-DENGINE_BUILD)
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore luabind xrAPI xrSound xrScriptEngine luajit theora ${OGG_LIBRARIES} xrNetServer xrCDB xrPhysics openal)
