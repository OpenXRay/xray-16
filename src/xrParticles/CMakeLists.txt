add_library(xrParticles SHARED)

target_sources_grouped(
    TARGET xrParticles
    NAME "Kernel"
    FILES
    "psystem.h"
    "stdafx.h"
    "stdafx.cpp"
)

target_sources_grouped(
    TARGET xrParticles
    NAME "ParticleAPI"
    FILES
    "noise.cpp"
    "noise.h"
    "particle_actions.cpp"
    "particle_actions.h"
    "particle_actions_collection.cpp"
    "particle_actions_collection.h"
    "particle_actions_collection_io.cpp"
    "particle_core.cpp"
    "particle_core.h"
    "particle_effect.cpp"
    "particle_effect.h"
    "particle_manager.cpp"
    "particle_manager.h"
)

target_include_directories(xrParticles
    PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/Externals"
)

target_link_libraries(xrParticles
    PRIVATE
    xrCore
    xrEngine
    xrMiscMath
)

target_compile_definitions(xrParticles
    PRIVATE
    XR_PARTICLES_EXPORTS
)

set_target_properties(xrParticles PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrParticles
    PRIVATE
    stdafx.h
)

install(TARGETS xrParticles LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
