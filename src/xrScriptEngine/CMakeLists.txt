project(xrScriptEngine)

list(APPEND DIRS
	"."
	"LuaStudio"
	)

add_dir("${DIRS}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/luabind ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include )

add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrAPI xrCore luabind luajit)
