project(xrGame)

list(APPEND DIRS
    "."
    "ai"
    "ik"
    "ui"
    )

add_dir("${DIRS}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../xrServerEntities
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/luabind
    ${SDL_INCLUDE_DIRS}
    )

list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ui/UIListWnd.h")

list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/ui/UIListWnd.h")

add_definitions(-DXRGAME_EXPORTS -D_USRDLL)
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore xrEngine xrMiscMath luabind xrAPI xrAICore xrSound xrScriptEngine cximage ${LUA_LIBRARIES} pugixml xrCDB xrPhysics ${SDL_LIBRARIES})
