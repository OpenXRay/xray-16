add_library(xrRenderPC_R1 SHARED)

target_sources(xrRenderPC_R1 PRIVATE
    FStaticRender_Blenders.cpp
    FStaticRender.cpp
    FStaticRender.h
    FStaticRender_Loader.cpp
    FStaticRender_RenderTarget.cpp
    FStaticRender_RenderTarget.h
    FStaticRender_Shaders.cpp
    FStaticRender_Types.h
    GlowManager.cpp
    GlowManager.h
    LightPPA.cpp
    LightPPA.h
    LightProjector.cpp
    LightProjector.h
    LightShadows.cpp
    LightShadows.h
    stdafx.cpp
    stdafx.h
    xrRender_R1.cpp
)

target_include_directories(xrRenderPC_R1
    PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/sdk/include"
)

target_link_libraries(xrRenderPC_R1
    PRIVATE
    xrCore
    xrCDB
    xrEngine
    xrParticles
    xrScriptEngine
    xrAPI
    xrMiscMath
)

target_compile_definitions(xrRenderPC_R1
    PRIVATE
    XRRENDER_R1_EXPORTS
)

set_target_properties(xrRenderPC_R1 PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrRenderPC_R1
    PRIVATE
    stdafx.h
)

install(TARGETS xrRenderPC_R1 LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
