add_library(xrRenderPC_R5 SHARED)

target_sources(xrRenderPC_R5 PRIVATE
    ComputeShader.cpp
    ComputeShader.h
    CSCompiler.cpp
    CSCompiler.h
    packages.config
    r2_test_hw.cpp
    r5_rendertarget.h
    r5_rendertarget_accum_direct.cpp
    r5_rendertarget_build_textures.cpp
    r5_rendertarget_phase_combine.cpp
    r5_rendertarget_phase_hdao.cpp
    r5_rendertarget_u_set_rt.cpp
    r5_shaders.cpp
    R_Backend_LOD.cpp
    R_Backend_LOD.h
    stdafx.cpp
    stdafx.h
    xrRender_r5.cpp
)

target_include_directories(xrRenderPC_R5
    PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/sdk/include"
)

target_link_libraries(xrRenderPC_R5
    PRIVATE
    xrCore
    xrCDB
    xrEngine
    xrParticles
    xrScriptEngine
    xrAPI
    xrMiscMath
)

target_compile_definitions(xrRenderPC_R5
    PRIVATE
    XRRENDER_r5_EXPORTS
)

set_target_properties(xrRenderPC_R5 PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrRenderPC_R5
    PRIVATE
    stdafx.h
)

install(TARGETS xrRenderPC_R5 LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
