#pragma once

#include "xrCore/xrCore.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include "Layers/xrRender/ClusterDAG.h"

class IRenderBackend;

namespace xray::render::resources {
    class AsyncIOManager;
}

namespace xray::render::fg {

class RenderDevice;

constexpr u32 GEOMETRY_PAGE_BLOCK_BYTES = 4096u;
constexpr u32 GEOMETRY_PAGE_SLOT_INVALID = UINT32_MAX;

class GeometryDemandView
{
public:
    Fvector origin = {};
    Fvector direction = {};
    Fvector boundsCenter = {};
    float boundsRadius = 0.0f;
    float errorScale = 0.0f;
    float errorBound = 0.0f;
    float minDistance = 0.1f;
    Fmatrix worldToBounds = {};
    Fvector boundsMinimum = {};
    Fvector boundsMaximum = {};
    Fvector4 planes[6] = {};
    u32 planeCount = 0;
    bool orthographicBounds = false;
    bool valid = false;
};

class GeometryResidencyStats
{
public:
    u32 groups = 0;
    u32 pinnedGroups = 0;
    u32 residentGroups = 0;
    u32 desiredGroups = 0;
    u32 activatingGroups = 0;
    u32 blockedGroups = 0;

    u32 pages = 0;
    u32 pinnedPages = 0;
    u32 residentPages = 0;
    u32 readingPages = 0;
    u32 uploadingPages = 0;
    u32 retiringPages = 0;

    u32 uploadsRecorded = 0;
    u32 uploadKiB = 0;
    u32 readsIssued = 0;
    u32 readsFailed = 0;
    u32 uploadsDiscarded = 0;
    u32 allocationDeferrals = 0;
    u32 budgetDeferrals = 0;
    u32 evictions = 0;

    u64 vertexArenaBytes = 0;
    u64 payloadArenaBytes = 0;
    u64 vertexUsedBytes = 0;
    u64 payloadUsedBytes = 0;
    u64 pinnedVertexBytes = 0;
    u64 pinnedPayloadBytes = 0;
    u64 stagingBytes = 0;
    u64 retiringArenaBytes = 0;
    u32 pageBudgetMiB = 0;
    bool pagingRequested = false;

    u32 liveSnapshots = 0;
    u32 failedSnapshots = 0;
    u32 mappingGeneration = 0;
    u32 cutRevision = 0;
    bool streaming = false;
};

class GeometryCutBounds
{
public:
    Fvector4 minimum = {};
    Fvector4 maximum = {};
    u32 revision = 0;
    u32 reserved[3] = {};
};

static_assert(sizeof(GeometryCutBounds) == 48);

class GeometryResidencyManager
{
public:
    GeometryResidencyManager();
    ~GeometryResidencyManager();

    void Initialize(RenderDevice* device, resources::AsyncIOManager* io);
    void Shutdown();

    bool BeginLevel(ClusterDAG* dag, const char* storePath);
    void EndLevel();

    bool IsActive() const;
    bool IsStreaming() const;

    void RegisterRuntimePages();

    void BeginFrame(IRenderBackend* backend);
    void ClearDemand();
    void AddDemandView(const GeometryDemandView& view);
    void AddShadowDemand(const GeometryDemandView& view);
    void PromoteShadowDemand();
    void AddInstanceDemand(u32 assetMember, const Fmatrix& world, float scaleBound, bool plain);
    void ResolveDemand();
    void RecordUploads(nvrhi::ICommandList* cmdList);
    void EndFrame(IRenderBackend* backend);
    nvrhi::IBuffer* GetShadowCutBuffer() const;
    nvrhi::IBuffer* GetArenaCopySource(bool vertices) const;

    nvrhi::IBuffer* GetPageTableBuffer() const;
    nvrhi::IBuffer* GetGroupBitsBuffer() const;
    nvrhi::IBuffer* GetClusterGroupBuffer() const;
    nvrhi::IBuffer* GetVertexArenaBuffer() const;
    nvrhi::IBuffer* GetPayloadArenaBuffer() const;

    u32 GroupCount() const;
    u32 MappingGeneration() const;
    u32 CutRevision() const;
    const xr_vector<u32>& CutChangedGroups() const;

    const GeometryResidencyStats& Stats() const;

private:
    enum class PageState : u8
    {
        Absent,
        Reading,
        Staged,
        Uploading,
        Resident
    };

    class Arena
    {
    public:
        nvrhi::BufferHandle buffer;
        u64 bytes = 0;
        u32 blockCount = 0;
        u32 usedBlocks = 0;
        u32 cursor = 0;
        xr_vector<u64> freeMask;

        void Grow(u64 arenaBytes);
        void Reset(u64 arenaBytes);
        bool Allocate(u32 blocks, u32& outBlock);
        void Release(u32 block, u32 blocks);
        u64 UsedBytes() const;
    };

    class PageSlot
    {
    public:
        u32 vertexBlock = GEOMETRY_PAGE_SLOT_INVALID;
        u32 vertexBlocks = 0;
        u32 payloadBlock = GEOMETRY_PAGE_SLOT_INVALID;
        u32 payloadBlocks = 0;
        u64 vertexRead = 0;
        u64 payloadRead = 0;
        u32 refs = 0;
        u64 publishedSnapshot = 0;
        PageState state = PageState::Absent;
        bool pinned = false;
        bool published = false;
        bool cpuSourced = false;
        bool runtime = false;
        xr_vector<u8> vertexStaging;
        xr_vector<u8> payloadStaging;
    };

    class GroupSlot
    {
    public:
        u32 firstMemberPage = 0;
        u32 memberPageCount = 0;
        u32 firstDependency = 0;
        u32 dependencyCount = 0;
        u32 dependentResidents = 0;
        u32 lastDesiredFrame = 0;
        float sphere[4] = {};
        float error = 0.0f;
        bool pinned = false;
        bool resident = false;
        bool desired = false;
        bool requested = false;
        Fbox worldBounds = {};
        Fbox previousWorldBounds = {};
        bool worldBoundsValid = false;
        bool previousWorldBoundsValid = false;
    };

    class FrameRecord
    {
    public:
        u64 id = 0;
        u64 lease = 0;
        xr_vector<u32> uploadedPages;
        nvrhi::BufferHandle vertexCopySource;
        nvrhi::BufferHandle payloadCopySource;
    };

    class RetiringPage
    {
    public:
        u64 retireAfter = 0;
        u32 vertexBlock = GEOMETRY_PAGE_SLOT_INVALID;
        u32 vertexBlocks = 0;
        u32 payloadBlock = GEOMETRY_PAGE_SLOT_INVALID;
        u32 payloadBlocks = 0;
    };

    bool CreateBuffers(u64 vertexBytes, u64 payloadBytes);
    void ReleaseBuffers();
    void BuildMemberGroups();
    bool AllocatePage(u32 page);
    void IssuePageRead(u32 page);
    void StageFromCook(u32 page);
    bool TryActivateGroup(u32 group);
    void EvictGroup(u32 group);
    void RetirePage(u32 page);
    void PublishPage(u32 page);
    void UnpublishPage(u32 page);
    void SetGroupResident(u32 group, bool resident);
    void RefreshCoarseReady(u32 group);
    void MarkGroupWordDirty(u32 word);
    void MarkPageDirty(u32 page);
    void MarkAllTablesDirty();
    void UploadDirtyTables(nvrhi::ICommandList* cmdList);
    void CompleteFrames(IRenderBackend* backend);
    void DeactivateIncompleteGroups();
    void ReleaseCookBytesIfConfirmed();
    bool GroupDesiredForInstance(const GroupSlot& group, const Fvector& center, float radius, float scaleBound) const;
    const u8* CookVertexBytes(u32 page, u32& outSize) const;
    const u8* CookPayloadBytes(u32 page, u32& outSize) const;
    void UploadShadowCuts(nvrhi::ICommandList* cmdList);
    bool ReserveGroupPages(u32 group);
    void GrowArena(Arena& arena, u64 bytes, nvrhi::BufferHandle& source);

    RenderDevice* m_device = nullptr;
    resources::AsyncIOManager* m_io = nullptr;
    ClusterDAG* m_dag = nullptr;

    bool m_active = false;
    bool m_tablesInitialized = false;
    bool m_cookBytesReleased = false;
    u32 m_ioEpoch = 0;
    shared_str m_storePath;
    u64 m_storeIdentity = 0;
    u64 m_storeVertexOrigin = 0;
    u64 m_storePayloadOrigin = 0;
    u32 m_storePageCount = 0;

    Arena m_vertexArena;
    Arena m_payloadArena;

    nvrhi::BufferHandle m_pageTableBuffer;
    nvrhi::BufferHandle m_groupBitsBuffer;
    nvrhi::BufferHandle m_clusterGroupBuffer;
    nvrhi::BufferHandle m_shadowCutBuffers;
    xr_vector<GeometryCutBounds> m_cutBounds;
    xr_vector<GeometryCutBounds> m_cutStaging;
    u32 m_cutStagingRevision = 0;
    nvrhi::BufferHandle m_vertexCopySource;
    nvrhi::BufferHandle m_payloadCopySource;
    xr_vector<u32> m_reservedPages;
    xr_vector<u32> m_demandClosure;

    xr_vector<GPUClusterPage> m_gpuPages;
    xr_vector<PageSlot> m_pageSlots;
    xr_vector<GroupSlot> m_groupSlots;
    xr_vector<u32> m_groupBits;
    xr_vector<u32> m_clusterGroupTable;
    xr_vector<u32> m_dirtyPages;
    xr_vector<u32> m_dirtyGroupWords;
    xr_vector<u32> m_pendingActivation;
    xr_vector<u32> m_cutChangedGroups;
    xr_vector<u32> m_memberGroupOffset;
    xr_vector<u32> m_memberGroupCount;
    xr_vector<u32> m_memberGroups;
    xr_vector<GeometryDemandView> m_views;
    xr_vector<GeometryDemandView> m_shadowPending;
    xr_vector<GeometryDemandView> m_shadowActive;
    xr_vector<FrameRecord> m_frames;
    xr_vector<RetiringPage> m_retiring;
    xr_vector<u32> m_uploadQueue;
    xr_vector<u32> m_uploadDeferred;

    u32 m_groupCount = 0;
    u32 m_fineGroupCount = 0;
    u32 m_frameIndex = 0;
    u32 m_mappingGeneration = 0;
    u32 m_cutRevision = 0;
    u64 m_snapshotId = 0;
    u64 m_settledSnapshot = 0;
    u64 m_frameLease = 0;

    GeometryResidencyStats m_stats;
};

} // namespace xray::render::fg
