project(xrRender_GL)

list(APPEND DIRS
    "."
    "../xrRender"
    "../xrRender/blenders"
    "../xrRenderGL"
    "../xrRenderGL/MSAA"
    "../xrRenderGL/GL Rain"
    "../../Include/xrRender"
    )


add_dir("${DIRS}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../xrRender
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include/xrRender
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Externals/luabind
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Externals/gli
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Externals/gli/external/glm
    ${SDL_INCLUDE_DIRS}
    )

list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./r2_R_sun.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./r3_R_rain.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/HW.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/SH_RT.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/stats_manager.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/Texture.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/SH_Texture.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/ResourceManager_Resources.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/xr_effgamma.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/r__pixel_calculator.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/ResourceManager_Scripting.cpp")

list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/stats_manager.h")
list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/xr_effgamma.h")
list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/r__pixel_calculator.h")

add_definitions(-DXRRENDER_GL_EXPORTS -DUSE_OGL -DNO_XR_VDECLARATOR)

add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "stdafx.h")
cotire(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore xrCDB xrEngine xrParticles xrScriptEngine xrAPI GL GLEW luabind ${LUA_LIBRARIES})
xr_install(${PROJECT_NAME})

