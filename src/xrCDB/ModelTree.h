#pragma once

#ifdef XRAY_USE_JOLT_CDB
#    include <Jolt/Jolt.h>

#    include <Jolt/AABBTree/AABBTreeBuilder.h>

#    include "xrCDB.h"

namespace CDB
{
class ModelTree
{
    JPH::Array<JPH::AABBTreeBuilder::Node> nodes;
    JPH::Array<u32> triangleIds;

    template <typename BoundsTest, typename TriangleTest>
    bool Visit(u32 index, u32 mask, BoundsTest& boundsTest, TriangleTest& triangleTest) const
    {
        const auto& node = nodes[index];
        if (!boundsTest(node.mBounds, mask))
            return false;

        if (node.HasChildren())
        {
            return Visit(node.mChild[0], mask, boundsTest, triangleTest) || Visit(node.mChild[1], mask, boundsTest, triangleTest);
        }

        for (u32 i = 0; i < node.mNumTriangles; i++)
        {
            if (triangleTest(triangleIds[node.mTrianglesBegin + i]))
                return true;
        }
        return false;
    }

public:
    bool Build(const Fvector* vertices, u32 vertexCount, const TRI* triangles, u32 triangleCount);
    size_t GetUsedBytes() const;

    template <typename BoundsTest, typename TriangleTest>
    void Query(BoundsTest boundsTest, TriangleTest triangleTest, u32 mask = 0) const
    {
        if (!nodes.empty())
            Visit(0, mask, boundsTest, triangleTest);
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
