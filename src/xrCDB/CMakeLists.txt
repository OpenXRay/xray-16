project(xrCDB)

set(ENGINE_SRC
    "ISpatial.cpp"
    "ISpatial.h"
    "ISpatial_q_box.cpp"
    "ISpatial_q_frustum.cpp"
    "ISpatial_q_ray.cpp"
    "ISpatial_verify.cpp"
    "xr_area.cpp"
    "xr_area.h"
    "xr_area_query.cpp"
    "xr_area_raypick.cpp"
    "xr_collide_defs.h"
    "xrXRC.cpp"
    "xrXRC.h"
)

set(KERNEL_SRC
    "Frustum.cpp"
    "Frustum.h"
    "Intersect.hpp"
    "stdafx.h"
    "StdAfx.cpp"
    "xrCDB.cpp"
    "xrCDB.h"
    "xrCDB_box.cpp"
    "xrCDB_Collector.cpp"
    "xrCDB_frustum.cpp"
    "xrCDB_ray.cpp"
)

source_group("engine" FILES ${ENGINE_SRC})
source_group("Kernel" FILES ${KERNEL_SRC})

set(SRC_FILES
    ${ENGINE_SRC}
    ${KERNEL_SRC}
)

add_library(${PROJECT_NAME} ${LIBS_LINK_TYPE} ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    xrCore
    xrMiscMath
    xrOPCODE
    xrAPI
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DXRCDB_EXPORTS
    $<$<BOOL:${STATIC_BUILD}>:OPCODE_STATIC>
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
