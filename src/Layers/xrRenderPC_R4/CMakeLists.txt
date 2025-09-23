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
)

file(GLOB XR_RENDERPC_R4_SHARED_SOURCES
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/*.cpp"
)

# Exclude the legacy DirectX 9 texture sources – the DirectX 11 specific
# implementations live under xrRenderDX11 and would otherwise clash with the
# shared versions (and pull in d3dx9 headers that are unavailable with the
# modern Windows SDK).
list(REMOVE_ITEM XR_RENDERPC_R4_SHARED_SOURCES
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/SH_Texture.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/Texture.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/ResourceManager_Scripting.cpp"
)

set(XR_RENDERPC_R4_BLENDER_SOURCES
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/Blender_Blur.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/Blender_BmmD_deferred.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/Blender_detail_still_deferred.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_bloom_build.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_combine.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_deffer_aref.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_deffer_flat.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_deffer_model.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_dof.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_gasmask_drops.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_gasmask_dudv.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_light_direct.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_light_mask.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_light_occq.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_light_point.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_light_reflected.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_light_spot.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_luminance.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_nightvision.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/blender_ssao.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/dx11HDAOCSBlender.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/dx11MSAABlender.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/dx11MinMaxSMBlender.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/dx11RainBlender.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/Blender_Editor_Selection.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/Blender_Editor_Wire.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/Blender_Lm(EbB).cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/Blender_Model_EbB_deferred.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/Blender_Particle_deferred.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/Blender_Screen_SET.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/Blender_tree_deferred.cpp"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/blenders/uber_deffer.cpp"
)

set(XR_RENDERPC_R4_UTILITY_SOURCES
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender/Utils/dxHashHelper.cpp"
)

file(GLOB_RECURSE XR_RENDERPC_R4_DX11_SOURCES
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRenderDX11/*.cpp"
)

file(GLOB XR_RENDERPC_R4_R2_SOURCES
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender_R2/*.cpp"
)

set(XR_IMGUI_DIR "${CMAKE_SOURCE_DIR}/Externals/imgui")

set(XR_IMGUI_SOURCES
    "${XR_IMGUI_DIR}/imgui.cpp"
    "${XR_IMGUI_DIR}/imgui_draw.cpp"
    "${XR_IMGUI_DIR}/imgui_tables.cpp"
    "${XR_IMGUI_DIR}/imgui_widgets.cpp"
)

if (MSVC)
    list(APPEND XR_IMGUI_SOURCES
        "${XR_IMGUI_DIR}/backends/imgui_impl_dx11.cpp"
        "${XR_IMGUI_DIR}/backends/imgui_impl_win32.cpp"
    )
endif()

set_source_files_properties(${XR_IMGUI_SOURCES} PROPERTIES
    SKIP_PRECOMPILE_HEADERS ON
)

target_sources(xrRenderPC_R4 PRIVATE
    ${XR_RENDERPC_R4_SHARED_SOURCES}
    ${XR_RENDERPC_R4_BLENDER_SOURCES}
    ${XR_RENDERPC_R4_UTILITY_SOURCES}
    ${XR_RENDERPC_R4_DX11_SOURCES}
    ${XR_RENDERPC_R4_R2_SOURCES}
    ${XR_IMGUI_SOURCES}
)

target_include_directories(xrRenderPC_R4
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender_R2"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRender"
    "${CMAKE_SOURCE_DIR}/src/Layers/xrRenderDX11"
    "${XR_IMGUI_DIR}"
    "${CMAKE_SOURCE_DIR}/sdk/include"
    "${CMAKE_SOURCE_DIR}/sdk/include/DirectXMesh"
    "${CMAKE_SOURCE_DIR}/sdk/include/nvapi"
    "${CMAKE_SOURCE_DIR}/Externals/AGS_SDK"
    "${CMAKE_SOURCE_DIR}/Externals/AGS_SDK/ags_lib/inc"
    "${CMAKE_SOURCE_DIR}/src/packages/directxtex_desktop_2019.2024.10.29.1/include"
)

target_link_libraries(xrRenderPC_R4
    PRIVATE
    xrCore
    xrCDB
    xrEngine
    xrMaterialSystem
    xrParticles
    xrScriptEngine
    xrAPI
    xrMiscMath
)

if (MSVC)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_xr_dxtex_arch "x64")
        set(_xr_nvapi_lib "${CMAKE_SOURCE_DIR}/sdk/libraries/x64/nvapi64.lib")
        set(_xr_ags_debug_lib "${CMAKE_SOURCE_DIR}/Externals/AGS_SDK/ags_lib/lib/amd_ags_x64_2019_MDd.lib")
        set(_xr_ags_release_lib "${CMAKE_SOURCE_DIR}/Externals/AGS_SDK/ags_lib/lib/amd_ags_x64_2019_MD.lib")
    else()
        set(_xr_dxtex_arch "x86")
        set(_xr_nvapi_lib "${CMAKE_SOURCE_DIR}/sdk/libraries/x86/nvapi32.lib")
        set(_xr_ags_debug_lib "${CMAKE_SOURCE_DIR}/Externals/AGS_SDK/ags_lib/lib/amd_ags_x86_2019_MDd.lib")
        set(_xr_ags_release_lib "${CMAKE_SOURCE_DIR}/Externals/AGS_SDK/ags_lib/lib/amd_ags_x86_2019_MD.lib")
    endif()

    set(_xr_dxtex_root "${CMAKE_SOURCE_DIR}/src/packages/directxtex_desktop_2019.2024.10.29.1/native/lib/${_xr_dxtex_arch}")

    target_link_libraries(xrRenderPC_R4
        PRIVATE
        $<$<CONFIG:Debug>:${_xr_dxtex_root}/Debug/DirectXTex.lib>
        $<$<NOT:$<CONFIG:Debug>>:${_xr_dxtex_root}/Release/DirectXTex.lib>
        ${_xr_nvapi_lib}
        $<$<CONFIG:Debug>:${_xr_ags_debug_lib}>
        $<$<NOT:$<CONFIG:Debug>>:${_xr_ags_release_lib}>
        d3d11.lib
        dxgi.lib
        dxguid.lib
        d3dcompiler.lib
    )
endif()

target_compile_definitions(xrRenderPC_R4
    PRIVATE
    XRRENDER_R4_EXPORTS
    USE_DX11
    RENDER_NAMESPACE=render_r4
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
