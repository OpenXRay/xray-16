add_library(xrRender STATIC)

target_sources(xrRender PRIVATE
    stdafx.h
    stdafx.cpp

    RenderContext/ResourceHandle.h
    RenderContext/RCShader.h
    RenderContext/RCShader.cpp
    RenderContext/PipelineState.h
    RenderContext/PipelineState.cpp
    RenderContext/RenderDevice.h
    RenderContext/RenderDevice.cpp
    RenderContext/RenderContext.h
    RenderContext/RenderContext.cpp

    FrameGraph/FGTypes.h
    FrameGraph/FGResource.h
    FrameGraph/FGResource.cpp
    FrameGraph/FGPass.h
    FrameGraph/FGPass.cpp
    FrameGraph/FrameGraph.h
    FrameGraph/FrameGraph.cpp
    FrameGraph/FGTest.h
    FrameGraph/FGTest.cpp
    FrameGraph/ShaderLoader.h
    FrameGraph/ShaderLoader.cpp
    FrameGraph/ShaderCache.h
    FrameGraph/ShaderCache.cpp
    FrameGraph/VolatileConstantBufferPool.h
    FrameGraph/VolatileConstantBufferPool.cpp
    FrameGraph/BindingLayoutBuilder.cpp
    FrameGraph/BindingSetBuilder.cpp
    FrameGraph/FGResourcePool.cpp
    FrameGraph/PassResourceCache.cpp
    FrameGraph/ShaderPhaseCache.cpp
    FrameGraph/ShaderReflection.cpp

    ShaderVariant/ShaderVariantRegistry.h
    ShaderVariant/ShaderVariantRegistry.cpp
    ShaderVariant/JsonParser.h
    ShaderVariant/JsonParser.cpp
    ShaderVariant/VariantPartitionConfig.h
    ShaderVariant/VariantPSOCache.h
    ShaderVariant/VariantPSOCache.cpp

    Bindless/GPUStructuredBuffer.h
    Bindless/GPUStructuredBuffer.cpp
    Bindless/VariantTextureBuffer.h
    Bindless/VariantTextureBuffer.cpp
    Bindless/MaterialBuffer.cpp
    Bindless/TerrainMaterialBuffer.cpp
    Bindless/VertexConverter.cpp

    Shaders/SlangCompiler.h
    Shaders/SlangCompiler.cpp
    Shaders/SlangCompilerTest.cpp
    Shaders/SlangVFSAdapter.h
    Shaders/SlangVFSAdapter.cpp
    Shaders/SlangReflectionWrapper.cpp

    FrameGraphPasses/ShaderConstants.h
    FrameGraphPasses/ShaderConstants.cpp
    FrameGraphPasses/ForwardColorPassSetup.h
    FrameGraphPasses/ForwardColorPassSetup.cpp
    FrameGraphPasses/HiZBuildPassSetup.h
    FrameGraphPasses/HiZBuildPassSetup.cpp
    FrameGraphPasses/SkinningPassSetup.h
    FrameGraphPasses/SkinningPassSetup.cpp
    FrameGraphPasses/ImGuiPassSetup.h
    FrameGraphPasses/ImGuiPassSetup.cpp
    FrameGraphPasses/StatGraphPassSetup.h
    FrameGraphPasses/StatGraphPassSetup.cpp
    FrameGraphPasses/DebugDrawPassSetup.h
    FrameGraphPasses/DebugDrawPassSetup.cpp
    FrameGraphPasses/RainPassSetup.h
    FrameGraphPasses/RainPassSetup.cpp
    FrameGraphPasses/ThunderboltPassSetup.h
    FrameGraphPasses/ThunderboltPassSetup.cpp
    FrameGraphPasses/LensFlarePassSetup.h
    FrameGraphPasses/LensFlarePassSetup.cpp
    fgStatGraphRender.h
    fgStatGraphRender.cpp
    fgRainRender.h
    fgRainRender.cpp
    fgFontRender.h
    fgFontRender.cpp
    FrameGraphPasses/FontPassSetup.h
    FrameGraphPasses/FontPassSetup.cpp
    fgThunderboltRender.h
    fgThunderboltRender.cpp
    fgLensFlareRender.h
    fgLensFlareRender.cpp
    fgFlareRender.h
    fgFlareRender.cpp
    fgEnvironmentRender.h
    fgEnvironmentRender.cpp
    FrameGraphPasses/ParticlePassSetup.h
    FrameGraphPasses/ParticlePassSetup.cpp
    FrameGraphPasses/RibbonPassSetup.h
    FrameGraphPasses/RibbonPassSetup.cpp
    FrameGraphPasses/TrailPassSetup.h
    FrameGraphPasses/TrailPassSetup.cpp
    FrameGraphPasses/SmokeTrailManager.h
    FrameGraphPasses/SmokeTrailManager.cpp
    FrameGraphPasses/SmokeTrailPassSetup.h
    FrameGraphPasses/SmokeTrailPassSetup.cpp
    FrameGraphPasses/DecalPassSetup.h
    FrameGraphPasses/DecalPassSetup.cpp
    FrameGraphPasses/DistortionApplyPassSetup.h
    FrameGraphPasses/DistortionApplyPassSetup.cpp
    FrameGraphPasses/ExposurePassSetup.h
    FrameGraphPasses/ExposurePassSetup.cpp
    FrameGraphPasses/TonemapPassSetup.h
    FrameGraphPasses/TonemapPassSetup.cpp
    FrameGraphPasses/UIPassSetup.h
    FrameGraphPasses/UIPassSetup.cpp
    FrameGraphPasses/SkyPassSetup.h
    FrameGraphPasses/SkyPassSetup.cpp
    FrameGraphPasses/SunPassSetup.h
    FrameGraphPasses/SunPassSetup.cpp
    FrameGraphPasses/DetailCullPassSetup.h
    FrameGraphPasses/DetailCullPassSetup.cpp
    FrameGraphPasses/DetailPassSetup.h
    FrameGraphPasses/DetailPassSetup.cpp
    FrameGraphPasses/TransparentPassSetup.h
    FrameGraphPasses/TransparentPassSetup.cpp
    FrameGraphPasses/PassCommon.h
    FrameGraphPasses/PassCommon.cpp
    FrameGraphPasses/PathTracerPassSetup.h
    FrameGraphPasses/PathTracerPassSetup.cpp
    FrameGraphPasses/MotionVectorPassSetup.h
    FrameGraphPasses/MotionVectorPassSetup.cpp
    FrameGraphPasses/ReSTIRGIPassSetup.h
    FrameGraphPasses/ReSTIRGIPassSetup.cpp
    FrameGraphPasses/ClusterLightPassSetup.cpp

    RayTracing/RTAccelStructManager.h
    RayTracing/RTAccelStructManager.cpp

    FGDetailManager.h
    FGDetailManager.cpp

    FGDebugDraw.h
    FGDebugDraw.cpp

    Geometry/GeometryBatch.h
    Geometry/GeometryBatch.cpp
    Geometry/SkinnedGeometryPools.h
    Geometry/SkinnedGeometryPools.cpp
    Geometry/MaterialCache.cpp

    UIGeometryBatch.h
    fgUIRender.h
    fgUIRender.cpp

    r_FrameGraphRenderer.h
    r_FrameGraphRenderer.cpp
    r_FrameGraphRenderer_Loader.cpp
    r_FrameGraphRenderer_Shaders.cpp
    r_FrameGraphRenderer_Blenders.cpp
    FGRenderHost.h
    FGRenderHost.cpp
    FrameGraphRendererModule.h
    FrameGraphRendererModule.cpp

    ImGuiRendererNVRHI.h
    ImGuiRendererNVRHI.cpp
    ImGuiRendererNVRHI_Shaders.cpp
    ImGuiRendererNVRHI_Render.cpp
    ImGuiRendererIntegration.cpp
    fgRenderFactory_ImGui.cpp
    GlobalRenderDevice.cpp

    Backend/VulkanBackend.h
    Backend/VulkanBackend.cpp

    Animation.cpp
    ClusteredLightManager.cpp
    ColorMapManager.cpp
    ConstantSystem/FGConstantSystem.cpp
    FGRenderBase.h
    FGRenderBase.cpp
    r4_rendertarget.h
    r4_rendertarget.cpp
    DetailModel.cpp
    ETextureParams.cpp
    FBasicVisual.cpp
    FHierrarhyVisual.cpp
    FLOD.cpp
    FProgressive.cpp
    FSkinned.cpp
    FTreeVisual.cpp
    FVisual.cpp
    GPUCullingManager.cpp
    LightTrack.cpp
    Light_DB.cpp
    Light_Package.cpp
    Light_Render_Direct.cpp
    Light_Render_Direct_ComputeXFS.cpp
    Materials/MaterialSystem.cpp
    ModelPool.cpp
    NvTriStrip.cpp
    NvTriStripObjects.cpp
    PBRConverter/ONNXModelRunner.cpp
    PBRConverter/PBRConversionUI.cpp
    PBRConverter/PBRTextureConverter.cpp
    PSLibrary.cpp
    ParticleEffect.cpp
    ParticleEffectDef.cpp
    ParticleGroup.cpp
    Profiler/GPUProfiler.cpp
    Profiler/StatsOverlay.cpp
    ResourceManager/AsyncIO.cpp
    ResourceManager/BufferManager.cpp
    ResourceManager/DDSLoader.cpp
    ResourceManager/FGResourceManager.cpp
    ResourceManager/NativeRTFactory.cpp
    ResourceManager/SamplerCache.cpp
    ResourceManager/TestTextureManager.cpp
    ResourceManager/TextureManager.cpp
    ResourceManager/TextureStreaming.cpp
    SH_Constant.cpp
    SH_Matrix.cpp
    ShaderKey.cpp
    SkeletonAnimated.cpp
    SkeletonCustom.cpp
    SH_Texture.h
    SH_Texture.cpp
    SkeletonRigid.cpp
    SkeletonX.cpp
    SkeletonXSkinXW_CPP.cpp
    SkeletonXSkinXW_SSE.cpp
    TextureDescrManager.cpp
    Utils/dxHashHelper.cpp
    VertexCache.cpp
    Decals/DecalManager.h
    Decals/DecalManager.cpp
    Decals/fgWallMarkArray.h
    Decals/fgWallMarkArray.cpp
    Decals/MeshPicker.h
    Decals/MeshPicker.cpp
    Decals/OverlayManager.h
    Decals/OverlayManager.cpp
    du_box.cpp
    du_cone.cpp
    du_cylinder.cpp
    du_sphere.cpp
    du_sphere_part.cpp
    dxParticleCustom.cpp
    fgRenderFactory.cpp
    fgUISequenceVideoItem.cpp
    light.cpp
    light_gi.cpp
    r__buffer_pool.cpp
    r__buffer_pool.h
    r__hw_compat.h
    VertexLayout.h
    r__pixel_calculator.cpp
    RenderView.h
    r__scene.cpp
    r__scene.h
    r__sector.cpp
    xrStripify.cpp
    BufferUtils.cpp

    ../../../Externals/meshoptimizer/src/allocator.cpp
    ../../../Externals/meshoptimizer/src/clusterizer.cpp
    ../../../Externals/meshoptimizer/src/indexanalyzer.cpp
    ../../../Externals/meshoptimizer/src/indexcodec.cpp
    ../../../Externals/meshoptimizer/src/indexgenerator.cpp
    ../../../Externals/meshoptimizer/src/overdrawoptimizer.cpp
    ../../../Externals/meshoptimizer/src/partition.cpp
    ../../../Externals/meshoptimizer/src/quantization.cpp
    ../../../Externals/meshoptimizer/src/rasterizer.cpp
    ../../../Externals/meshoptimizer/src/simplifier.cpp
    ../../../Externals/meshoptimizer/src/spatialorder.cpp
    ../../../Externals/meshoptimizer/src/stripifier.cpp
    ../../../Externals/meshoptimizer/src/vcacheoptimizer.cpp
    ../../../Externals/meshoptimizer/src/vertexcodec.cpp
    ../../../Externals/meshoptimizer/src/vertexfilter.cpp
    ../../../Externals/meshoptimizer/src/vfetchoptimizer.cpp

    ./Blender.cpp
    ./Blender_Recorder.cpp
    ./Blender_Recorder_R2.cpp
    ./Blender_Recorder_R3.cpp
    ./ResourceManager.cpp
    ./ResourceManager_Loader.cpp
    ./ResourceManager_Reset.cpp
    ./ResourceManager_Resources.cpp
    ./Shader.cpp
    ./SH_Atomic.cpp
    ./fgUIShader.cpp
    ./fgThunderboltDescRender.cpp
    ./fgObjectSpaceRender.cpp
    D3DUtils.cpp
    ./HOM.cpp
    ./occRasterizer.cpp
    ./occRasterizer_core.cpp
    ./R_DStreams.cpp
    ./r_constants.cpp
    ShaderInfo.cpp
    ./tss_def.cpp
    ./xrRender_console.cpp
    xr_effgamma.cpp
    ./ResourceManager_Scripting.cpp

    ./blenders/Blender_BmmD_deferred.cpp
    ./blenders/Blender_Editor_Selection.cpp
    ./blenders/Blender_Editor_Wire.cpp
    "./blenders/Blender_Lm(EbB).cpp"
    ./blenders/Blender_Model_EbB_deferred.cpp
    ./blenders/Blender_Particle_deferred.cpp
    ./blenders/Blender_Screen_SET.cpp
    ./blenders/Blender_detail_still_deferred.cpp
    ./blenders/Blender_tree_deferred.cpp
    ./blenders/blender_deffer_aref.cpp
    ./blenders/blender_deffer_flat.cpp
    ./blenders/blender_deffer_model.cpp
    ./blenders/uber_deffer.cpp


)

if(WIN32)
    target_sources(xrRender PRIVATE
        Backend/D3D12Backend.cpp
    )
endif()

target_include_directories(xrRender
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/sdk/include"
    "${CMAKE_SOURCE_DIR}/sdk/include/DirectXMesh"
    "${CMAKE_SOURCE_DIR}/sdk/include/nvapi"
    "${CMAKE_SOURCE_DIR}/Externals/AGS_SDK"
    "${CMAKE_SOURCE_DIR}/src/packages/directxtex_desktop_2019.2024.10.29.1/include"
    "${CMAKE_SOURCE_DIR}/src/packages/directxmath.2024.10.15.1/include"
    "${CMAKE_SOURCE_DIR}/Externals/nvrhi/include"
    "${CMAKE_SOURCE_DIR}/Externals/slang/include"
    "${CMAKE_SOURCE_DIR}/Externals/DirectX-Headers/include"
)

if (APPLE)
    set(XRAY_VULKAN_LIBS MoltenVK Vulkan::Headers)
else()
    find_package(Vulkan REQUIRED)
    target_include_directories(xrRender PRIVATE ${Vulkan_INCLUDE_DIRS})
    set(XRAY_VULKAN_LIBS ${Vulkan_LIBRARIES})
endif()

target_link_libraries(xrRender
    PRIVATE
    xrCore
    xrCDB
    xrEngine
    xrMaterialSystem
    xrUICore
    xrParticles
    xrScriptEngine
    xrAPI
    xrMiscMath
    nvrhi_vk
    nvrhi
    ${XRAY_VULKAN_LIBS}
    slang
)

if(WIN32)
    target_link_libraries(xrRender PRIVATE nvrhi_d3d11 nvrhi_d3d12 d3dcompiler)
endif()

target_compile_definitions(xrRender
    PRIVATE
    USE_DX11
    USE_NVRHI_IMGUI
)

if(XRAY_USE_AI_PBR)
    set(XRAY_ONNXRUNTIME_ROOT "${CMAKE_SOURCE_DIR}/Externals/onnxruntime" CACHE PATH "ONNX Runtime root directory")

    find_path(XRAY_ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h
        HINTS
        "${XRAY_ONNXRUNTIME_ROOT}/include/onnxruntime/core/session"
        "${XRAY_ONNXRUNTIME_ROOT}/include/onnxruntime"
        "${XRAY_ONNXRUNTIME_ROOT}/include"
        "/opt/homebrew/opt/onnxruntime/include/onnxruntime"
        "/usr/local/opt/onnxruntime/include/onnxruntime"
    )
    find_library(XRAY_ONNXRUNTIME_LIBRARY onnxruntime
        HINTS
        "${XRAY_ONNXRUNTIME_ROOT}/vs_build/Release"
        "${XRAY_ONNXRUNTIME_ROOT}/vs_build/RelWithDebInfo"
        "${XRAY_ONNXRUNTIME_ROOT}/build/Release"
        "${XRAY_ONNXRUNTIME_ROOT}/lib"
        "/opt/homebrew/opt/onnxruntime/lib"
        "/usr/local/opt/onnxruntime/lib"
    )

    if(NOT XRAY_ONNXRUNTIME_INCLUDE_DIR OR NOT XRAY_ONNXRUNTIME_LIBRARY)
        message(FATAL_ERROR
            "XRAY_USE_AI_PBR=ON but ONNX Runtime was not found under ${XRAY_ONNXRUNTIME_ROOT}.\n"
            "Build Externals/onnxruntime, install onnxruntime (e.g. brew install onnxruntime), "
            "or point XRAY_ONNXRUNTIME_ROOT at an ONNX Runtime install.")
    endif()

    message(STATUS "AI PBR converter: ON (onnxruntime: ${XRAY_ONNXRUNTIME_LIBRARY})")

    target_compile_definitions(xrRender PRIVATE USE_AI_PBR)
    target_include_directories(xrRender PRIVATE "${XRAY_ONNXRUNTIME_INCLUDE_DIR}")
    target_link_libraries(xrRender PRIVATE "${XRAY_ONNXRUNTIME_LIBRARY}")
endif()

set_target_properties(xrRender PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrRender
    PRIVATE
    stdafx.h
)
