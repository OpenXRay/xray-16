project(xrCDB)

set(SRC_FILES
    "Frustum.h"
    "Frustum.cpp"
    "Intersect.hpp"
    "ISpatial.h"
    "ISpatial.cpp"
    "ISpatial_q_box.cpp"
    "ISpatial_q_frustum.cpp"
    "ISpatial_q_ray.cpp"
    "ISpatial_verify.cpp"
    "stdafx.h"
    "StdAfx.cpp"
    "xr_area.h"
    "xr_area.cpp"
    "xr_area_query.cpp"
    "xr_area_raypick.cpp"
    "xrCDB_box.cpp"
    "xrCDB_Collector.cpp"
    "xrCDB.cpp"
    "xrCDB_frustum.cpp"
    "xrCDB.h"
    "xrCDB_ray.cpp"
    "xr_collide_defs.h"
    "xrXRC.h"
    "xrXRC.cpp"
)

group_sources(SRC_FILES)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SDL_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    xrCore
    xrMiscMath
    OPCODE
    xrAPI
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DXRCDB_EXPORTS
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
