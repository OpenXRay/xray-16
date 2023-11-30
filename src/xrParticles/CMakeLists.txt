project(xrParticles)

set(KERNEL_SRC
    "psystem.h"
    "stdafx.h"
    "stdafx.cpp"
)

set(PARTICLE_API_SRC
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

source_group("Kernel" FILES ${KERNEL_SRC})
source_group("ParticleAPI" FILES ${PARTICLE_API_SRC})

add_library(${PROJECT_NAME} ${LIBS_LINK_TYPE}
    ${KERNEL_SRC}
    ${PARTICLE_API_SRC}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    xrCore
    xrMiscMath
    xrEngine
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DXR_PARTICLES_EXPORTS
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    "stdafx.h"
)

install(TARGETS ${PROJECT_NAME} LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
