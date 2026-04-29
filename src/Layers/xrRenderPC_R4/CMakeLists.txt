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
    ../xrRender/FrameGraphPasses/RibbonPassSetup.h
    ../xrRender/FrameGraphPasses/RibbonPassSetup.cpp
    ../xrRender/FrameGraphPasses/TrailPassSetup.h
    ../xrRender/FrameGraphPasses/TrailPassSetup.cpp
    ../xrRender/FrameGraphPasses/SmokeTrailManager.h
    ../xrRender/FrameGraphPasses/SmokeTrailManager.cpp
    ../xrRender/FrameGraphPasses/SmokeTrailPassSetup.h
    ../xrRender/FrameGraphPasses/SmokeTrailPassSetup.cpp
    ../xrRender/FrameGraphPasses/DecalPassSetup.h
    ../xrRender/FrameGraphPasses/DecalPassSetup.cpp

    ../xrRender/FrameGraphPasses/DistortionApplyPassSetup.h
    ../xrRender/FrameGraphPasses/DistortionApplyPassSetup.cpp
    ../xrRender/Decals/DecalManager.h
    ../xrRender/Decals/DecalManager.cpp
    ../xrRender/Decals/fgWallMarkArray.h
    ../xrRender/Decals/fgWallMarkArray.cpp
    ../xrRender/Decals/MeshPicker.h
    ../xrRender/Decals/MeshPicker.cpp
    ../xrRender/Decals/OverlayManager.h
    ../xrRender/Decals/OverlayManager.cpp
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
    ../xrRender/FrameGraphPasses/DetailCullPassSetup.h
    ../xrRender/FrameGraphPasses/DetailCullPassSetup.cpp
    ../xrRender/FrameGraphPasses/DetailPassSetup.h
    ../xrRender/FrameGraphPasses/DetailPassSetup.cpp
    ../xrRender/FrameGraphPasses/TransparentPassSetup.h
    ../xrRender/FrameGraphPasses/TransparentPassSetup.cpp
    ../xrRender/FrameGraphPasses/PassCommon.h
    ../xrRender/FrameGraphPasses/PassCommon.cpp
    ../xrRender/FrameGraphPasses/PathTracerPassSetup.h
    ../xrRender/FrameGraphPasses/PathTracerPassSetup.cpp
    ../xrRender/FrameGraphPasses/MotionVectorPassSetup.h
    ../xrRender/FrameGraphPasses/MotionVectorPassSetup.cpp
    ../xrRender/FrameGraphPasses/ReSTIRGIPassSetup.h
    ../xrRender/FrameGraphPasses/ReSTIRGIPassSetup.cpp

    # Ray Tracing
    ../xrRender/RayTracing/RTAccelStructManager.h
    ../xrRender/RayTracing/RTAccelStructManager.cpp

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

    # FrameGraph Renderer (Phase 3) - Renderer agnostic
    ../xrRender/r_FrameGraphRenderer.h
    ../xrRender/r_FrameGraphRenderer.cpp
    ../xrRender/r5.h
    ../xrRender/r5.cpp
    ../xrRender/FGRenderHost.h
    ../xrRender/FGRenderHost.cpp
    ../xrRender/FrameGraphRendererModule.h
    ../xrRender/FrameGraphRendererModule.cpp

    # ImGui NVRHI Renderer - Backend agnostic
    ../xrRenderDX11/dxImGuiRender.h
    ../xrRenderDX11/dxImGuiRender.cpp
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

    # === ported from xrRender_R4.vcxproj ===
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

    ../xrRender/Animation.cpp
    ../xrRender/Backend/D3D11BackendWrapper.cpp
    ../xrRender/Backend/D3D12Backend.cpp
    ../xrRender/Bindless/MaterialBuffer.cpp
    ../xrRender/Bindless/TerrainMaterialBuffer.cpp
    ../xrRender/Bindless/VertexConverter.cpp
    ../xrRenderDX11/Blender.cpp
    ../xrRenderDX11/Blender_Recorder.cpp
    ../xrRenderDX11/Blender_Recorder_R2.cpp
    ../xrRenderDX11/Blender_Recorder_StandartBinding.cpp
    ../xrRender/ClusteredLightManager.cpp
    ../xrRender/ColorMapManager.cpp
    ../xrRender/ConstantSystem/FGConstantSystem.cpp
    ../xrRender/D3DUtils.cpp
    ../xrRender/D3DXRenderBase.cpp
    ../xrRenderDX11/DetailManager.cpp
    ../xrRenderDX11/DetailManager_CACHE.cpp
    ../xrRenderDX11/DetailManager_Decompress.cpp
    ../xrRenderDX11/DetailManager_VS.cpp
    ../xrRenderDX11/DetailManager_soft.cpp
    ../xrRender/DetailModel.cpp
    ../xrRender/ETextureParams.cpp
    ../xrRender/FBasicVisual.cpp
    ../xrRender/FHierrarhyVisual.cpp
    ../xrRender/FLOD.cpp
    ../xrRender/FProgressive.cpp
    ../xrRender/FSkinned.cpp
    ../xrRender/FTreeVisual.cpp
    ../xrRender/FVisual.cpp
    ../xrRender/FrameGraph/BindingLayoutBuilder.cpp
    ../xrRender/FrameGraph/BindingSetBuilder.cpp
    ../xrRender/FrameGraph/FGResourcePool.cpp
    ../xrRender/FrameGraph/PassResourceCache.cpp
    ../xrRender/FrameGraph/ShaderPhaseCache.cpp
    ../xrRender/FrameGraph/ShaderReflection.cpp
    ../xrRender/FrameGraphPasses/ClusterLightPassSetup.cpp
    ../xrRender/GPUCullingManager.cpp
    ../xrRender/Geometry/MaterialCache.cpp
    ../xrRender/HOM.cpp
    ../xrRender/LightTrack.cpp
    ../xrRender/Light_DB.cpp
    ../xrRender/Light_Package.cpp
    ../xrRender/Light_Render_Direct.cpp
    ../xrRender/Light_Render_Direct_ComputeXFS.cpp
    ../xrRender/Materials/MaterialSystem.cpp
    ../xrRender/ModelPool.cpp
    ../xrRender/NvTriStrip.cpp
    ../xrRender/NvTriStripObjects.cpp
    ../xrRender/PBRConverter/ONNXModelRunner.cpp
    ../xrRender/PBRConverter/PBRTextureConverter.cpp
    ../xrRender/PSLibrary.cpp
    ../xrRender/ParticleEffect.cpp
    ../xrRender/ParticleEffectDef.cpp
    ../xrRender/ParticleGroup.cpp
    ../xrRender/Profiler/GPUProfiler.cpp
    ../xrRender/Profiler/StatsOverlay.cpp
    ../xrRenderDX11/R_Backend.cpp
    ../xrRenderDX11/R_Backend_DBG.cpp
    ../xrRenderDX11/R_Backend_LOD.h
    ../xrRenderDX11/R_Backend_LOD.cpp
    ../xrRenderDX11/R_Backend_Runtime.cpp
    ../xrRenderDX11/R_Backend_hemi.cpp
    ../xrRenderDX11/R_Backend_tree.cpp
    ../xrRenderDX11/R_Backend_xform.cpp
    ../xrRender/R_DStreams.cpp
    ../xrRenderDX11/ResourceManager.cpp
    ../xrRender/ResourceManager/AsyncIO.cpp
    ../xrRender/ResourceManager/BufferManager.cpp
    ../xrRender/ResourceManager/DDSLoader.cpp
    ../xrRender/ResourceManager/FGResourceManager.cpp
    ../xrRender/ResourceManager/NativeRTFactory.cpp
    ../xrRender/ResourceManager/SamplerCache.cpp
    ../xrRender/ResourceManager/TestTextureManager.cpp
    ../xrRender/ResourceManager/TextureManager.cpp
    ../xrRender/ResourceManager/TextureStreaming.cpp
    ../xrRenderDX11/ResourceManager_Loader.cpp
    ../xrRenderDX11/ResourceManager_Reset.cpp
    ../xrRenderDX11/ResourceManager_Resources.cpp
    ../xrRender/SH_Atomic.cpp
    ../xrRender/SH_Constant.cpp
    ../xrRender/SH_Matrix.cpp
    ../xrRender/Shader.cpp
    ../xrRender/ShaderKey.cpp
    ../xrRender/Shaders/SlangReflectionWrapper.cpp
    ../xrRender/SkeletonAnimated.cpp
    ../xrRender/SkeletonCustom.cpp
    ../xrRender/SkeletonRigid.cpp
    ../xrRender/SkeletonX.cpp
    ../xrRender/SkeletonXSkinXW_CPP.cpp
    ../xrRender/SkeletonXSkinXW_SSE.cpp
    ../xrRender/TextureDescrManager.cpp
    ../xrRender/Utils/dxHashHelper.cpp
    ../xrRender/VertexCache.cpp
    ../xrRender/WallmarksEngine.cpp
    ../xrRenderDX11/blenders/Blender_BmmD_deferred.cpp
    ../xrRenderDX11/blenders/Blender_Detail_GPU.cpp
    ../xrRenderDX11/blenders/Blender_Editor_Selection.cpp
    ../xrRenderDX11/blenders/Blender_Editor_Wire.cpp
    "../xrRenderDX11/blenders/Blender_Lm(EbB).cpp"
    ../xrRenderDX11/blenders/Blender_Model_EbB_deferred.cpp
    ../xrRenderDX11/blenders/Blender_Particle_deferred.cpp
    ../xrRenderDX11/blenders/Blender_Screen_SET.cpp
    ../xrRenderDX11/blenders/Blender_detail_still_deferred.cpp
    ../xrRenderDX11/blenders/Blender_tree_deferred.cpp
    ../xrRenderDX11/blenders/blender_bloom_build.cpp
    ../xrRenderDX11/blenders/blender_combine.cpp
    ../xrRenderDX11/blenders/blender_deffer_aref.cpp
    ../xrRenderDX11/blenders/blender_deffer_flat.cpp
    ../xrRenderDX11/blenders/blender_deffer_model.cpp
    ../xrRenderDX11/blenders/blender_light_direct.cpp
    ../xrRenderDX11/blenders/blender_light_mask.cpp
    ../xrRenderDX11/blenders/blender_light_occq.cpp
    ../xrRenderDX11/blenders/blender_light_point.cpp
    ../xrRenderDX11/blenders/blender_light_reflected.cpp
    ../xrRenderDX11/blenders/blender_light_spot.cpp
    ../xrRenderDX11/blenders/blender_luminance.cpp
    ../xrRenderDX11/blenders/blender_ssao.cpp
    ../xrRenderDX11/blenders/dx11HDAOCSBlender.cpp
    ../xrRenderDX11/blenders/dx11MSAABlender.cpp
    ../xrRenderDX11/blenders/dx11MinMaxSMBlender.cpp
    ../xrRenderDX11/blenders/dx11RainBlender.cpp
    ../xrRenderDX11/blenders/uber_deffer.cpp
    ../xrRender/du_box.cpp
    ../xrRender/du_cone.cpp
    ../xrRender/du_cylinder.cpp
    ../xrRender/du_sphere.cpp
    ../xrRender/du_sphere_part.cpp
    ../xrRender/dxDebugRender.cpp
    ../xrRender/dxEnvironmentRender.cpp
    ../xrRender/dxFontRender.cpp
    ../xrRender/dxLensFlareRender.cpp
    ../xrRender/dxObjectSpaceRender.cpp
    ../xrRender/dxParticleCustom.cpp
    ../xrRender/dxRainRender.cpp
    ../xrRender/dxRenderFactory.cpp
    ../xrRender/dxStatGraphRender.cpp
    ../xrRender/dxThunderboltDescRender.cpp
    ../xrRender/dxThunderboltRender.cpp
    ../xrRender/dxUIRender.cpp
    ../xrRender/dxUISequenceVideoItem.cpp
    ../xrRender/dxUIShader.cpp
    ../xrRender/dxWallMarkArray.cpp
    ../xrRender/light.cpp
    ../xrRender/light_GI.cpp
    ../xrRender/light_smapvis.cpp
    ../xrRender/light_vis.cpp
    ../xrRender/occRasterizer.cpp
    ../xrRender/occRasterizer_core.cpp
    ../xrRender/r__dsgraph_build.cpp
    ../xrRender/r__dsgraph_render.cpp
    ../xrRender/r__dsgraph_render_lods.cpp
    ../xrRender/r__occlusion.cpp
    ../xrRender/r__pixel_calculator.cpp
    ../xrRender/r__sector.cpp
    ../xrRender/r__sector_detect.cpp
    ../xrRender/r__sector_traversal.cpp
    ../xrRender/r__sync_point.cpp
    ../xrRender/r_constants.cpp
    ../xrRender/stats_manager.cpp
    ../xrRenderDX11/tss_def.cpp
    ../xrRenderDX11/xrRender_console.cpp
    ../xrRender/xrStripify.cpp
    ../xrRenderDX11/xr_effgamma.cpp

    ../xrRenderDX11/3DFluid/dx113DFluidBlenders.cpp
    ../xrRenderDX11/3DFluid/dx113DFluidData.cpp
    ../xrRenderDX11/3DFluid/dx113DFluidEmitters.cpp
    ../xrRenderDX11/3DFluid/dx113DFluidGrid.cpp
    ../xrRenderDX11/3DFluid/dx113DFluidManager.cpp
    ../xrRenderDX11/3DFluid/dx113DFluidObstacles.cpp
    ../xrRenderDX11/3DFluid/dx113DFluidRenderer.cpp
    ../xrRenderDX11/3DFluid/dx113DFluidVolume.cpp
    ../xrRenderDX11/Blender_Recorder_R3.cpp
    ../xrRenderDX11/StateManager/dx11SamplerStateCache.cpp
    ../xrRenderDX11/StateManager/dx11ShaderResourceStateCache.cpp
    ../xrRenderDX11/StateManager/dx11State.cpp
    ../xrRenderDX11/StateManager/dx11StateCache.cpp
    ../xrRenderDX11/StateManager/dx11StateManager.cpp
    ../xrRender/BufferUtils.cpp
    ../xrRenderDX11/dx11ComputeTest.cpp
    ../xrRenderDX11/dx11ComputeTest_Matrices.cpp
    ../xrRenderDX11/dx11ComputeTest_SIMD.cpp
    ../xrRenderDX11/dx11ConstantBuffer.cpp
    ../xrRenderDX11/dx11DetailManager_VS.cpp
    ../xrRenderDX11/dx11HW.cpp
    ../xrRenderDX11/dx11HWCaps.cpp
    ../xrRenderDX11/dx11ResourceManager_Resources.cpp
    ../xrRenderDX11/dx11ResourceManager_Scripting.cpp
    ../xrRenderDX11/dx11SH_RT.cpp
    ../xrRenderDX11/dx11SH_Texture.cpp
    ../xrRenderDX11/dx11StateUtils.cpp
    ../xrRenderDX11/dx11Texture.cpp
    ../xrRenderDX11/dx11TextureUtils.cpp
    ../xrRenderDX11/dx11r_constants.cpp
    ../xrRenderDX11/dx11r_constants_cache.cpp
    ../xrRenderDX11/dx11r_screenshot.cpp

    ../xrRender_R2/r2.cpp
    ../xrRender_R2/r2_R_calculate.cpp
    ../xrRender_R2/r2_R_lights.cpp
    ../xrRender_R2/r2_R_render.cpp
    ../xrRender_R2/r2_blenders.cpp
    ../xrRender_R2/r2_loader.cpp
    ../xrRender_R2/r2_rendertarget.cpp
    ../xrRender_R2/r2_rendertarget_accum_omnipart_geom.cpp
    ../xrRender_R2/r2_rendertarget_accum_point_geom.cpp
    ../xrRender_R2/r2_rendertarget_accum_reflected.cpp
    ../xrRender_R2/r2_rendertarget_accum_spot_geom.cpp
    ../xrRender_R2/r2_rendertarget_draw_volume.cpp
    ../xrRender_R2/r2_rendertarget_enable_scissor.cpp
    ../xrRender_R2/r2_rendertarget_phase_PP.cpp
    ../xrRender_R2/r2_rendertarget_phase_accumulator.cpp
    ../xrRender_R2/r2_rendertarget_phase_bloom.cpp
    ../xrRender_R2/r2_rendertarget_phase_luminance.cpp
    ../xrRender_R2/r2_rendertarget_phase_smap_S.cpp
    ../xrRender_R2/r3_R_rain.cpp
    ../xrRender_R2/r3_rendertarget_accum_point.cpp
    ../xrRender_R2/r3_rendertarget_accum_spot.cpp
    ../xrRender_R2/r3_rendertarget_create_minmaxSM.cpp
    ../xrRender_R2/r3_rendertarget_draw_rain.cpp
    ../xrRender_R2/r3_rendertarget_mark_msaa_edges.cpp
    ../xrRender_R2/r3_rendertarget_phase_occq.cpp
    ../xrRender_R2/r3_rendertarget_phase_scene.cpp
    ../xrRender_R2/r3_rendertarget_phase_smap_D.cpp
    ../xrRender_R2/r3_rendertarget_phase_ssao.cpp
    ../xrRender_R2/render_phase_sun.cpp
    ../xrRender_R2/render_phase_sun_old.cpp
)

target_include_directories(xrRenderPC_R4
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender_R2"
    "${CMAKE_SOURCE_DIR}/sdk/include"
    "${CMAKE_SOURCE_DIR}/sdk/include/DirectXMesh"
    "${CMAKE_SOURCE_DIR}/sdk/include/nvapi"
    "${CMAKE_SOURCE_DIR}/Externals/AGS_SDK"
    "${CMAKE_SOURCE_DIR}/src/packages/directxtex_desktop_2019.2024.10.29.1/include"
    "${CMAKE_SOURCE_DIR}/src/packages/directxmath.2024.10.15.1/include"
    "${CMAKE_SOURCE_DIR}/Externals/nvrhi/include"
    "${CMAKE_SOURCE_DIR}/Externals/slang/include"
    "${CMAKE_SOURCE_DIR}/Externals/nvrhi/thirdparty/Vulkan-Headers/include"
    "${CMAKE_SOURCE_DIR}/Externals/nvrhi/thirdparty/DirectX-Headers/include"
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
    nvrhi_d3d11
    nvrhi_d3d12
    nvrhi_vk
    d3dcompiler
    ${Vulkan_LIBRARIES}
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
