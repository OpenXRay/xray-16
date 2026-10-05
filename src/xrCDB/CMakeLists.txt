add_library(xrCDB)

option(XRAY_COLLISION_EMBREE "Enable Embree static ray-query routing" OFF)

target_sources_grouped(
    TARGET xrCDB
    NAME "engine"
    FILES
    ISpatial.cpp
    ISpatial.h
    ISpatial_q_box.cpp
    ISpatial_q_frustum.cpp
    ISpatial_q_ray.cpp
    ISpatial_verify.cpp
    xr_area.cpp
    xr_area.h
    xr_area_query.cpp
    xr_area_raypick.cpp
    xr_collide_defs.h
    xrXRC.cpp
    xrXRC.h
)

target_sources_grouped(
    TARGET xrCDB
    NAME "Kernel"
    FILES
    Frustum.cpp
    Frustum.h
    Intersect.hpp
    stdafx.h
    StdAfx.cpp
    ModelTree.cpp
    ModelTree.h
    xrCDB.cpp
    xrCDB.h
    xrCDB_box.cpp
    xrCDB_Collector.cpp
    xrCDB_frustum.cpp
    xrCDB_ray.cpp
)

if (XRAY_COLLISION_EMBREE)
    if (NOT XRAY_EMBREE_ROOT)
        message(FATAL_ERROR "Set XRAY_EMBREE_ROOT to the Embree SDK root")
    endif()
    target_sources_grouped(TARGET xrCDB NAME "Embree" FILES EmbreeModelQueries.cpp EmbreeModelQueries.h)
    target_include_directories(xrCDB PRIVATE "${XRAY_EMBREE_ROOT}/include")
    target_link_directories(xrCDB PRIVATE "${XRAY_EMBREE_ROOT}/lib")
    target_link_libraries(xrCDB PRIVATE embree4)
    target_compile_definitions(xrCDB PRIVATE XRAY_COLLISION_EMBREE=1)
endif()

target_include_directories(xrCDB
    PRIVATE
    "${XRAY_SOURCE_DIR}/src"
    "${XRAY_SOURCE_DIR}/Externals"
    "${XRAY_SOURCE_DIR}/Externals/imgui"
)

target_link_libraries(xrCDB
    PRIVATE
    xrCore
    xrMiscMath
    xrOPCODE
    xrAPI
)

target_compile_definitions(xrCDB
    PRIVATE
    XRCDB_EXPORTS
    BAN_OPCODE_AUTOLINK
)

set_target_properties(xrCDB PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrCDB
    PRIVATE
    stdafx.h
)

if (BUILD_SHARED_LIBS)
    install(TARGETS xrCDB LIBRARY
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()

option(XRAY_BUILD_CDB_TESTS "Build OPCODE collision and cache regression checks" OFF)
if (XRAY_BUILD_CDB_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
