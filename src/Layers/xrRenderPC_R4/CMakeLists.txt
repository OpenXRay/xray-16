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

    # RenderContext (Phase 1) - Renderer agnostic

    # FrameGraph (Phase 2) - Renderer agnostic

    # Shader Variant System - Data-driven shader descriptors

    # Bindless Rendering - GPU Structured Buffer Base

    # Bindless Rendering - Variant Texture Buffer

    # Shader System (Phase 2) - Slang integration

    # FrameGraph Passes (Phase 3) - Renderer agnostic


    # Ray Tracing

    # Detail System (Framegraph) - Renderer agnostic

    # Geometry System (Phase 3) - Renderer agnostic

    # UI Rendering System (NVRHI-based) - Renderer agnostic

    # FrameGraph Renderer (Phase 3) - Renderer agnostic

    # ImGui NVRHI Renderer - Backend agnostic
    ../xrRenderDX11/dxImGuiRender.h
    ../xrRenderDX11/dxImGuiRender.cpp

    # Vulkan Backend

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

    ../xrRenderDX11/Blender.cpp
    ../xrRenderDX11/Blender_Recorder.cpp
    ../xrRenderDX11/Blender_Recorder_R2.cpp
    ../xrRenderDX11/Blender_Recorder_StandartBinding.cpp
    ../xrRenderDX11/DetailManager.cpp
    ../xrRenderDX11/DetailManager_CACHE.cpp
    ../xrRenderDX11/DetailManager_Decompress.cpp
    ../xrRenderDX11/DetailManager_VS.cpp
    ../xrRenderDX11/DetailManager_soft.cpp
    ../xrRenderDX11/R_Backend.cpp
    ../xrRenderDX11/R_Backend_DBG.cpp
    ../xrRenderDX11/R_Backend_LOD.h
    ../xrRenderDX11/R_Backend_LOD.cpp
    ../xrRenderDX11/R_Backend_Runtime.cpp
    ../xrRenderDX11/R_Backend_hemi.cpp
    ../xrRenderDX11/R_Backend_tree.cpp
    ../xrRenderDX11/R_Backend_xform.cpp
    ../xrRenderDX11/ResourceManager.cpp
    ../xrRenderDX11/ResourceManager_Loader.cpp
    ../xrRenderDX11/ResourceManager_Reset.cpp
    ../xrRenderDX11/ResourceManager_Resources.cpp
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
    ../xrRenderDX11/tss_def.cpp
    ../xrRenderDX11/xrRender_console.cpp
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
    xrRender
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
