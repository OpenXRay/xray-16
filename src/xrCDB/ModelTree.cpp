#include "stdafx.h"
#include "ModelTree.h"

#include <Jolt/TriangleSplitter/TriangleSplitterBinning.h>
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

    JPH::TriangleSplitterBinning splitter(points, indices);
    JPH::AABBTreeBuilder builder(splitter, 4);
    JPH::AABBTreeBuilderStats stats;
    builder.Build(stats);

    nodes = builder.GetNodes();
    triangleIds.clear();
    triangleIds.reserve(triangleCount);
    for (const auto& triangle : builder.GetTriangles())
        triangleIds.push_back(triangle.mUserData);
    return true;
}

size_t ModelTree::GetUsedBytes() const
{
    return nodes.capacity() * sizeof(JPH::AABBTreeBuilder::Node) + triangleIds.capacity() * sizeof(u32);
}
}
