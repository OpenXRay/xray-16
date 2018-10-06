project(xrEngine)

list(APPEND DIRS
    "."
    "StringTable"
    )

add_dir("${DIRS}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/OpenAutomate/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/luabind
    ${SDL_INCLUDE_DIRS}
    ${FREEIMAGE_INCLUDE_PATH}
    ${LOCKFILE_INCLUDE_DIR}
    )

list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./tntQAVI.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./editor_environment_manager_properties.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./Text_Console.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./Text_Console_WndProc.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./Device_wndproc.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./xrSASH.cpp")

list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./tntQAVI.h")
list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./Text_Console.h")
list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./xrSASH.h")

add_definitions(-DENGINE_BUILD)
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "stdafx.h")
cotire(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore xrMiscMath luabind xrAPI xrSound xrScriptEngine ${LUA_LIBRARIES} ${THEORA_LIBRARY} ${OGG_LIBRARIES} xrCDB xrPhysics ${FREEIMAGE_LIBRARY} openal ${LOCKFILE_LIBRARIES} ${SDL_LIBRARIES})
xr_install(${PROJECT_NAME})

