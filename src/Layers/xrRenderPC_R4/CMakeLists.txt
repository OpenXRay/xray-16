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

    # NVRHI wrapper - Renderer agnostic
    ../xrRender/NVRHI/NVRHIDevice.h
    ../xrRender/NVRHI/NVRHIDevice.cpp

    # RenderContext (Phase 1) - Renderer agnostic
    ../xrRender/RenderContext/ResourceHandle.h
    ../xrRender/RenderContext/RCShader.h
    ../xrRender/RenderContext/RCShader.cpp
    ../xrRender/RenderContext/PipelineState.h
    ../xrRender/RenderContext/PipelineState.cpp
    ../xrRender/RenderContext/RenderDevice.h
    ../xrRender/RenderContext/RenderDevice.cpp
    ../xrRender/RenderContext/RenderContext.h
    ../xrRender/RenderContext/RenderContext.cpp

    # FrameGraph (Phase 2) - Renderer agnostic
    ../xrRender/FrameGraph/FGTypes.h
    ../xrRender/FrameGraph/FGResource.h
    ../xrRender/FrameGraph/FGResource.cpp
    ../xrRender/FrameGraph/FGPass.h
    ../xrRender/FrameGraph/FGPass.cpp
    ../xrRender/FrameGraph/FrameGraph.h
    ../xrRender/FrameGraph/FrameGraph.cpp
    ../xrRender/FrameGraph/FGTest.h
    ../xrRender/FrameGraph/FGTest.cpp
    ../xrRender/FrameGraph/ShaderLoader.h
    ../xrRender/FrameGraph/ShaderLoader.cpp
    ../xrRender/FrameGraph/VolatileConstantBufferPool.h
    ../xrRender/FrameGraph/VolatileConstantBufferPool.cpp

    # FrameGraph Passes (Phase 3) - Renderer agnostic
    ../xrRender/FrameGraphPasses/GBufferPass.h
    ../xrRender/FrameGraphPasses/GBufferPass.cpp
    ../xrRender/FrameGraphPasses/LightingPass.h
    ../xrRender/FrameGraphPasses/LightingPass.cpp
    ../xrRender/FrameGraphPasses/TonemapPass.h
    ../xrRender/FrameGraphPasses/TonemapPass.cpp

    # Geometry System (Phase 3) - Renderer agnostic
    ../xrRender/Geometry/GeometryBatch.h
    ../xrRender/Geometry/GeometryBatch.cpp

    # Debug Tools (Phase 3) - Renderer agnostic
    ../xrRender/Debug/GBufferVisualization.h
    ../xrRender/Debug/GBufferVisualization.cpp

    # FrameGraph Renderer (Phase 3) - Renderer agnostic
    ../xrRender/r_FrameGraphRenderer.h
    ../xrRender/r_FrameGraphRenderer.cpp
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
