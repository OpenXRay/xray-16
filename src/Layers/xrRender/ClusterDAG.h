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
    u32 member;
    u32 reserved;
};
#pragma pack(pop)

static_assert(sizeof(ClusterMetaProto) == 80, "ClusterMetaProto layout is cache-serialized");

constexpr u32 CLUSTER_PROTO_FLAG_AT = 1u << 0;
constexpr u32 CLUSTER_PROTO_FLAG_TERRAIN = 1u << 1;

constexpr u32 CLUSTER_RANGE_FLAG_AT = 1u << 0;
constexpr u32 CLUSTER_RANGE_FLAG_MERGEABLE = 1u << 1;
constexpr u32 CLUSTER_RANGE_FLAG_TERRAIN = 1u << 2;

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

struct ClusterBakeRange {
    ClusterMeshKey key;
    u32 flags;
};

struct ClusterUnitRecord {
    u32 firstMember;
    u32 memberCount;
    u32 firstProto;
    u32 protoCount;
    u32 firstIndex;
    u32 indexTotal;
    u32 isComponent;
};

struct ClusterBakeStats {
    u32 eligibleMeshes;
    u32 bakedMeshes;
    u32 components;
    u32 componentMembers;
    u32 orphansAttached;
    u32 capSplits;
    u32 pinnedVerts;
    u32 clusters;
    u32 terrainMeshes;
    u32 terrainComponents;
    u32 terrainClusters;
    u32 droppedSelfLoops;
    u32 holes;
    u32 invalidRanges;
    u32 bakeFailed;
    u32 smallStandalone;
    u32 orphanStandalone;
    u32 maxDepth;
    u32 levelCounts[16];
    u32 histInf, hist100, hist10, hist1, hist01, histSmall;
    u64 bakedIndexCount;
    u32 bakeMs;
};

class ClusterDAG {
public:
    struct MemberRef {
        u32 record;
        u32 member;
    };

    void Bake(
        const xr_vector<ClusterBakeRange>& ranges,
        const bindless::UnifiedVertex* megaVertices,
        const u32* megaIndices);

    bool TryLoadCache(const char* path, u64 geomStamp, const xr_vector<ClusterBakeRange>& ranges);
    void SaveCache(const char* path, u64 geomStamp, const xr_vector<ClusterBakeRange>& ranges) const;

    void Clear();

    bool Empty() const { return m_records.empty(); }
    const ClusterUnitRecord* FindRecord(const ClusterMeshKey& key, u32& outMember) const;
    const xr_vector<ClusterUnitRecord>& Records() const { return m_records; }
    const xr_vector<ClusterMeshKey>& MemberKeys() const { return m_memberKeys; }
    const xr_vector<ClusterMetaProto>& Protos() const { return m_protos; }
    const xr_vector<u32>& BakedIndices() const { return m_bakedIndices; }
    const ClusterBakeStats& Stats() const { return m_stats; }

    void SetMegaIndexBase(u32 base) { m_megaIndexBase = base; }
    u32 MegaIndexBase() const { return m_megaIndexBase; }
    void ReleaseIndexData() { m_bakedIndices.clear(); m_bakedIndices.shrink_to_fit(); }

private:
    struct LookupEntry {
        ClusterMeshKey key;
        u32 record;
        u32 member;
    };

    xr_vector<ClusterUnitRecord> m_records;
    xr_vector<ClusterMeshKey> m_memberKeys;
    xr_vector<LookupEntry> m_lookup;
    xr_vector<ClusterMetaProto> m_protos;
    xr_vector<u32> m_bakedIndices;
    ClusterBakeStats m_stats = {};
    u32 m_megaIndexBase = 0;

    void BuildLookup();
    void RunDiagnostics();
};

} // namespace xray::render::fg
