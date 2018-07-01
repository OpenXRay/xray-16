project(xrRenderPC_GL)

list(APPEND DIRS
	"."
	)


add_dir("${DIRS}")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../..
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Externals/luabind
	${CMAKE_CURRENT_SOURCE_DIR}/../../../sdk/include
	)

#list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./LevelCompilerLoggerWindow.cpp")
#list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./LevelCompilerLoggerWindow.hpp")

add_definitions(-DXRLCUTIL_EXPORTS -D_USRDLL -DXRRENDER_GL_EXPORTS -DUSE_OGL -DNO_XR_VDECLARATOR)
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore luabind LuaJIT xrCDB xrEngine xrParticles xrScriptEngine xrAPI)
