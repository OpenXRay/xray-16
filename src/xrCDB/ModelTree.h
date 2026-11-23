#pragma once

#ifdef XRAY_USE_JOLT_CDB
#    include <Jolt/Jolt.h>

#    include "Jolt-proj/XRayMeshQuery.h"

#    include "xrCDB.h"
#    include <limits>

namespace CDB
{
class ModelTree
{
    JPH::RefConst<JPH::MeshShape> mesh;
    struct DegenerateTriangle { JPH::AABox bounds; u32 id; };
    JPH::Array<DegenerateTriangle> degenerateTriangles;

public:
    bool Build(const Fvector* vertices, u32 vertexCount, const TRI* triangles, u32 triangleCount);
    size_t GetUsedBytes() const;
    void* AcquirePhysicsShape() const {
        if (!mesh) return nullptr;
        mesh->AddRef();
        return const_cast<JPH::MeshShape*>(mesh.GetPtr());
    }

    template <typename BoundsTest, typename TriangleTest>
    void QueryRay(BoundsTest boundsTest, TriangleTest triangleTest, JPH::Vec3Arg origin,
        JPH::Vec3Arg direction, float& range) const
    {
        if (range == std::numeric_limits<float>::max()) {
            Query(boundsTest, triangleTest);
            return;
        }
        if (mesh && JPH::XRayVisitMeshRay(*mesh, &triangleTest, origin, direction, range,
            +[](void* state, u32 id) { return (*static_cast<TriangleTest*>(state))(id); })) return;
        QueryDegenerate(boundsTest, triangleTest, 0);
    }
    template <typename BoundsTest, typename TriangleTest>
    void QueryBox(BoundsTest boundsTest, TriangleTest triangleTest, JPH::Vec3Arg minimum,
        JPH::Vec3Arg maximum) const
    {
        if (mesh && JPH::XRayVisitMeshBox(*mesh, &triangleTest, minimum, maximum,
            +[](void* state, u32 id) { return (*static_cast<TriangleTest*>(state))(id); })) return;
        QueryDegenerate(boundsTest, triangleTest, 0);
    }
    template <typename BoundsTest, typename TriangleTest>
    void Query(BoundsTest boundsTest, TriangleTest triangleTest, u32 mask = 0) const
    {
        struct Context { BoundsTest& bounds; TriangleTest& triangle; } context{boundsTest, triangleTest};
        if (mesh && JPH::XRayVisitMesh(*mesh, &context,
            +[](void* state, const JPH::AABox& bounds, u32& stateMask) {
                return static_cast<Context*>(state)->bounds(bounds, stateMask);
            }, +[](void* state, u32 id) { return static_cast<Context*>(state)->triangle(id); }, mask)) return;
        QueryDegenerate(boundsTest, triangleTest, mask);
    }
private:
    template <typename BoundsTest, typename TriangleTest>
    void QueryDegenerate(BoundsTest& boundsTest, TriangleTest& triangleTest, u32 mask) const
    {
        // Degenerate triangles cannot enter a simulation mesh. Retain their
        // original query behavior without building a second spatial tree.
        for (const auto& triangle : degenerateTriangles) {
            auto stateMask = mask;
            if (boundsTest(triangle.bounds, stateMask) && triangleTest(triangle.id)) return;
        }
    }
};
}
#else
#    include "OPCODE/Opcode.h"

namespace CDB
{
class ModelTree : public Opcode::OPCODE_Model
{};
}
#endif
