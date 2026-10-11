#include "stdafx.h"
#include "xrCDB.h"
#include "ModelTree.h"

#include <cmath>
#include <memory>

namespace CDB
{
bool ModelTree::Build(Fvector* vertices, u32 vertexCount, TRI* triangles, u32 triangleCount)
{
    static_assert(sizeof(IceMaths::Point) == sizeof(Fvector));
    static_assert(sizeof(IceMaths::IndexedTriangle) == sizeof(u32) * 3);
    mesh.SetNbVertices(vertexCount);
    mesh.SetNbTriangles(triangleCount);
    if (!mesh.SetStrides(sizeof(TRI), sizeof(Fvector)) ||
        !mesh.SetPointers(reinterpret_cast<IceMaths::IndexedTriangle*>(triangles),
            reinterpret_cast<IceMaths::Point*>(vertices)))
        return false;

    Opcode::OPCODECREATE create;
    create.mIMesh = &mesh;
    create.mSettings.mLimit = 1;
    create.mSettings.mRules = Opcode::SPLIT_SPLATTER_POINTS | Opcode::SPLIT_GEOM_CENTER;
    create.mNoLeaf = true;
    create.mQuantized = false;
    create.mCanRemap = false; // Triangle IDs and their material/sector metadata are stable.
    create.mKeepOriginal = false;
    return Opcode::Model::Build(create);
}

bool ModelTree::Load(IReader* stream, u32 triangleCount, bool readCrc32, bool skipCrc32)
{
    // Decode the original non-quantized no-leaf representation, including
    // embedded 1.2 level.cform trees. Never install unvalidated pointer offsets.
    if (stream->elapsed() < sizeof(u32) * (readCrc32 ? 4 : 3))
        return false;
    const auto noLeaf = stream->r_u32();
    const auto quantized = stream->r_u32();
    const auto count = stream->r_u32();
    if (noLeaf != 1 || quantized != 0 || triangleCount < 2 || count != triangleCount - 1)
        return false;
    using Node = Opcode::AABBNoLeafNode;
    const size_t bytes = size_t(count) * sizeof(Node);
    const auto expectedCrc = readCrc32 ? stream->r_u32() : 0;
    if (bytes > stream->elapsed() || (readCrc32 && !skipCrc32 && crc32(stream->pointer(), bytes) != expectedCrc))
        return false;
    auto nodes = std::make_unique<Node[]>(count);
    std::memcpy(nodes.get(), stream->pointer(), bytes);
    for (u32 index = 0; index < count; ++index)
    {
        auto& node = nodes[index];
        for (u32 axis = 0; axis < 3; ++axis)
            if (!std::isfinite(node.mAABB.mCenter[axis]) || !std::isfinite(node.mAABB.mExtents[axis]) ||
                node.mAABB.mExtents[axis] < 0)
                return false;
        for (auto* child : { &node.mPosData, &node.mNegData })
        {
            if (*child & 1)
            {
                if ((*child >> 1) >= triangleCount)
                    return false;
            }
            else
            {
                // Both upstream builders place descendants after their parent.
                // Requiring that ordering also rejects cycles in damaged caches.
                if (*child % sizeof(Node) || *child >= bytes || *child / sizeof(Node) <= index)
                    return false;
                *child += reinterpret_cast<uintptr_t>(nodes.get());
            }
        }
    }
    auto restored = std::make_unique<Opcode::AABBNoLeafTree>();
    restored->SetData(nodes.release(), count);
    ReleaseBase();
    mTree = restored.release();
    mModelCode = Opcode::OPC_NO_LEAF;
    stream->advance(bytes);
    return true;
}

void ModelTree::Save(IWriter* stream) const
{
    R_ASSERT(mTree && !IsQuantized() && !HasLeafNodes());
    stream->w_u32(1);
    stream->w_u32(0);
    const u32 count = mTree->GetNbNodes();
    stream->w_u32(count);
    using Node = Opcode::AABBNoLeafNode;
    const size_t bytes = size_t(count) * sizeof(Node);
    auto nodes = std::make_unique<Node[]>(count);
    std::memcpy(nodes.get(), mTree->GetData(), bytes);
    const auto root = reinterpret_cast<uintptr_t>(mTree->GetData());
    for (u32 index = 0; index < count; ++index)
        for (auto* child : { &nodes[index].mPosData, &nodes[index].mNegData })
            if (!(*child & 1))
                *child -= root;
    stream->w_u32(crc32(nodes.get(), bytes));
    stream->w(nodes.get(), bytes);
}
}
