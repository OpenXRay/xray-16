add_library(xrRenderPC_R2 SHARED)

target_sources(xrRenderPC_R2 PRIVATE
    Light_Render_Direct_ComputeXFS.cpp
    r2_rendertarget.h
    r2_rendertarget_accum_direct.cpp
    r2_rendertarget_accum_point.cpp
    r2_rendertarget_accum_spot.cpp
    r2_rendertarget_build_textures.cpp
    r2_rendertarget_phase_combine.cpp
    r2_rendertarget_phase_occq.cpp
    r2_rendertarget_phase_scene.cpp
    r2_rendertarget_phase_smap_D.cpp
    r2_rendertarget_phase_ssao.cpp
    r2_rendertarget_u_set_rt.cpp
    r2_shaders.cpp
    r2_test_hw.cpp
    stdafx.cpp
    stdafx.h
    xrRender_R2.cpp
)

target_include_directories(xrRenderPC_R2
    PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/sdk/include"
)

target_link_libraries(xrRenderPC_R2
    PRIVATE
    xrCore
    xrCDB
    xrEngine
    xrParticles
    xrScriptEngine
    xrAPI
    xrMiscMath
    DXSDK::D3D9
)

target_compile_definitions(xrRenderPC_R2
    PRIVATE
    XRRENDER_R2_EXPORTS
    _USRDLL
    USE_DX9
)

set_target_properties(xrRenderPC_R2 PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrRenderPC_R2
    PRIVATE
    stdafx.h
)

install(TARGETS xrRenderPC_R2 LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
