project(xrEngine)

list(APPEND DIRS
	"."
	)

add_dir("${DIRS}")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/OpenAutomate/inc
	${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/luabind
    ${SDL_INCLUDE_DIR}
	)

list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./tntQAVI.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./editor_environment_manager_properties.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./Text_Console.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./Text_Console_WndProc.cpp")

list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./tntQAVI.h")
list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./Text_Console.h")

add_definitions(-DENGINE_BUILD)
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore luabind xrAPI xrSound xrScriptEngine luajit theora ${OGG_LIBRARIES} xrCDB xrPhysics openal lockfile ${SDL_LIBRARIES})
