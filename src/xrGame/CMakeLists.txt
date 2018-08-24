project(xrGame)

list(APPEND DIRS
    "."
    "ai"
    "ai/crow"
    "ai/monsters"
    "ai/monsters/basemonster"
    "ai/monsters/bloodsucker"
    "ai/monsters/boar"
    "ai/monsters/burer"
    "ai/monsters/cat"
    "ai/monsters/chimera"
    "ai/monsters/controller"
    "ai/monsters/dog"
    "ai/monsters/flesh"
    "ai/monsters/fracture"
    "ai/monsters/group_states"
    "ai/monsters/poltergeist"
    "ai/monsters/pseudodog"
    "ai/monsters/pseudogigant"
    "ai/monsters/rats"
    "ai/monsters/snork"
    "ai/monsters/states"
    "ai/monsters/tushkano"
    "ai/monsters/zombie"
    "ai/phantom"
    "ai/stalker"
    "ai/trader"
    "CdkeyDecode"
    "ik"
    "ui"
    "gamespy"
    "../xrServerEntities"
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
