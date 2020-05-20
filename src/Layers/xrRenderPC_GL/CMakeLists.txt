project(xrRender_GL)

list(APPEND DIRS
    "."
    "../xrRender"
    "../xrRender/blenders"
    "../xrRenderGL"
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

list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/HW.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/SH_RT.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/stats_manager.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/Texture.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/SH_Texture.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/xr_effgamma.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/ResourceManager_Scripting.cpp")

# Unused blenders
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/blenders/BlenderDefault.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/blenders/Blender_default_aref.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/blenders/Blender_LaEmB.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/blenders/Blender_Model.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/blenders/Blender_Vertex.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/blenders/Blender_Vertex_aref.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/blenders/dx10MinMaxSMBlender.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/blenders/dx10MSAABlender.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/blenders/dx10RainBlender.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/blenders/dx11HDAOCSBlender.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/blenders/dx11MinMaxSMBlender.cpp")

list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/stats_manager.h")
list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/xr_effgamma.h")

add_definitions(-DXRRENDER_GL_EXPORTS -DUSE_OGL -DNO_XR_VDECLARATOR)

add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "stdafx.h")
cotire(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore xrCDB xrEngine xrParticles xrScriptEngine xrAPI GL GLEW xrLuabind ${LUA_LIBRARIES})

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} PERMISSIONS OWNER_READ OWNER_WRITE 
    GROUP_READ 
    WORLD_READ )

