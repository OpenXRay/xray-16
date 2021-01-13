project(xrParticles)

set(SRC_FILES
    "noise.h"
    "noise.cpp"
    "particle_actions_collection.h"
    "particle_actions_collection.cpp"
    "particle_actions_collection_io.cpp"
    "particle_actions.h"
    "particle_actions.cpp"
    "particle_core.h"
    "particle_core.cpp"
    "particle_effect.h"
    "particle_effect.cpp"
    "particle_manager.h"
    "particle_manager.cpp"
    "psystem.h"
    "stdafx.h"
    "stdafx.cpp"
)

group_sources(SRC_FILES)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/Externals/sse2neon/include
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
    COTIRE_CXX_PREFIX_HEADER_INIT "stdafx.h"
)

cotire(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
