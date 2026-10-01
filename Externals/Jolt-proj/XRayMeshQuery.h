#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/SubShapeID.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

namespace JPH
{
// The material is owned by the mesh, so metadata survives every shared shape
// reference. Original triangle IDs and all 14 material bits remain separate.
class XRayMeshMaterial final : public PhysicsMaterial
{
public:
    static constexpr uint64 Tag = 0x585241594d455348ULL;
    Array<uint16> materials;
    // Conservative bounds per material let gameplay prequeries skip regions
    // without slowdown volumes without introducing a second collision BVH.
    Array<AABox> materialBounds;
    const char* GetDebugName() const override { return "X-Ray triangle materials"; }
};
inline uint16 XRayTriangleMaterial(const Shape* shape, SubShapeID id)
{
    SubShapeID remainder;
    const auto* leaf = shape->GetLeafShape(id, remainder);
    if (leaf->GetSubType() != EShapeSubType::Mesh || leaf->GetUserData() != XRayMeshMaterial::Tag)
        return uint16(-1);
    const auto* mesh = static_cast<const MeshShape*>(leaf);
    const auto* metadata = static_cast<const XRayMeshMaterial*>(mesh->GetMaterial(remainder));
    const auto triangle = mesh->GetTriangleUserData(remainder);
    return triangle < metadata->materials.size() ? metadata->materials[triangle] : uint16(-1);
}
inline uint32 XRayTriangleIndex(const Shape* shape, SubShapeID id)
{
    SubShapeID remainder;
    const auto* leaf = shape->GetLeafShape(id, remainder);
    return leaf->GetSubType() == EShapeSubType::Mesh && leaf->GetUserData() == XRayMeshMaterial::Tag ?
        static_cast<const MeshShape*>(leaf)->GetTriangleUserData(remainder) : uint32(-1);
}
using XRayMeshBoundsCallback = bool(*)(void*, const AABox&, uint32&);
using XRayMeshTriangleCallback = bool(*)(void*, uint32);
// Traverse the actual simulation BVH while testing original, uncompressed
// triangles in the engine callback. Return true for an early-out query.
bool XRayVisitMesh(const MeshShape& mesh, void* context, XRayMeshBoundsCallback bounds,
    XRayMeshTriangleCallback triangle, uint32 mask);
// Specialized traversals test the four BVH children together and retain
// original triangle callbacks/order. Ray range is narrowed by exact hits.
bool XRayVisitMeshRay(const MeshShape& mesh, void* context, Vec3Arg origin,
    Vec3Arg direction, float& range, XRayMeshTriangleCallback triangle);
bool XRayVisitMeshBox(const MeshShape& mesh, void* context, Vec3Arg minimum,
    Vec3Arg maximum, XRayMeshTriangleCallback triangle);
// Optional filter for synchronous mesh overlap/sweep queries. The simulation
// and unrelated threads keep their ordinary collision behavior. Nested scopes
// restore the previous filter, including when a query exits early.
class XRayMeshTriangleFilter final
{
public:
    using Callback = bool(*)(void*, const MeshShape&, SubShapeID);
    XRayMeshTriangleFilter(void* state, Callback function);
    ~XRayMeshTriangleFilter();
    XRayMeshTriangleFilter(const XRayMeshTriangleFilter&) = delete;
    XRayMeshTriangleFilter& operator=(const XRayMeshTriangleFilter&) = delete;
    void* context;
    Callback callback;
private:
    const XRayMeshTriangleFilter* previous;
};
const XRayMeshTriangleFilter* XRayGetMeshTriangleFilter();
}
