#pragma once

#include "xrCore/xrCore.h"
#include "Bindless/UnifiedVertex.h"

namespace xray::render::fg
{
class dxRender_Visual;

#pragma pack(push, 4)
struct ClusterBoundsKey {
    float x, y, z, r, e;

    bool operator==(const ClusterBoundsKey& o) const {
        return memcmp(this, &o, sizeof(*this)) == 0;
    }
    bool operator<(const ClusterBoundsKey& o) const {
        return memcmp(this, &o, sizeof(*this)) < 0;
    }
};

struct ClusterMetaProto {
    float sphere[4];
    float lodSelf[4];
    float lodParent[4];
    u32 indexCount;
    u32 ibFirst;
    u32 depth;
    u32 flags;
    float selfError;
    float parentError;
};
#pragma pack(pop)

static_assert(sizeof(ClusterMetaProto) == 72, "ClusterMetaProto layout is cache-serialized");

struct ClusterMeshKey {
    u32 vertexOffset;
    u32 indexOffset;
    u32 vertexCount;
    u32 indexCount;

    bool operator==(const ClusterMeshKey& o) const {
        return vertexOffset == o.vertexOffset && indexOffset == o.indexOffset &&
               vertexCount == o.vertexCount && indexCount == o.indexCount;
    }
    bool operator<(const ClusterMeshKey& o) const {
        if (vertexOffset != o.vertexOffset) return vertexOffset < o.vertexOffset;
        if (indexOffset != o.indexOffset) return indexOffset < o.indexOffset;
        if (vertexCount != o.vertexCount) return vertexCount < o.vertexCount;
        return indexCount < o.indexCount;
    }
};

struct ClusterMeshRecord {
    ClusterMeshKey key;
    u32 firstProto;
    u32 protoCount;
    u32 firstIndex;
    u32 indexTotal;
};

struct ClusterBakeStats {
    u32 eligibleMeshes;
    u32 bakedMeshes;
    u32 clusters;
    u32 droppedSelfLoops;
    u32 holes;
    u32 maxDepth;
    u32 levelCounts[16];
    u32 histInf, hist100, hist10, hist1, hist01, histSmall;
    u64 bakedIndexCount;
    u32 bakeMs;
};

class ClusterDAG {
public:
    struct MeshRange {
        ClusterMeshKey key;
    };

    void Bake(
        const xr_vector<ClusterMeshKey>& ranges,
        const bindless::UnifiedVertex* megaVertices,
        const u32* megaIndices);

    void Clear();

    bool Empty() const { return m_records.empty(); }
    const ClusterMeshRecord* FindRecord(const ClusterMeshKey& key) const;
    const xr_vector<ClusterMetaProto>& Protos() const { return m_protos; }
    const xr_vector<u32>& BakedIndices() const { return m_bakedIndices; }
    const ClusterBakeStats& Stats() const { return m_stats; }

    void SetMegaIndexBase(u32 base) { m_megaIndexBase = base; }
    u32 MegaIndexBase() const { return m_megaIndexBase; }
    void ReleaseIndexData() { m_bakedIndices.clear(); m_bakedIndices.shrink_to_fit(); }

private:
    xr_vector<ClusterMeshRecord> m_records;
    xr_vector<ClusterMetaProto> m_protos;
    xr_vector<u32> m_bakedIndices;
    ClusterBakeStats m_stats = {};
    u32 m_megaIndexBase = 0;

    void RunDiagnostics();
};

} // namespace xray::render::fg
