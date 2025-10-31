add_library(xrRenderPC_R4)

target_sources(xrRenderPC_R4 PRIVATE
    packages.config
    r2_test_hw.cpp
    r4_rendertarget.h
    r4_rendertarget_accum_direct.cpp
    r4_rendertarget_build_textures.cpp
    r4_rendertarget_phase_combine.cpp
    r4_rendertarget_phase_hdao.cpp
    r4_rendertarget_u_set_rt.cpp
    r4_shaders.cpp
    R_Backend_LOD.cpp
    R_Backend_LOD.h
    stdafx.cpp
    stdafx.h
    xrRender_R4.cpp

    # NVRHI wrapper
    NVRHI/NVRHIDevice.h
    NVRHI/NVRHIDevice.cpp

    # RenderContext (Phase 1)
    RenderContext/RenderContext.h
    RenderContext/RenderContext.cpp

    # FrameGraph (Phase 2)
    FrameGraph/FGTypes.h
    FrameGraph/FGResource.h
    FrameGraph/FGResource.cpp
    FrameGraph/FGPass.h
    FrameGraph/FGPass.cpp
    FrameGraph/FrameGraph.h
    FrameGraph/FrameGraph.cpp
    FrameGraph/FGTest.h
    FrameGraph/FGTest.cpp
)

target_include_directories(xrRenderPC_R4
    PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/sdk/include"
    "${CMAKE_SOURCE_DIR}/Externals/nvrhi/include"
)

target_link_libraries(xrRenderPC_R4
    PRIVATE
    xrCore
    xrCDB
    xrEngine
    xrParticles
    xrScriptEngine
    xrAPI
    xrMiscMath
    nvrhi
    nvrhi-d3d11
)

target_compile_definitions(xrRenderPC_R4
    PRIVATE
    XRRENDER_R4_EXPORTS
    USE_DX11
)

set_target_properties(xrRenderPC_R4 PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrRenderPC_R4
    PRIVATE
    stdafx.h
)

if (BUILD_SHARED_LIBS)
    install(TARGETS xrRenderPC_R4 LIBRARY
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
