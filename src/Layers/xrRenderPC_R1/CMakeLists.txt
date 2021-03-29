project(xrRenderPC_R1)

set(SRC_FILES
    "FStaticRender_Blenders.cpp"
    "FStaticRender.cpp"
    "FStaticRender_DetectSector.cpp"
    "FStaticRender.h"
    "FStaticRender_Loader.cpp"
    "FStaticRender_RenderTarget.cpp"
    "FStaticRender_RenderTarget.h"
    "FStaticRender_Shaders.cpp"
    "FStaticRender_Types.h"
    "GlowManager.cpp"
    "GlowManager.h"
    "LightPPA.cpp"
    "LightPPA.h"
    "LightProjector.cpp"
    "LightProjector.h"
    "LightShadows.cpp"
    "LightShadows.h"
    "stdafx.cpp"
    "stdafx.h"
    "xrRender_R1.cpp"
)

group_sources(SRC_FILES)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/Externals/luabind
    ${CMAKE_SOURCE_DIR}/sdk/include
    ${SDL_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    xrCore
    xrLuabind
    xrCDB
    xrEngine
    xrParticles
    xrScriptEngine
    xrAPI
    xrMiscMath
    ${SDL_LIBRARIES}
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DXRRENDER_R1_EXPORTS
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    "stdafx.h"
)

install(TARGETS ${PROJECT_NAME} LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
