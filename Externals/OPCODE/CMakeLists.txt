if (STATIC_BUILD)
    add_library(xrOPCODE STATIC)

    target_compile_definitions(xrOPCODE
        PRIVATE
        OPCODE_STATIC
    )
else()
    add_library(xrOPCODE SHARED)
endif()

target_sources(xrOPCODE PRIVATE
    OPC_AABBCollider.cpp
    OPC_AABBCollider.h
    OPC_AABB.cpp
    OPC_AABB.h
    OPC_AABBTree.cpp
    OPC_AABBTree.h
    OPC_BoundingSphere.h
    OPC_BoxBoxOverlap.h
    OPC_BVTCache.h
    OPC_Collider.cpp
    OPC_Collider.h
    OPC_Common.cpp
    OPC_Common.h
    OPC_Container.cpp
    OPC_Container.h
    OPC_FPU.h
    OPC_Matrix3x3.cpp
    OPC_Matrix3x3.h
    OPC_Matrix4x4.cpp
    OPC_Matrix4x4.h
    OPC_MemoryMacros.h
    OPC_Model.cpp
    OPC_Model.h
    OPC_OBBCollider.cpp
    OPC_OBBCollider.h
    OPC_OBB.cpp
    OPC_OBB.h
    Opcode.cpp
    Opcode.h
    OPC_OptimizedTree.cpp
    OPC_OptimizedTree.h
    OPC_Plane.cpp
    OPC_Plane.h
    OPC_PlanesAABBOverlap.h
    OPC_PlanesCollider.cpp
    OPC_PlanesCollider.h
    OPC_PlanesTriOverlap.h
    OPC_Point.cpp
    OPC_Point.h
    OPC_Preprocessor.h
    OPC_RayAABBOverlap.h
    OPC_RayCollider.cpp
    OPC_RayCollider.h
    OPC_Ray.cpp
    OPC_Ray.h
    OPC_RayTriOverlap.h
    OPC_Settings.h
    OPC_SphereAABBOverlap.h
    OPC_SphereCollider.cpp
    OPC_SphereCollider.h
    OPC_SphereTriOverlap.h
    OPC_TreeBuilders.cpp
    OPC_TreeBuilders.h
    OPC_TreeCollider.cpp
    OPC_TreeCollider.h
    OPC_Triangle.cpp
    OPC_Triangle.h
    OPC_TriBoxOverlap.h
    OPC_TriTriOverlap.h
    OPC_Types.h
    OPC_VolumeCollider.cpp
    OPC_VolumeCollider.h
    pch.cpp
    pch.hpp
)

target_include_directories(xrOPCODE
    PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(xrOPCODE
    PRIVATE
    xrCore
)

target_compile_definitions(xrOPCODE
    PRIVATE
    OPCODE_EXPORTS
)

set_target_properties(xrOPCODE PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON
)

target_precompile_headers(xrOPCODE
    PRIVATE
    pch.hpp
)

if (NOT STATIC_BUILD)
    install(TARGETS xrOPCODE LIBRARY
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
