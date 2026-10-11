#include "stdafx.h"
#include "ModelTree.h"

#include <Jolt/TriangleSplitter/TriangleSplitterBinning.h>
#include <Jolt/AABBTree/TriangleCodec/TriangleCodecIndexed8BitPackSOA4Flags.h>
#include <cstdarg>
#include <mutex>

namespace CDB
{
static void TraceJolt(pcstr format, ...)
{
    char message[1024];
    va_list arguments;
    va_start(arguments, format);
    std::vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    Msg("* Jolt: %s", message);
}

bool ModelTree::Build(const Fvector* vertices, u32 vertexCount, const TRI* triangles, u32 triangleCount)
{
    if (!vertices || !triangles || vertexCount == 0 || triangleCount == 0)
        return false;

    static std::once_flag initialize;
    std::call_once(initialize,
        []
        {
            JPH::Trace = TraceJolt;
        });

    JPH::VertexList points;
    points.reserve(vertexCount);
    for (u32 i = 0; i < vertexCount; i++)
    {
        const auto& point = vertices[i];
        if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z))
            return false;
        points.emplace_back(point.x, point.y, point.z);
    }

    JPH::IndexedTriangleList indices;
    indices.reserve(triangleCount);
    for (u32 i = 0; i < triangleCount; i++)
    {
        const auto& triangle = triangles[i];
        if (triangle.verts[0] >= vertexCount || triangle.verts[1] >= vertexCount || triangle.verts[2] >= vertexCount)
            return false;
        indices.emplace_back(triangle.verts[0], triangle.verts[1], triangle.verts[2], 0, i);
    }

    mesh = nullptr;
    degenerateTriangles.clear();
    JPH::TriangleCodecIndexed8BitPackSOA4Flags::ValidationContext validation(indices, points);
    JPH::IndexedTriangleList simulationTriangles;
    simulationTriangles.reserve(triangleCount);
    JPH::Ref<JPH::XRayMeshMaterial> metadata = new JPH::XRayMeshMaterial;
    metadata->materials.reserve(triangleCount);
    u16 maxMaterial = 0;
    for (u32 index = 0; index < triangleCount; ++index)
        maxMaterial = std::max(maxMaterial, static_cast<u16>(triangles[index].material));
    metadata->materialBounds.resize(size_t(maxMaterial) + 1);
    for (u32 index = 0; index < triangleCount; ++index) {
        metadata->materials.push_back(static_cast<u16>(triangles[index].material));
        const auto& triangle = indices[index];
        auto& materialBounds = metadata->materialBounds[triangles[index].material];
        for (const auto vertex : triangle.mIdx)
            materialBounds.Encapsulate(JPH::Vec3::sLoadFloat3Unsafe(points[vertex]));
        if (triangle.IsDegenerate(points) || validation.IsDegenerate(triangle)) {
            JPH::AABox bounds;
            for (const auto vertex : triangle.mIdx) bounds.Encapsulate(JPH::Vec3::sLoadFloat3Unsafe(points[vertex]));
            degenerateTriangles.push_back({bounds, index});
        } else simulationTriangles.push_back(triangle);
    }
    if (!simulationTriangles.empty()) {
        // Do not sanitize away coincident triangles: original IDs and metadata
        // are observable through CDB queries and must all remain available.
        JPH::MeshShapeSettings settings;
        settings.mTriangleVertices = std::move(points);
        settings.mIndexedTriangles = std::move(simulationTriangles);
        settings.mMaterials.emplace_back(metadata.GetPtr());
        settings.mPerTriangleUserData = true;
        settings.mMaxTrianglesPerLeaf = 4;
        settings.mUserData = JPH::XRayMeshMaterial::Tag;
        const auto result = settings.Create();
        if (result.HasError()) { Msg("! Jolt level mesh: %s", result.GetError().c_str()); return false; }
        mesh = static_cast<const JPH::MeshShape*>(result.Get().GetPtr());
    }
    return true;
}

size_t ModelTree::GetUsedBytes() const
{
    size_t bytes = degenerateTriangles.capacity() * sizeof(DegenerateTriangle);
    if (mesh) {
        bytes += mesh->GetStats().mSizeBytes;
        bytes += static_cast<const JPH::XRayMeshMaterial*>(mesh->GetMaterialList().front().GetPtr())->materials.capacity() * sizeof(u16);
        bytes += static_cast<const JPH::XRayMeshMaterial*>(mesh->GetMaterialList().front().GetPtr())->materialBounds.capacity() * sizeof(JPH::AABox);
    }
    return bytes;
}
}
