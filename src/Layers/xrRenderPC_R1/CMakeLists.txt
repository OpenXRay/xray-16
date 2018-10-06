project(xrRenderPC_R1)

list(APPEND DIRS
	"."
	)


add_dir("${DIRS}")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../..
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Externals/luabind
	${CMAKE_CURRENT_SOURCE_DIR}/../../../sdk/include
    ${SDL_INCLUDE_DIRS}
	)

#list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./LevelCompilerLoggerWindow.cpp")
#list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./LevelCompilerLoggerWindow.hpp")

add_definitions(-DXRRENDER_R1_EXPORTS)
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore luabind LuaJIT xrCDB xrEngine xrParticles xrScriptEngine xrAPI xrMiscMath  ${SDL_LIBRARIES})
xr_install(${PROJECT_NAME})

