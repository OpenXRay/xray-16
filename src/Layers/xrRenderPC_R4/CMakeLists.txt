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
    ../xrRender/FrameGraph/ShaderCache.h
    ../xrRender/FrameGraph/ShaderCache.cpp
    ../xrRender/FrameGraph/VolatileConstantBufferPool.h
    ../xrRender/FrameGraph/VolatileConstantBufferPool.cpp

    # Shader Variant System - Data-driven shader descriptors
    ../xrRender/ShaderVariant/ShaderVariantRegistry.h
    ../xrRender/ShaderVariant/ShaderVariantRegistry.cpp
    ../xrRender/ShaderVariant/JsonParser.h
    ../xrRender/ShaderVariant/JsonParser.cpp
    ../xrRender/ShaderVariant/VariantPartitionConfig.h
    ../xrRender/ShaderVariant/VariantPSOCache.h
    ../xrRender/ShaderVariant/VariantPSOCache.cpp

    # Bindless Rendering - GPU Structured Buffer Base
    ../xrRender/Bindless/GPUStructuredBuffer.h
    ../xrRender/Bindless/GPUStructuredBuffer.cpp

    # Bindless Rendering - Variant Texture Buffer
    ../xrRender/Bindless/VariantTextureBuffer.h
    ../xrRender/Bindless/VariantTextureBuffer.cpp

    # Shader System (Phase 2) - Slang integration
    ../xrRender/Shaders/SlangCompiler.h
    ../xrRender/Shaders/SlangCompiler.cpp
    ../xrRender/Shaders/SlangCompilerTest.cpp
    ../xrRender/Shaders/SlangVFSAdapter.h
    ../xrRender/Shaders/SlangVFSAdapter.cpp

    # FrameGraph Passes (Phase 3) - Renderer agnostic
    ../xrRender/FrameGraphPasses/ShaderConstants.h
    ../xrRender/FrameGraphPasses/ShaderConstants.cpp
    ../xrRender/FrameGraphPasses/ForwardColorPassSetup.h
    ../xrRender/FrameGraphPasses/ForwardColorPassSetup.cpp
    ../xrRender/FrameGraphPasses/HiZBuildPassSetup.h
    ../xrRender/FrameGraphPasses/HiZBuildPassSetup.cpp
    ../xrRender/FrameGraphPasses/SkinningPassSetup.h
    ../xrRender/FrameGraphPasses/SkinningPassSetup.cpp
    ../xrRender/FrameGraphPasses/ImGuiPassSetup.h
    ../xrRender/FrameGraphPasses/ImGuiPassSetup.cpp
    ../xrRender/FrameGraphPasses/ParticlePassSetup.h
    ../xrRender/FrameGraphPasses/ParticlePassSetup.cpp
    ../xrRender/FrameGraphPasses/ParticleGPUCullingManager.h
    ../xrRender/FrameGraphPasses/ParticleGPUCullingManager.cpp
    ../xrRender/FrameGraphPasses/ExposurePassSetup.h
    ../xrRender/FrameGraphPasses/ExposurePassSetup.cpp
    ../xrRender/FrameGraphPasses/TonemapPassSetup.h
    ../xrRender/FrameGraphPasses/TonemapPassSetup.cpp
    ../xrRender/FrameGraphPasses/UIPassSetup.h
    ../xrRender/FrameGraphPasses/UIPassSetup.cpp
    ../xrRender/FrameGraphPasses/SkyPassSetup.h
    ../xrRender/FrameGraphPasses/SkyPassSetup.cpp
    ../xrRender/FrameGraphPasses/SunPassSetup.h
    ../xrRender/FrameGraphPasses/SunPassSetup.cpp
    ../xrRender/FrameGraphPasses/DetailPassSetup.h
    ../xrRender/FrameGraphPasses/DetailPassSetup.cpp
    ../xrRender/FrameGraphPasses/TransparentPassSetup.h
    ../xrRender/FrameGraphPasses/TransparentPassSetup.cpp
    ../xrRender/FrameGraphPasses/PassCommon.h
    ../xrRender/FrameGraphPasses/PassCommon.cpp

    # Detail System (Framegraph) - Renderer agnostic
    ../xrRender/FGDetailManager.h
    ../xrRender/FGDetailManager.cpp

    # Geometry System (Phase 3) - Renderer agnostic
    ../xrRender/Geometry/GeometryBatch.h
    ../xrRender/Geometry/GeometryBatch.cpp

    # UI Rendering System (NVRHI-based) - Renderer agnostic
    ../xrRender/UIGeometryBatch.h
    ../xrRender/UIRenderCollector.h
    ../xrRender/UIRenderCollector.cpp
    ../xrRender/NVRHIUIRenderer.h
    ../xrRender/NVRHIUIRenderer.cpp

    # Debug Tools (Phase 3) - Renderer agnostic
    ../xrRender/Debug/GBufferVisualization.h
    ../xrRender/Debug/GBufferVisualization.cpp

    # FrameGraph Renderer (Phase 3) - Renderer agnostic
    ../xrRender/r_FrameGraphRenderer.h
    ../xrRender/r_FrameGraphRenderer.cpp

    # ImGui NVRHI Renderer - Backend agnostic
    ../xrRender/dxImGuiRender.h
    ../xrRender/dxImGuiRender.cpp
    ../xrRender/ImGuiRendererNVRHI.h
    ../xrRender/ImGuiRendererNVRHI.cpp
    ../xrRender/ImGuiRendererNVRHI_Shaders.cpp
    ../xrRender/ImGuiRendererNVRHI_Render.cpp
    ../xrRender/ImGuiRendererIntegration.cpp
    ../xrRender/dxRenderFactory_ImGui.cpp
    ../xrRender/GlobalRenderDevice.cpp

    # Vulkan Backend
    ../xrRender/Backend/VulkanBackend.h
    ../xrRender/Backend/VulkanBackend.cpp
)

target_include_directories(xrRenderPC_R4
    PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/sdk/include"
    "${CMAKE_SOURCE_DIR}/Externals/nvrhi/include"
    "${CMAKE_SOURCE_DIR}/Externals/slang/include"
)

find_package(Vulkan REQUIRED)

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
    nvrhi_vk
    Vulkan::Vulkan
    slang
)

target_compile_definitions(xrRenderPC_R4
    PRIVATE
    XRRENDER_R4_EXPORTS
    USE_DX11
    USE_NVRHI_IMGUI
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
