#pragma once

#include "xrCore/xrCore.h"
#include "Bindless/UnifiedVertex.h"
#include "ClusterShadowBVH.h"

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
    float extent[3];
    u32 indexCount;
    u32 ibFirst;
    u32 depth;
    u32 flags;
    float selfError;
    float parentError;
    u32 member;
    u32 owningGroup;
    u32 refinedGroup;
};

class ClusterGroupProto
{
public:
    float sphere[4];
    float error;
    u32 unit;
    u32 localGroup;
    u32 depth;
    u32 flags;
    u32 reserved;
};
#pragma pack(pop)

#pragma pack(push, 4)
class GPUClusterMeta
{
public:
    float sphere[4];
    float lodSelf[4];
    float lodParent[4];
    float extent[3];
    float selfError;
    float parentError;
    u32 page;
    u32 payloadOffset;
    u32 flags;
};

class GPUClusterAssetMember
{
public:
    float sphere[4];
    float extent[3];
    u32 firstCluster;
    u32 clusterCount;
    u32 firstNode;
    u32 nodeCount;
    u32 firstPage;
};

class GPUClusterPage
{
public:
    u32 vertexBase;
    u32 payloadBase;
    u32 vertexCount;
    u32 vertexBytes;
    u32 payloadBytes;
    u32 attributeMask;
    u32 vertexFormat;
    u32 assetMember;
};
#pragma pack(pop)

static_assert(sizeof(GPUClusterMeta) == 80, "GPUClusterMeta is shader-visible");
static_assert(sizeof(GPUClusterAssetMember) == 48, "GPUClusterAssetMember is shader-visible");
static_assert(sizeof(GPUClusterPage) == 32, "GPUClusterPage is shader-visible");

static_assert(sizeof(ClusterMetaProto) == 96, "ClusterMetaProto layout is cache-serialized");
static_assert(sizeof(ClusterGroupProto) == 40, "ClusterGroupProto layout is cache-serialized");

constexpr u32 CLUSTER_META_FLAG_AT = 1u;
constexpr u32 CLUSTER_META_FLAG_TERRAIN = 4u;
constexpr u32 CLUSTER_META_DEPTH_SHIFT = 8u;
constexpr u32 CLUSTER_META_TRIANGLE_SHIFT = 16u;
constexpr u32 CLUSTER_META_VERTEX_SHIFT = 23u;
constexpr u32 CLUSTER_META_COUNT_MASK = 0x7Fu;
constexpr u32 CLUSTER_PROTO_FLAG_AT = 1u << 0;
constexpr u32 CLUSTER_PROTO_FLAG_TERRAIN = 1u << 1;
constexpr u32 CLUSTER_GROUP_FLAG_TERMINAL = 1u << 0;

constexpr u32 CLUSTER_RANGE_FLAG_AT = 1u << 0;
constexpr u32 CLUSTER_RANGE_FLAG_MERGEABLE = 1u << 1;
constexpr u32 CLUSTER_RANGE_FLAG_TERRAIN = 1u << 2;


constexpr u32 CLUSTER_PAYLOAD_VERSION = 1u;
constexpr u32 CLUSTER_MAX_VERTICES = 128u;
constexpr u32 CLUSTER_MAX_TRIANGLES = 128u;
constexpr u32 CLUSTER_PAGE_ALIGNMENT = 16u;
constexpr u32 CLUSTER_PAGE_VERTEX_STRIDE = 32u;
constexpr u32 CLUSTER_PAGE_UV1_STRIDE = 8u;
constexpr u32 CLUSTER_PAGE_COLOR_STRIDE = 4u;
constexpr u32 CLUSTER_PAGE_FLAGS_STRIDE = 4u;
constexpr u32 CLUSTER_PAGE_MAX_VERTICES = 4096u;
constexpr u32 CLUSTER_PAGE_MAX_CLUSTERS = 256u;
constexpr u32 CLUSTER_PAGE_MAX_PAYLOAD_BYTES = 65536u;
constexpr u32 CLUSTER_PAYLOAD_TAIL_PAD = 4u;

constexpr u32 CLUSTER_PAGE_CLASS_ROOT = 0u;
constexpr u32 CLUSTER_PAGE_CLASS_FINE = 1u;
constexpr u32 CLUSTER_PAGE_CLASS_RUNTIME = 2u;

constexpr u32 CLUSTER_PAGE_STORE_MAGIC = 0x53474C43u;
constexpr u32 CLUSTER_PAGE_STORE_VERSION = 1u;

constexpr u32 CLUSTER_VERTEX_PACKED_BASIS = 0u;
constexpr u32 CLUSTER_VERTEX_FLOAT_NORMAL = 1u;

constexpr u32 CLUSTER_PAGE_ATTR_UV1 = 1u << 0;
constexpr u32 CLUSTER_PAGE_ATTR_COLOR = 1u << 1;
constexpr u32 CLUSTER_PAGE_ATTR_FLAGS = 1u << 2;

class ClusterSourceView
{
public:
    const bindless::UnifiedVertex* vertices = nullptr;
    const Fvector3* floatNormals = nullptr;
    const u32* indices = nullptr;
    u32 vertexBase = 0;
};

class ClusterGroupRecord
{
public:
    float sphere[4];
    float error;
    u32 depth;
    u32 flags;
    u32 firstMember;
    u32 memberCount;
    u32 firstReplacement;
    u32 replacementCount;
    u32 firstPage;
    u32 pageCount;
    u32 firstDependency;
    u32 dependencyCount;
    u32 firstMemberPage;
    u32 memberPageCount;
    u32 pinned;
};

class ClusterPayloadStats
{
public:
    u32 pages;
    u32 clusters;
    u32 maxPageVertices;
    u32 maxClusterVertices;
    u32 maxClusterTriangles;
    u32 reclusterSplits;
    u32 floatNormalPages;
    u32 uv1Pages;
    u32 colorPages;
    u32 flagPages;
    u32 groups;
    u32 groupDependencies;
    u32 multiPageGroups;
    u64 payloadBytes;
    u64 remapBytes;
    u64 triangleBytes;
    u64 paddingBytes;
    u64 vertexBaseBytes;
    u64 vertexOptionalBytes;
    u64 pageTableBytes;
    u64 distinctSourceVertices;
    u64 pageVertexSlots;
    u64 clusterVertexReferences;
    u64 replacedIndexBytes;
};

class ClusterPageStoreHeader
{
public:
    u32 magic;
    u32 version;
    u32 payloadVersion;
    u32 pageCount;
    u64 paramsHash;
    u64 geomStamp;
    u64 rangeSetHash;
    u64 layoutHash;
    u64 vertexBytes;
    u64 payloadBytes;
    u32 maxClusterVertices;
    u32 maxClusterTriangles;
    u32 vertexStride;
    u32 pageAlignment;
    u32 maxPageVertices;
    u32 maxPageClusters;
    u32 maxPagePayloadBytes;
    u32 reserved;
};

class ClusterResidencyStats
{
public:
    u32 rootPages = 0;
    u32 finePages = 0;
    u32 runtimePages = 0;
    u32 pinnedGroups = 0;
    u32 fineGroups = 0;
    u64 rootVertexBytes = 0;
    u64 rootPayloadBytes = 0;
    u64 fineVertexBytes = 0;
    u64 finePayloadBytes = 0;
    u64 runtimeVertexBytes = 0;
    u64 runtimePayloadBytes = 0;
    u32 maxPageVertexBytes = 0;
    u32 maxPagePayloadBytes = 0;
};

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
    u32 firstGroup;
    u32 groupCount;
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
    u32 memberPinnedVerts;
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
    u32 simplifiedGroups;
    u32 stalledGroups;
    u32 deviationRaised;
    float deviationMaxRaise;
    u32 metricRejectedGroups;
    u32 memberSplitViolations;
    u32 invalidProtos;
    u32 errorInversions;
    u32 lodViolations;
    u64 bakedIndexCount;
    u32 bakeMs;
    u32 exactLeafFallbacks;
    u32 runtimeLeafAssets;
};

class ClusterDAG {
public:
    struct MemberRef {
        u32 record;
        u32 member;
    };

    void Bake(
        const xr_vector<ClusterBakeRange>& ranges,
        const ClusterSourceView& source);

    bool TryLoadCache(const char* path, u64 geomStamp, const xr_vector<ClusterBakeRange>& ranges);
    void SaveCache(const char* path, u64 geomStamp, const xr_vector<ClusterBakeRange>& ranges) const;

    void Clear();

    bool Empty() const { return m_records.empty(); }
    const ClusterUnitRecord* FindRecord(const ClusterMeshKey& key, u32& outMember) const;
    bool FindAssetMember(const ClusterMeshKey& key, u32& outAssetMember) const;
    const xr_vector<ClusterUnitRecord>& Records() const { return m_records; }
    const xr_vector<ClusterMeshKey>& MemberKeys() const { return m_memberKeys; }
    const xr_vector<ClusterMetaProto>& Protos() const { return m_protos; }
    const ClusterBakeStats& Stats() const { return m_stats; }

    void BuildAssetTable(const ClusterSourceView& source);
    bool AppendRuntimeLeafAsset(
        const ClusterMeshKey& key,
        u32 rangeFlags,
        const ClusterSourceView& source,
        u32& outAssetMember);
    const xr_vector<GPUClusterMeta>& ClusterMeta() const { return m_clusterMeta; }
    const xr_vector<GPUClusterAssetMember>& AssetMembers() const { return m_assetMembers; }
    const xr_vector<GPUClusterAssetNode>& AssetNodes() const { return m_assetNodes; }
    const xr_vector<GPUClusterPage>& Pages() const { return m_pages; }
    const xr_vector<u8>& PageVertexData() const { return m_pageVertexData; }
    const xr_vector<u8>& PagePayloadData() const { return m_pagePayloadData; }

    const xr_vector<ClusterGroupRecord>& Groups() const { return m_groups; }
    const xr_vector<u32>& GroupMemberClusters() const { return m_groupMemberClusters; }
    const xr_vector<u32>& GroupReplacementClusters() const { return m_groupReplacementClusters; }
    const xr_vector<u32>& GroupPages() const { return m_groupPages; }
    const xr_vector<u32>& GroupDependencies() const { return m_groupDependencies; }
    const xr_vector<u32>& ClusterOwningGroups() const { return m_clusterOwningGroup; }
    const xr_vector<u32>& ClusterRefinedGroups() const { return m_clusterRefinedGroup; }
    const ClusterPayloadStats& PayloadStats() const { return m_payloadStats; }
    const xr_vector<u8>& PageClasses() const { return m_pageClass; }
    const xr_vector<u8>& RuntimePageVertexData() const { return m_runtimePageVertexData; }
    const xr_vector<u8>& RuntimePagePayloadData() const { return m_runtimePagePayloadData; }
    const xr_vector<u8>& GroupPinned() const { return m_groupPinned; }
    const xr_vector<u32>& GroupMemberPageList() const { return m_groupMemberPages; }
    const ClusterResidencyStats& ResidencyStats() const { return m_residencyStats; }

    bool WritePageStore(const char* path, u64 geomStamp, const xr_vector<ClusterBakeRange>& ranges);
    void ReleaseLevelPageBytes();
    void ReleaseRuntimePageBytes();
    bool LevelPageBytesResident() const { return !m_pageVertexData.empty() || !m_pagePayloadData.empty(); }
    u32 StorePageCount() const { return m_storePageCount; }
    u64 PageStoreVertexOrigin() const { return m_storeVertexOrigin; }
    u64 PageStorePayloadOrigin() const { return m_storePayloadOrigin; }
    u64 PageStoreIdentity() const { return m_storeIdentity; }
    u64 PageStoreVertexBytes() const { return m_storeVertexBytes; }
    u64 PageStorePayloadBytes() const { return m_storePayloadBytes; }

    u32 MaxAssetNodeDepth() const { return m_maxAssetNodeDepth; }
    u32 RecordOfAssetMember(u32 assetMember) const { return m_assetMemberRecord[assetMember]; }

private:
    struct LookupEntry {
        ClusterMeshKey key;
        u32 record;
        u32 member;
    };

    class PageBuilder
    {
    public:
        ClusterDAG* dag = nullptr;
        const ClusterSourceView* source = nullptr;
        xr_vector<u8>* vertexArena = nullptr;
        xr_vector<u8>* payloadArena = nullptr;
        u32 pageIndex = UINT32_MAX;
        u32 pageClass = CLUSTER_PAGE_CLASS_ROOT;
        u32 pageFormat = CLUSTER_VERTEX_PACKED_BASIS;
        u32 pageAssetMember = UINT32_MAX;
        u32 pageClusters = 0;
        u32 pagePayloadBytes = 0;
        xr_vector<u32> pageSourceVertices;
        xr_vector<u8> pagePayload;
        xr_unordered_map<u32, u32> pageSlotOf;
        xr_vector<u32> clusterVertices;
        xr_vector<unsigned char> clusterCorners;
        xr_vector<u32> absolute;

        void Flush();
        void Cook(u32 clusterIndex, u32 assetMember, u32 memberVertexBase,
            const u32* indices, u32 indexCount, u32 cookClass);

    private:
        const bindless::UnifiedVertex& SourceVertex(u32 absoluteIndex) const;
        const Fvector3* SourceNormal(u32 absoluteIndex) const;
    };

    xr_vector<ClusterUnitRecord> m_records;
    xr_vector<ClusterMeshKey> m_memberKeys;
    xr_vector<LookupEntry> m_lookup;
    xr_vector<ClusterMetaProto> m_protos;
    xr_vector<ClusterGroupProto> m_groupProtos;
    xr_vector<u32> m_bakedIndices;
    ClusterBakeStats m_stats = {};
    ClusterPayloadStats m_payloadStats = {};
    xr_vector<GPUClusterMeta> m_clusterMeta;
    xr_vector<GPUClusterAssetMember> m_assetMembers;
    xr_vector<GPUClusterAssetNode> m_assetNodes;
    xr_vector<GPUClusterPage> m_pages;
    xr_vector<u8> m_pageVertexData;
    xr_vector<u8> m_pagePayloadData;
    xr_vector<u8> m_runtimePageVertexData;
    xr_vector<u8> m_runtimePagePayloadData;
    xr_vector<u8> m_pageClass;
    xr_vector<u8> m_groupPinned;
    xr_vector<u32> m_clusterProto;
    xr_vector<u32> m_clusterOwningGroup;
    xr_vector<u32> m_clusterRefinedGroup;
    xr_vector<ClusterGroupRecord> m_groups;
    xr_vector<u32> m_groupMemberClusters;
    xr_vector<u32> m_groupReplacementClusters;
    xr_vector<u32> m_groupPages;
    xr_vector<u32> m_groupDependencies;
    xr_vector<u32> m_groupMemberPages;
    xr_vector<u32> m_assetMemberRecord;
    u32 m_maxAssetNodeDepth = 0;
    ClusterResidencyStats m_residencyStats = {};
    u32 m_storePageCount = 0;
    u64 m_storeVertexOrigin = 0;
    u64 m_storePayloadOrigin = 0;
    u64 m_storeIdentity = 0;
    u64 m_storeVertexBytes = 0;
    u64 m_storePayloadBytes = 0;

    void BuildLookup();
    bool RunDiagnostics();
    void BuildAssetMember(u32 recordIndex, u32 member);
    void CookPages(const ClusterSourceView& source, u32 firstMember, u32 firstCluster, bool runtime);
    void ClassifyGroups();
    void BuildResidencyStats();
    void BuildGroupRecords();
};

} // namespace xray::render::fg
