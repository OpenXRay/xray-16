#include "stdafx.h"
#include "GeometryResidency.h"

#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/AsyncIO.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrEngine/IRenderBackend.h"

namespace xray::render::fg {

namespace {

u32 BlocksFor(u64 bytes)
{
    return u32((bytes + GEOMETRY_PAGE_BLOCK_BYTES - 1ull) / GEOMETRY_PAGE_BLOCK_BYTES);
}

u64 AlignBytes(u64 bytes)
{
    return u64(BlocksFor(bytes)) * GEOMETRY_PAGE_BLOCK_BYTES;
}

constexpr u64 GEOMETRY_ARENA_MAX_BYTES =
    (u64(UINT32_MAX) / GEOMETRY_PAGE_BLOCK_BYTES) * GEOMETRY_PAGE_BLOCK_BYTES;
constexpr u64 GEOMETRY_ARENA_HEADROOM_MAX = 16ull * 1024ull * 1024ull;
constexpr u32 GEOMETRY_ARENA_COPY_GAP_BLOCKS = 16u;
constexpr u32 GEOMETRY_ARENA_COPY_RANGE_LIMIT = 64u;
constexpr u32 GEOMETRY_PAGE_READ_ATTEMPTS = 4u;
constexpr u32 GEOMETRY_PAGE_RETRY_FRAMES = 8u;
constexpr u32 GEOMETRY_EVICTION_LIMIT = 64u;

} // namespace

void GeometryResidencyManager::Arena::Reset(u64 arenaBytes)
{
    bytes = AlignBytes(arenaBytes);
    blockCount = BlocksFor(bytes);
    usedBlocks = 0;
    pinnedBlocks = 0;
    reservedBlocks = 0;
    cursor = 0;
    freeMask.assign((blockCount + 63u) / 64u, 0ull);
    for (u32 b = 0; b < blockCount; ++b)
        freeMask[b >> 6] |= (1ull << (b & 63u));
}

void GeometryResidencyManager::Arena::Grow(u64 arenaBytes)
{
    const u32 previousCount = blockCount;
    bytes = AlignBytes(arenaBytes);
    blockCount = BlocksFor(bytes);
    R_ASSERT(blockCount >= previousCount);
    freeMask.resize((blockCount + 63u) / 64u, 0ull);
    for (u32 block = previousCount; block < blockCount; ++block)
        freeMask[block >> 6] |= 1ull << (block & 63u);
}

bool GeometryResidencyManager::Arena::Allocate(u32 blocks, bool pinnedRequest, u32& outBlock)
{
    if (blocks == 0 || blocks > blockCount)
        return false;

    const u32 ceiling = pinnedRequest ? blockCount : blockCount - reservedBlocks;
    if (usedBlocks + blocks > ceiling)
        return false;

    u32 block = cursor < blockCount ? cursor : 0u;
    u32 run = 0;
    const u32 attempts = blockCount * 2u;
    for (u32 step = 0; step < attempts; ++step) {
        if (block >= blockCount) {
            block = 0;
            run = 0;
        }
        if ((freeMask[block >> 6] & (1ull << (block & 63u))) != 0ull) {
            ++run;
            if (run == blocks) {
                const u32 first = block + 1u - blocks;
                for (u32 i = 0; i < blocks; ++i) {
                    const u32 index = first + i;
                    freeMask[index >> 6] &= ~(1ull << (index & 63u));
                }
                usedBlocks += blocks;
                if (pinnedRequest)
                {
                    pinnedBlocks += blocks;
                    reservedBlocks -= std::min(reservedBlocks, blocks);
                }
                cursor = first + blocks;
                outBlock = first;
                return true;
            }
        } else {
            run = 0;
        }
        ++block;
    }
    return false;
}

void GeometryResidencyManager::Arena::Release(u32 block, u32 blocks, bool pinnedRelease)
{
    if (block == GEOMETRY_PAGE_SLOT_INVALID || blocks == 0)
        return;
    for (u32 i = 0; i < blocks; ++i) {
        const u32 index = block + i;
        if (index >= blockCount)
            break;
        freeMask[index >> 6] |= (1ull << (index & 63u));
    }
    usedBlocks = usedBlocks > blocks ? usedBlocks - blocks : 0u;
    if (!pinnedRelease)
        return;
    const u32 released = std::min(pinnedBlocks, blocks);
    pinnedBlocks -= released;
    reservedBlocks += released;
}

bool GeometryResidencyManager::Arena::CanAllocate(u32 blocks, bool pinnedRequest) const
{
    if (blocks == 0 || blocks > blockCount)
        return false;

    const u32 ceiling = pinnedRequest ? blockCount : blockCount - reservedBlocks;
    if (usedBlocks + blocks > ceiling)
        return false;

    u32 run = 0;
    for (u32 block = 0; block < blockCount; ++block)
    {
        if ((freeMask[block >> 6] & (1ull << (block & 63u))) == 0ull)
        {
            run = 0;
            continue;
        }
        if (++run == blocks)
            return true;
    }
    return false;
}

void GeometryResidencyManager::Arena::CollectLiveRanges(xr_vector<ArenaCopyRange>& out) const
{
    out.clear();
    u32 runStart = GEOMETRY_PAGE_SLOT_INVALID;
    u32 runEnd = 0;
    const u32 words = u32(freeMask.size());
    for (u32 word = 0; word < words; ++word)
    {
        u64 occupied = ~freeMask[word];
        const u32 base = word << 6;
        if (blockCount - base < 64u)
            occupied &= (1ull << (blockCount - base)) - 1ull;
        for (u32 bit = 0; bit < 64u && (occupied >> bit) != 0ull; ++bit)
        {
            if (((occupied >> bit) & 1ull) == 0ull)
                continue;
            const u32 index = base + bit;
            if (runStart != GEOMETRY_PAGE_SLOT_INVALID
                && index <= runEnd + GEOMETRY_ARENA_COPY_GAP_BLOCKS)
            {
                runEnd = index;
                continue;
            }
            if (runStart != GEOMETRY_PAGE_SLOT_INVALID)
            {
                out.push_back(ArenaCopyRange{ u64(runStart) * GEOMETRY_PAGE_BLOCK_BYTES,
                    u64(runEnd + 1u - runStart) * GEOMETRY_PAGE_BLOCK_BYTES });
            }
            runStart = index;
            runEnd = index;
        }
    }
    if (runStart != GEOMETRY_PAGE_SLOT_INVALID)
    {
        out.push_back(ArenaCopyRange{ u64(runStart) * GEOMETRY_PAGE_BLOCK_BYTES,
            u64(runEnd + 1u - runStart) * GEOMETRY_PAGE_BLOCK_BYTES });
    }
    if (out.size() > GEOMETRY_ARENA_COPY_RANGE_LIMIT)
    {
        const u64 first = out.front().offset;
        const u64 last = out.back().offset + out.back().bytes;
        out.clear();
        out.push_back(ArenaCopyRange{ first, last - first });
    }
}

GeometryResidencyManager::GeometryResidencyManager() = default;

GeometryResidencyManager::~GeometryResidencyManager()
{
    Shutdown();
}

void GeometryResidencyManager::Initialize(RenderDevice* device, resources::AsyncIOManager* io)
{
    m_device = device;
    m_io = io;
}

void GeometryResidencyManager::Shutdown()
{
    EndLevel();
    m_device = nullptr;
    m_io = nullptr;
}

void GeometryResidencyManager::ReleaseBuffers()
{
    m_pageTableBuffer = nullptr;
    m_groupBitsBuffer = nullptr;
    m_clusterGroupBuffer = nullptr;
    m_shadowCutBuffers = nullptr;
    m_vertexCopySource = nullptr;
    m_payloadCopySource = nullptr;
    m_vertexCopyRanges.clear();
    m_payloadCopyRanges.clear();
    m_vertexArena.buffer = nullptr;
    m_payloadArena.buffer = nullptr;
    m_vertexArena.Reset(0);
    m_payloadArena.Reset(0);
}

bool GeometryResidencyManager::CreateBuffers(u64 vertexBytes, u64 payloadBytes)
{
    nvrhi::IDevice* nvDevice = m_device ? m_device->GetNVRHIDevice() : nullptr;
    if (!nvDevice)
        return false;

    auto makeRaw = [&](const char* name, u64 size) {
        nvrhi::BufferDesc desc;
        desc.debugName = name;
        desc.byteSize = std::max<u64>(size, 4ull);
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
        desc.keepInitialState = true;
        return nvDevice->createBuffer(desc);
    };

    auto makeStructured = [&](const char* name, u64 elements, u32 stride) {
        nvrhi::BufferDesc desc;
        desc.debugName = name;
        desc.byteSize = std::max<u64>(elements, 1ull) * stride;
        desc.structStride = stride;
        desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
        desc.keepInitialState = true;
        return nvDevice->createBuffer(desc);
    };

    m_vertexArena.buffer = makeRaw("GeoResidency_PageVertices", vertexBytes);
    m_payloadArena.buffer = makeRaw("GeoResidency_PagePayload", payloadBytes);
    m_pageTableBuffer = makeStructured("GeoResidency_PageTable", m_gpuPages.size(), sizeof(GPUClusterPage));
    m_groupBitsBuffer = makeRaw("GeoResidency_GroupState", u64(std::max(m_groupCount, 1u)) * sizeof(u32));
    m_clusterGroupBuffer = makeStructured("GeoResidency_ClusterGroups",
        m_clusterGroupTable.size() / 2u, 8u);
    m_shadowCutBuffers = makeRaw("GeoResidency_ShadowCuts", 16ull + u64(m_groupCount) * sizeof(GeometryCutBounds));

    return m_vertexArena.buffer && m_payloadArena.buffer && m_pageTableBuffer
        && m_groupBitsBuffer && m_clusterGroupBuffer && m_shadowCutBuffers;
}

void GeometryResidencyManager::BuildMemberGroups()
{
    const xr_vector<GPUClusterAssetMember>& members = m_dag->AssetMembers();
    const xr_vector<u32>& owning = m_dag->ClusterOwningGroups();

    m_memberGroupOffset.assign(members.size(), 0u);
    m_memberGroupCount.assign(members.size(), 0u);
    m_memberGroups.clear();

    xr_vector<u32> unique;
    for (u32 m = 0; m < u32(members.size()); ++m) {
        unique.clear();
        const GPUClusterAssetMember& member = members[m];
        for (u32 c = 0; c < member.clusterCount; ++c) {
            const u32 cluster = member.firstCluster + c;
            if (cluster < u32(owning.size()))
                unique.push_back(owning[cluster]);
        }
        std::sort(unique.begin(), unique.end());
        unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
        m_memberGroupOffset[m] = u32(m_memberGroups.size());
        m_memberGroupCount[m] = u32(unique.size());
        m_memberGroups.insert(m_memberGroups.end(), unique.begin(), unique.end());
    }
}

bool GeometryResidencyManager::BeginLevel(ClusterDAG* dag, const char* storePath)
{
    EndLevel();

    if (!dag || !m_device)
        return false;

    m_dag = dag;
    m_storePath = storePath ? storePath : "";
    m_storeIdentity = dag->PageStoreIdentity();
    m_storeVertexOrigin = dag->PageStoreVertexOrigin();
    m_storePayloadOrigin = dag->PageStorePayloadOrigin();
    m_storePageCount = dag->StorePageCount();

    const xr_vector<GPUClusterPage>& pages = dag->Pages();
    const xr_vector<u8>& pageClass = dag->PageClasses();
    const xr_vector<ClusterGroupRecord>& groups = dag->Groups();

    const bool leases = GEnv.Backend && GEnv.Backend->SupportsSubmissionLeases();
    const bool storeUsable = m_io && m_storePageCount != 0 && m_storeIdentity != 0
        && m_storePath.size() != 0;
    const bool needsPaging = ps_r_geo_paging != 0 && dag->ResidencyStats().finePages != 0;
    const bool streaming = needsPaging && leases && storeUsable;

    if (needsPaging && !streaming)
        FATAL_F("[GeoResidency] requested paging requires submission leases (%u), a page store (%u), and shared IO (%u)",
            u32(leases), u32(storeUsable), u32(m_io != nullptr));

    m_groupCount = u32(groups.size());
    m_groupBits.assign(std::max(m_groupCount, 1u), 0u);

    m_groupSlots.clear();
    m_groupSlots.resize(m_groupCount);
    m_cutBounds.resize(m_groupCount);
    m_fineGroupCount = 0;
    for (u32 g = 0; g < m_groupCount; ++g) {
        const ClusterGroupRecord& rec = groups[g];
        GroupSlot& slot = m_groupSlots[g];
        slot.firstMemberPage = rec.firstMemberPage;
        slot.memberPageCount = rec.memberPageCount;
        slot.firstDependency = rec.firstDependency;
        slot.dependencyCount = rec.dependencyCount;
        slot.sphere[0] = rec.sphere[0];
        slot.sphere[1] = rec.sphere[1];
        slot.sphere[2] = rec.sphere[2];
        slot.sphere[3] = rec.sphere[3];
        slot.error = rec.error;
        slot.pinned = !streaming || rec.pinned != 0u;
        if (slot.pinned)
            m_stats.pinnedGroups++;
        else
            m_fineGroupCount++;
    }

    m_pageSlots.clear();
    m_pageSlots.resize(pages.size());
    m_gpuPages.assign(pages.begin(), pages.end());

    u64 pinnedVertexBytes = 0;
    u64 pinnedPayloadBytes = 0;
    u64 totalVertexBytes = 0;
    u64 totalPayloadBytes = 0;
    for (u32 p = 0; p < u32(pages.size()); ++p) {
        const u32 cls = p < u32(pageClass.size()) ? pageClass[p] : CLUSTER_PAGE_CLASS_ROOT;
        PageSlot& slot = m_pageSlots[p];
        slot.runtime = cls == CLUSTER_PAGE_CLASS_RUNTIME;
        slot.pinned = !streaming || cls != CLUSTER_PAGE_CLASS_FINE;
        const u64 vertexBytes = AlignBytes(pages[p].vertexBytes);
        const u64 payloadBytes = AlignBytes(u64(pages[p].payloadBytes) + CLUSTER_PAYLOAD_TAIL_PAD);
        totalVertexBytes += vertexBytes;
        totalPayloadBytes += payloadBytes;
        if (slot.pinned) {
            pinnedVertexBytes += vertexBytes;
            pinnedPayloadBytes += payloadBytes;
        }
        m_gpuPages[p].vertexBase = UINT32_MAX;
        m_gpuPages[p].payloadBase = UINT32_MAX;
    }

    u64 vertexArenaBytes = pinnedVertexBytes;
    u64 payloadArenaBytes = pinnedPayloadBytes;
    if (streaming) {
        const u64 fineBudget = u64(std::max(8, ps_r_geo_page_budget)) * 1024ull * 1024ull;
        const u64 fineVertex = totalVertexBytes - pinnedVertexBytes;
        const u64 finePayload = totalPayloadBytes - pinnedPayloadBytes;
        const u64 fineTotal = std::max<u64>(fineVertex + finePayload, 1ull);
        const u64 grant = std::min(fineBudget, fineTotal);
        vertexArenaBytes += (grant * fineVertex) / fineTotal;
        payloadArenaBytes += (grant * finePayload) / fineTotal;
    }
    vertexArenaBytes = AlignBytes(vertexArenaBytes + m_dag->ResidencyStats().maxPageVertexBytes);
    payloadArenaBytes = AlignBytes(payloadArenaBytes + m_dag->ResidencyStats().maxPagePayloadBytes
        + CLUSTER_PAYLOAD_TAIL_PAD);

    if (vertexArenaBytes > UINT32_MAX || payloadArenaBytes > UINT32_MAX) {
        Msg("! [GeoResidency] arena budget %llu/%llu bytes exceeds the shader addressable range",
            (unsigned long long)vertexArenaBytes, (unsigned long long)payloadArenaBytes);
        m_dag = nullptr;
        return false;
    }

    m_vertexArena.Reset(vertexArenaBytes);
    m_payloadArena.Reset(payloadArenaBytes);

    {
        const xr_vector<u32>& owning = dag->ClusterOwningGroups();
        const xr_vector<u32>& refined = dag->ClusterRefinedGroups();
        m_clusterGroupTable.assign(owning.size() * 2u, UINT32_MAX);
        for (u32 c = 0; c < u32(owning.size()); ++c) {
            m_clusterGroupTable[size_t(c) * 2u + 0u] = owning[c];
            m_clusterGroupTable[size_t(c) * 2u + 1u] = c < u32(refined.size()) ? refined[c] : UINT32_MAX;
        }
    }

    if (!CreateBuffers(vertexArenaBytes, payloadArenaBytes)) {
        Msg("! [GeoResidency] failed to create the residency arenas");
        ReleaseBuffers();
        m_dag = nullptr;
        return false;
    }

    BuildMemberGroups();

    m_frames.clear();
    m_retiring.clear();
    m_uploadQueue.clear();
    m_uploadDeferred.clear();
    m_dirtyPages.clear();
    m_dirtyGroupWords.clear();
    m_pendingActivation.clear();
    m_cutChangedGroups.clear();
    m_views.clear();
    m_groupUnreachable.clear();
    m_unreachableDirty = true;
    m_readFailureLogged = false;
    m_pressureVertexBytes = 0;
    m_pressurePayloadBytes = 0;
    m_frameIndex = 0;
    m_mappingGeneration = 1;
    m_cutRevision = 1;
    m_snapshotId = 0;
    m_settledSnapshot = 0;
    m_frameLease = 0;
    m_tablesInitialized = false;
    m_cookBytesReleased = false;
    m_cookDemandStarted = false;
    m_stats.groups = m_groupCount;
    m_stats.pages = u32(pages.size());
    m_stats.vertexArenaBytes = vertexArenaBytes;
    m_stats.payloadArenaBytes = payloadArenaBytes;
    m_stats.pinnedVertexBytes = pinnedVertexBytes;
    m_stats.pinnedPayloadBytes = pinnedPayloadBytes;
    m_stats.streaming = streaming;
    m_stats.pageBudgetMiB = u32(std::max(8, ps_r_geo_page_budget));
    m_stats.pagingRequested = ps_r_geo_paging != 0;

    if (m_io)
        m_ioEpoch = m_io->AcquireEpoch();

    m_active = true;

    for (u32 p = 0; p < u32(m_pageSlots.size()); ++p) {
        if (!m_pageSlots[p].pinned)
            continue;
        if (!AllocatePage(p))
            FATAL_F("[GeoResidency] pinned page %u does not fit the reserved arena range", p);
        StageFromCook(p);
        m_stats.pinnedPages++;
    }

    for (u32 g = 0; g < m_groupCount; ++g) {
        if (m_groupSlots[g].pinned)
            m_pendingActivation.push_back(g);
    }

    Msg("* [GeoResidency] %s: %u pages (%u pinned), %u groups (%u pinned, %u streamed)",
        streaming ? "demand paging active" : "full residency",
        u32(pages.size()), m_stats.pinnedPages, m_groupCount,
        m_groupCount - m_fineGroupCount, m_fineGroupCount);
    Msg("* [GeoResidency] arenas: %.2f MB vertices + %.2f MB payload (pinned minimum %.2f + %.2f MB)",
        vertexArenaBytes / (1024.0f * 1024.0f), payloadArenaBytes / (1024.0f * 1024.0f),
        pinnedVertexBytes / (1024.0f * 1024.0f), pinnedPayloadBytes / (1024.0f * 1024.0f));
    return true;
}

void GeometryResidencyManager::EndLevel()
{
    if (m_io && m_ioEpoch)
        m_io->DrainEpoch(m_ioEpoch);
    m_ioEpoch = 0;

    if (GEnv.Backend) {
        GEnv.Backend->WaitForIdle();
        for (FrameRecord& frame : m_frames) {
            if (frame.lease)
                GEnv.Backend->ReleaseSubmissionLease(frame.lease);
        }
        if (m_frameLease)
            GEnv.Backend->ReleaseSubmissionLease(m_frameLease);
    }
    m_frames.clear();
    m_frameLease = 0;

    ReleaseBuffers();
    m_gpuPages.clear();
    m_pageSlots.clear();
    m_groupSlots.clear();
    m_groupBits.clear();
    m_clusterGroupTable.clear();
    m_dirtyPages.clear();
    m_dirtyGroupWords.clear();
    m_pendingActivation.clear();
    m_cutChangedGroups.clear();
    m_cutBounds.clear();
    m_cutStaging.clear();
    m_cutStagingRevision = 0;
    m_memberGroupOffset.clear();
    m_memberGroupCount.clear();
    m_memberGroups.clear();
    m_retiring.clear();
    m_uploadQueue.clear();
    m_uploadDeferred.clear();
    m_views.clear();
    m_shadowPending.clear();
    m_shadowActive.clear();
    m_groupUnreachable.clear();
    m_unreachableDirty = false;
    m_readFailureLogged = false;
    m_pressureVertexBytes = 0;
    m_pressurePayloadBytes = 0;
    m_dag = nullptr;
    m_active = false;
    m_tablesInitialized = false;
    m_cookBytesReleased = false;
    m_cookDemandStarted = false;
    m_groupCount = 0;
    m_fineGroupCount = 0;
    m_storePageCount = 0;
    m_storeIdentity = 0;
    m_stats = GeometryResidencyStats();
}

void GeometryResidencyManager::RegisterRuntimePages()
{
    if (!m_active || !m_dag)
        return;

    const xr_vector<GPUClusterPage>& pages = m_dag->Pages();
    const xr_vector<u8>& pageClass = m_dag->PageClasses();
    const xr_vector<ClusterGroupRecord>& groups = m_dag->Groups();

    const u32 groupCount = u32(groups.size());
    if (groupCount > m_groupCount) {
        m_groupBits.resize(std::max(groupCount, 1u), 0u);
        m_groupSlots.resize(groupCount);
        m_cutBounds.resize(groupCount);
        for (u32 g = m_groupCount; g < groupCount; ++g) {
            const ClusterGroupRecord& rec = groups[g];
            GroupSlot& slot = m_groupSlots[g];
            slot.firstMemberPage = rec.firstMemberPage;
            slot.memberPageCount = rec.memberPageCount;
            slot.firstDependency = rec.firstDependency;
            slot.dependencyCount = rec.dependencyCount;
            slot.sphere[0] = rec.sphere[0];
            slot.sphere[1] = rec.sphere[1];
            slot.sphere[2] = rec.sphere[2];
            slot.sphere[3] = rec.sphere[3];
            slot.error = rec.error;
            slot.pinned = true;
            m_pendingActivation.push_back(g);
        }
        m_groupCount = groupCount;
        auto desc = m_groupBitsBuffer->getDesc();
        desc.byteSize = u64(groupCount) * sizeof(u32);
        m_groupBitsBuffer = m_device->GetNVRHIDevice()->createBuffer(desc);
        R_ASSERT(m_groupBitsBuffer);
        desc = m_shadowCutBuffers->getDesc();
        desc.byteSize = 16ull + u64(groupCount) * sizeof(GeometryCutBounds);
        m_shadowCutBuffers = m_device->GetNVRHIDevice()->createBuffer(desc);
        R_ASSERT(m_shadowCutBuffers);
        MarkAllTablesDirty();
    }

    for (u32 g = 0; g < m_groupCount; ++g) {
        const ClusterGroupRecord& rec = groups[g];
        GroupSlot& slot = m_groupSlots[g];
        slot.firstMemberPage = rec.firstMemberPage;
        slot.memberPageCount = rec.memberPageCount;
        slot.firstDependency = rec.firstDependency;
        slot.dependencyCount = rec.dependencyCount;
    }

    const u32 oldCount = u32(m_pageSlots.size());
    if (u32(pages.size()) > oldCount) {
        u64 vertexBytes = 0;
        u64 payloadBytes = 0;
        for (u32 page = oldCount; page < u32(pages.size()); ++page)
        {
            vertexBytes += AlignBytes(pages[page].vertexBytes);
            payloadBytes += AlignBytes(u64(pages[page].payloadBytes) + CLUSTER_PAYLOAD_TAIL_PAD);
        }
        ReserveRuntimeArena(m_vertexArena, vertexBytes, false, m_vertexCopySource, m_vertexCopyRanges);
        ReserveRuntimeArena(m_payloadArena, payloadBytes, false, m_payloadCopySource, m_payloadCopyRanges);
        m_stats.pinnedVertexBytes += vertexBytes;
        m_stats.pinnedPayloadBytes += payloadBytes;
        m_stats.vertexArenaBytes = m_vertexArena.bytes;
        m_stats.payloadArenaBytes = m_payloadArena.bytes;
        auto desc = m_pageTableBuffer->getDesc();
        desc.byteSize = u64(pages.size()) * sizeof(GPUClusterPage);
        m_pageTableBuffer = m_device->GetNVRHIDevice()->createBuffer(desc);
        R_ASSERT(m_pageTableBuffer);
        m_tablesInitialized = false;
        m_pageSlots.resize(pages.size());
        m_gpuPages.resize(pages.size());
        for (u32 p = oldCount; p < u32(pages.size()); ++p) {
            const u32 cls = p < u32(pageClass.size()) ? pageClass[p] : CLUSTER_PAGE_CLASS_RUNTIME;
            PageSlot& slot = m_pageSlots[p];
            slot = PageSlot();
            slot.runtime = cls == CLUSTER_PAGE_CLASS_RUNTIME;
            slot.pinned = true;
            m_gpuPages[p] = pages[p];
            m_gpuPages[p].vertexBase = UINT32_MAX;
            m_gpuPages[p].payloadBase = UINT32_MAX;
            if (!AllocatePage(p))
            {
                const u32 vertexRun = BlocksFor(pages[p].vertexBytes);
                const u32 payloadRun = BlocksFor(u64(pages[p].payloadBytes) + CLUSTER_PAYLOAD_TAIL_PAD);
                if (!m_vertexArena.CanAllocate(vertexRun, true))
                {
                    ReserveRuntimeArena(m_vertexArena, u64(vertexRun) * GEOMETRY_PAGE_BLOCK_BYTES, true,
                        m_vertexCopySource, m_vertexCopyRanges);
                }
                if (!m_payloadArena.CanAllocate(payloadRun, true))
                {
                    ReserveRuntimeArena(m_payloadArena, u64(payloadRun) * GEOMETRY_PAGE_BLOCK_BYTES, true,
                        m_payloadCopySource, m_payloadCopyRanges);
                }
                m_stats.vertexArenaBytes = m_vertexArena.bytes;
                m_stats.payloadArenaBytes = m_payloadArena.bytes;
                if (!AllocatePage(p))
                    FATAL_F("[GeoResidency] pinned runtime page %u does not fit the grown arena", p);
            }
            u32 vertexSize = 0;
            u32 payloadSize = 0;
            const u8* vertices = CookVertexBytes(p, vertexSize);
            const u8* payload = CookPayloadBytes(p, payloadSize);
            R_ASSERT(vertices && payload);
            slot.vertexStaging.assign(vertices, vertices + vertexSize);
            slot.payloadStaging.assign(payload, payload + payloadSize);
            slot.state = PageState::Staged;
            m_uploadQueue.push_back(p);
            m_stats.pinnedPages++;
        }
        m_dag->ReleaseRuntimePageBytes();
    }

    const xr_vector<u32>& owning = m_dag->ClusterOwningGroups();
    if (owning.size() * 2u > m_clusterGroupTable.size()) {
        const xr_vector<u32>& refined = m_dag->ClusterRefinedGroups();
        m_clusterGroupTable.assign(owning.size() * 2u, UINT32_MAX);
        for (u32 c = 0; c < u32(owning.size()); ++c) {
            m_clusterGroupTable[size_t(c) * 2u + 0u] = owning[c];
            m_clusterGroupTable[size_t(c) * 2u + 1u] = c < u32(refined.size()) ? refined[c] : UINT32_MAX;
        }
        nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
        nvrhi::BufferDesc desc;
        desc.debugName = "GeoResidency_ClusterGroups";
        desc.byteSize = std::max<u64>(m_clusterGroupTable.size() / 2u, 1ull) * 8ull;
        desc.structStride = 8u;
        desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
        desc.keepInitialState = true;
        m_clusterGroupBuffer = nvDevice->createBuffer(desc);
        R_ASSERT(m_clusterGroupBuffer);
        m_tablesInitialized = false;
    }

    if (u32(m_dag->AssetMembers().size()) > u32(m_memberGroupCount.size()))
        BuildMemberGroups();
}

const u8* GeometryResidencyManager::CookVertexBytes(u32 page, u32& outSize) const
{
    const GPUClusterPage& source = m_dag->Pages()[page];
    const xr_vector<u8>& data = m_pageSlots[page].runtime
        ? m_dag->RuntimePageVertexData() : m_dag->PageVertexData();
    const u64 end = u64(source.vertexBase) + source.vertexBytes;
    if (end > data.size())
        return nullptr;
    outSize = source.vertexBytes;
    return data.data() + source.vertexBase;
}

const u8* GeometryResidencyManager::CookPayloadBytes(u32 page, u32& outSize) const
{
    const GPUClusterPage& source = m_dag->Pages()[page];
    const xr_vector<u8>& data = m_pageSlots[page].runtime
        ? m_dag->RuntimePagePayloadData() : m_dag->PagePayloadData();
    const u64 end = u64(source.payloadBase) + source.payloadBytes + CLUSTER_PAYLOAD_TAIL_PAD;
    if (end > data.size())
        return nullptr;
    outSize = source.payloadBytes + CLUSTER_PAYLOAD_TAIL_PAD;
    return data.data() + source.payloadBase;
}

void GeometryResidencyManager::StageFromCook(u32 page)
{
    PageSlot& slot = m_pageSlots[page];
    u32 vertexSize = 0;
    u32 payloadSize = 0;
    if (!CookVertexBytes(page, vertexSize) || !CookPayloadBytes(page, payloadSize))
        FATAL_F("[GeoResidency] page %u is outside the retained cook arena", page);

    slot.cpuSourced = true;
    slot.state = PageState::Staged;
    m_uploadQueue.push_back(page);
}

bool GeometryResidencyManager::AllocatePage(u32 page)
{
    PageSlot& slot = m_pageSlots[page];
    if (slot.vertexBlock != GEOMETRY_PAGE_SLOT_INVALID)
        return true;

    const GPUClusterPage& source = m_dag->Pages()[page];
    const u32 vertexBlocks = BlocksFor(source.vertexBytes);
    const u32 payloadBlocks = BlocksFor(u64(source.payloadBytes) + CLUSTER_PAYLOAD_TAIL_PAD);

    u32 vertexBlock = GEOMETRY_PAGE_SLOT_INVALID;
    u32 payloadBlock = GEOMETRY_PAGE_SLOT_INVALID;
    if (!m_vertexArena.Allocate(vertexBlocks, slot.pinned, vertexBlock))
        return false;
    if (!m_payloadArena.Allocate(payloadBlocks, slot.pinned, payloadBlock))
    {
        m_vertexArena.Release(vertexBlock, vertexBlocks, slot.pinned);
        return false;
    }

    slot.vertexBlock = vertexBlock;
    slot.vertexBlocks = vertexBlocks;
    slot.payloadBlock = payloadBlock;
    slot.payloadBlocks = payloadBlocks;
    return true;
}

void GeometryResidencyManager::RecordAllocationPressure(u32 page)
{
    const GPUClusterPage& source = m_dag->Pages()[page];
    const bool pinned = m_pageSlots[page].pinned;
    const auto record = [pinned](const Arena& arena, u64 bytes, u64& pressure)
    {
        const u64 required = AlignBytes(bytes);
        if (arena.CanAllocate(BlocksFor(required), pinned))
            return;
        const u64 capacity = pinned ? arena.bytes : arena.FineBudgetBytes();
        const u64 used = pinned ? arena.UsedBytes() : arena.FineUsedBytes();
        const u64 available = capacity - std::min(capacity, used);
        const u64 missing = required > available ? required - available : required;
        pressure = std::max(pressure, missing);
    };
    record(m_vertexArena, source.vertexBytes, m_pressureVertexBytes);
    record(m_payloadArena, u64(source.payloadBytes) + CLUSTER_PAYLOAD_TAIL_PAD, m_pressurePayloadBytes);
}

void GeometryResidencyManager::IssuePageRead(u32 page)
{
    PageSlot& slot = m_pageSlots[page];
    if (slot.state != PageState::Absent || !m_io || page >= m_storePageCount)
        return;
    if (slot.readAttempts != 0 && m_frameIndex < slot.retryFrame)
        return;

    const GPUClusterPage& source = m_dag->Pages()[page];
    if (source.vertexBytes == 0 || source.payloadBytes == 0)
        return;

    slot.vertexRead = m_io->ReadAsync(m_storePath.c_str(), m_storeVertexOrigin + source.vertexBase,
        source.vertexBytes, m_ioEpoch, nullptr, nullptr, m_storeIdentity);
    slot.payloadRead = m_io->ReadAsync(m_storePath.c_str(), m_storePayloadOrigin + source.payloadBase,
        source.payloadBytes, m_ioEpoch, nullptr, nullptr, m_storeIdentity);

    if (!slot.vertexRead || !slot.payloadRead)
    {
        if (slot.vertexRead)
            m_io->CancelRequest(slot.vertexRead);
        if (slot.payloadRead)
            m_io->CancelRequest(slot.payloadRead);
        slot.vertexRead = 0;
        slot.payloadRead = 0;
        RecordPageReadFailure(page, "the request could not be queued");
        return;
    }

    slot.state = PageState::Reading;
    m_stats.readsIssued++;
}

void GeometryResidencyManager::RecordPageReadFailure(u32 page, const char* reason)
{
    PageSlot& slot = m_pageSlots[page];
    slot.vertexRead = 0;
    slot.payloadRead = 0;
    slot.vertexStaging.clear();
    slot.vertexStaging.shrink_to_fit();
    slot.payloadStaging.clear();
    slot.payloadStaging.shrink_to_fit();
    slot.state = PageState::Absent;
    m_stats.readsFailed++;

    if (slot.readAttempts < GEOMETRY_PAGE_READ_ATTEMPTS)
        ++slot.readAttempts;
    if (slot.readAttempts < GEOMETRY_PAGE_READ_ATTEMPTS)
    {
        const u32 backoff = GEOMETRY_PAGE_RETRY_FRAMES << (slot.readAttempts - 1u);
        slot.retryFrame = m_frameIndex + backoff;
        return;
    }

    slot.state = PageState::Failed;
    slot.retryFrame = 0;
    m_stats.failedPages++;
    m_unreachableDirty = true;
    if (!m_readFailureLogged)
    {
        m_readFailureLogged = true;
        Msg("! [GeoResidency] page %u abandoned after %u failed reads from '%s' (%s); the coarse residency stays in place",
            page, GEOMETRY_PAGE_READ_ATTEMPTS, m_storePath.c_str(), reason);
    }
}

void GeometryResidencyManager::MarkGroupWordDirty(u32 word)
{
    if (word >= u32(m_groupBits.size()))
        return;
    if (std::find(m_dirtyGroupWords.begin(), m_dirtyGroupWords.end(), word) == m_dirtyGroupWords.end())
        m_dirtyGroupWords.push_back(word);
}

void GeometryResidencyManager::MarkPageDirty(u32 page)
{
    if (std::find(m_dirtyPages.begin(), m_dirtyPages.end(), page) == m_dirtyPages.end())
        m_dirtyPages.push_back(page);
}

void GeometryResidencyManager::MarkAllTablesDirty()
{
    m_dirtyPages.clear();
    m_dirtyPages.reserve(m_gpuPages.size());
    for (u32 p = 0; p < u32(m_gpuPages.size()); ++p)
        m_dirtyPages.push_back(p);

    m_dirtyGroupWords.clear();
    m_dirtyGroupWords.reserve(m_groupBits.size());
    for (u32 w = 0; w < u32(m_groupBits.size()); ++w)
        m_dirtyGroupWords.push_back(w);
}

void GeometryResidencyManager::RefreshCoarseReady(u32 group)
{
    if (group >= m_groupCount)
        return;
    const GroupSlot& slot = m_groupSlots[group];
    bool ready = slot.dependencyCount != 0;
    const xr_vector<u32>& dependencies = m_dag->GroupDependencies();
    for (u32 i = 0; i < slot.dependencyCount && ready; ++i) {
        const u32 dependency = dependencies[slot.firstDependency + i];
        ready = dependency < m_groupCount && m_groupSlots[dependency].resident;
    }

    const u32 before = m_groupBits[group];
    if (ready)
        m_groupBits[group] |= 2u;
    else
        m_groupBits[group] &= ~2u;
    if (m_groupBits[group] != before)
        MarkGroupWordDirty(group);
}

void GeometryResidencyManager::SetGroupResident(u32 group, bool resident)
{
    GroupSlot& slot = m_groupSlots[group];
    if (slot.resident == resident)
        return;

    slot.resident = resident;
    if (resident)
        m_groupBits[group] |= 1u;
    else
        m_groupBits[group] &= ~1u;
    MarkGroupWordDirty(group);

    const xr_vector<u32>& dependencies = m_dag->GroupDependencies();
    for (u32 i = 0; i < slot.dependencyCount; ++i) {
        const u32 dependency = dependencies[slot.firstDependency + i];
        if (dependency >= m_groupCount)
            continue;
        GroupSlot& owner = m_groupSlots[dependency];
        if (resident)
            owner.dependentResidents++;
        else if (owner.dependentResidents)
            owner.dependentResidents--;
    }

    const xr_vector<u32>& replacements = m_dag->GroupReplacementClusters();
    const xr_vector<u32>& owningOfCluster = m_dag->ClusterOwningGroups();
    for (u32 i = 0; i < slot.memberPageCount; ++i)
        MarkPageDirty(m_dag->GroupMemberPageList()[slot.firstMemberPage + i]);

    const ClusterGroupRecord& record = m_dag->Groups()[group];
    for (u32 i = 0; i < record.replacementCount; ++i) {
        const u32 cluster = replacements[record.firstReplacement + i];
        if (cluster < u32(owningOfCluster.size()))
            RefreshCoarseReady(owningOfCluster[cluster]);
    }
    RefreshCoarseReady(group);

    m_cutChangedGroups.push_back(group);
    R_ASSERT2(m_cutRevision != UINT32_MAX, "Geometry cut revision exhausted");
    ++m_cutRevision;
    if (slot.worldBoundsValid || slot.previousWorldBoundsValid)
    {
        Fbox bounds = slot.worldBoundsValid ? slot.worldBounds : slot.previousWorldBounds;
        if (slot.previousWorldBoundsValid)
            bounds.merge(slot.previousWorldBounds);
        auto& cut = m_cutBounds[group];
        if (cut.revision != 0)
        {
            bounds.modify(Fvector().set(cut.minimum.x, cut.minimum.y, cut.minimum.z));
            bounds.modify(Fvector().set(cut.maximum.x, cut.maximum.y, cut.maximum.z));
        }
        cut.minimum.set(bounds.vMin.x, bounds.vMin.y, bounds.vMin.z, 0.0f);
        cut.maximum.set(bounds.vMax.x, bounds.vMax.y, bounds.vMax.z, 0.0f);
        cut.revision = m_cutRevision;
    }
}

void GeometryResidencyManager::PublishPage(u32 page)
{
    PageSlot& slot = m_pageSlots[page];
    const GPUClusterPage& source = m_dag->Pages()[page];

    GPUClusterPage& entry = m_gpuPages[page];
    entry = source;
    entry.vertexBase = slot.vertexBlock * GEOMETRY_PAGE_BLOCK_BYTES;
    entry.payloadBase = slot.payloadBlock * GEOMETRY_PAGE_BLOCK_BYTES;
    slot.published = true;
    slot.publishedSnapshot = m_snapshotId;
    MarkPageDirty(page);
}

void GeometryResidencyManager::UnpublishPage(u32 page)
{
    PageSlot& slot = m_pageSlots[page];
    GPUClusterPage& entry = m_gpuPages[page];
    entry.vertexBase = UINT32_MAX;
    entry.payloadBase = UINT32_MAX;
    slot.published = false;
    MarkPageDirty(page);
}

void GeometryResidencyManager::RetirePage(u32 page)
{
    PageSlot& slot = m_pageSlots[page];
    const bool abandoned = slot.state == PageState::Failed;
    UnpublishPage(page);

    if (slot.vertexBlock != GEOMETRY_PAGE_SLOT_INVALID) {
        RetiringPage retiring;
        retiring.retireAfter = m_snapshotId;
        retiring.vertexBlock = slot.vertexBlock;
        retiring.vertexBlocks = slot.vertexBlocks;
        retiring.payloadBlock = slot.payloadBlock;
        retiring.payloadBlocks = slot.payloadBlocks;
        retiring.pinned = slot.pinned;
        m_retiring.push_back(retiring);
    }

    slot.vertexBlock = GEOMETRY_PAGE_SLOT_INVALID;
    slot.vertexBlocks = 0;
    slot.payloadBlock = GEOMETRY_PAGE_SLOT_INVALID;
    slot.payloadBlocks = 0;
    slot.state = abandoned ? PageState::Failed : PageState::Absent;
    slot.cpuSourced = false;
    slot.vertexStaging.clear();
    slot.vertexStaging.shrink_to_fit();
    slot.payloadStaging.clear();
    slot.payloadStaging.shrink_to_fit();
    if (slot.vertexRead && m_io)
        m_io->CancelRequest(slot.vertexRead);
    if (slot.payloadRead && m_io)
        m_io->CancelRequest(slot.payloadRead);
    slot.vertexRead = 0;
    slot.payloadRead = 0;
}

bool GeometryResidencyManager::TryActivateGroup(u32 group)
{
    GroupSlot& slot = m_groupSlots[group];
    if (slot.resident)
        return true;

    const xr_vector<u32>& memberPages = m_dag->GroupMemberPageList();
    for (u32 i = 0; i < slot.memberPageCount; ++i) {
        const u32 page = memberPages[slot.firstMemberPage + i];
        if (page >= u32(m_pageSlots.size()) || !m_pageSlots[page].published)
            return false;
    }

    const xr_vector<u32>& dependencies = m_dag->GroupDependencies();
    for (u32 i = 0; i < slot.dependencyCount; ++i) {
        const u32 dependency = dependencies[slot.firstDependency + i];
        if (dependency >= m_groupCount || !m_groupSlots[dependency].resident)
            return false;
    }

    if (!slot.requested)
    {
        for (u32 i = 0; i < slot.memberPageCount; ++i)
            m_pageSlots[memberPages[slot.firstMemberPage + i]].refs++;
    }

    SetGroupResident(group, true);
    slot.requested = false;
    return true;
}

void GeometryResidencyManager::EvictGroup(u32 group)
{
    GroupSlot& slot = m_groupSlots[group];
    if (!slot.resident || slot.pinned || slot.dependentResidents != 0)
        return;

    SetGroupResident(group, false);
    slot.requested = false;

    const xr_vector<u32>& memberPages = m_dag->GroupMemberPageList();
    for (u32 i = 0; i < slot.memberPageCount; ++i) {
        const u32 page = memberPages[slot.firstMemberPage + i];
        PageSlot& pageSlot = m_pageSlots[page];
        if (pageSlot.refs)
            pageSlot.refs--;
        if (pageSlot.refs == 0 && !pageSlot.pinned)
            RetirePage(page);
    }
    m_stats.evictions++;
}

void GeometryResidencyManager::AbandonGroup(u32 group)
{
    GroupSlot& slot = m_groupSlots[group];
    if (!slot.requested || slot.resident)
        return;

    slot.requested = false;
    const xr_vector<u32>& memberPages = m_dag->GroupMemberPageList();
    for (u32 i = 0; i < slot.memberPageCount; ++i)
    {
        const u32 page = memberPages[slot.firstMemberPage + i];
        PageSlot& pageSlot = m_pageSlots[page];
        R_ASSERT(pageSlot.refs != 0);
        if (--pageSlot.refs == 0 && !pageSlot.pinned)
            RetirePage(page);
    }
}

void GeometryResidencyManager::RefreshUnreachableGroups()
{
    if (m_groupUnreachable.size() != size_t(m_groupCount))
        m_unreachableDirty = true;
    if (!m_unreachableDirty)
        return;

    m_unreachableDirty = false;
    m_groupUnreachable.assign(m_groupCount, u8(0));

    const xr_vector<u32>& memberPages = m_dag->GroupMemberPageList();
    const xr_vector<u32>& dependencies = m_dag->GroupDependencies();
    for (u32 group = 0; group < m_groupCount; ++group)
    {
        const GroupSlot& slot = m_groupSlots[group];
        for (u32 i = 0; i < slot.memberPageCount; ++i)
        {
            const u32 page = memberPages[slot.firstMemberPage + i];
            if (page < u32(m_pageSlots.size()) && m_pageSlots[page].state == PageState::Failed)
            {
                m_groupUnreachable[group] = 1;
                break;
            }
        }
    }

    bool changed = true;
    while (changed)
    {
        changed = false;
        for (u32 group = 0; group < m_groupCount; ++group)
        {
            if (m_groupUnreachable[group])
                continue;
            const GroupSlot& slot = m_groupSlots[group];
            for (u32 i = 0; i < slot.dependencyCount; ++i)
            {
                const u32 dependency = dependencies[slot.firstDependency + i];
                if (dependency >= m_groupCount || !m_groupUnreachable[dependency])
                    continue;
                m_groupUnreachable[group] = 1;
                changed = true;
                break;
            }
        }
    }

    m_stats.unreachableGroups = 0;
    for (u32 group = 0; group < m_groupCount; ++group)
    {
        if (!m_groupUnreachable[group])
            continue;
        m_stats.unreachableGroups++;
        AbandonGroup(group);
    }
}

void GeometryResidencyManager::DeactivateIncompleteGroups()
{
    const auto& pages = m_dag->GroupMemberPageList();
    const auto& dependencies = m_dag->GroupDependencies();
    xr_vector<u8> incomplete(m_groupCount, 0);
    for (u32 group = 0; group < m_groupCount; ++group)
    {
        const auto& slot = m_groupSlots[group];
        for (u32 i = 0; i < slot.memberPageCount; ++i)
            incomplete[group] |= !m_pageSlots[pages[slot.firstMemberPage + i]].published;
    }
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (u32 group = 0; group < m_groupCount; ++group)
        {
            const auto& slot = m_groupSlots[group];
            if (incomplete[group])
                continue;
            for (u32 i = 0; i < slot.dependencyCount; ++i)
            {
                if (incomplete[dependencies[slot.firstDependency + i]])
                {
                    incomplete[group] = 1;
                    changed = true;
                    break;
                }
            }
        }
    }
    changed = true;
    while (changed)
    {
        changed = false;
        for (u32 group = 0; group < m_groupCount; ++group)
        {
            auto& slot = m_groupSlots[group];
            if (!incomplete[group] || !slot.resident || slot.dependentResidents != 0)
                continue;
            SetGroupResident(group, false);
            slot.requested = false;
            for (u32 i = 0; i < slot.memberPageCount; ++i)
            {
                auto& page = m_pageSlots[pages[slot.firstMemberPage + i]];
                R_ASSERT(page.refs != 0);
                --page.refs;
            }
            if (slot.pinned)
                m_pendingActivation.push_back(group);
            changed = true;
        }
    }
}

void GeometryResidencyManager::ReleaseCookBytesIfConfirmed()
{
    if (m_cookBytesReleased || !m_dag || !m_dag->LevelPageBytesResident())
        return;
    if (IsStreaming() && !m_cookDemandStarted)
        return;

    for (const FrameRecord& frame : m_frames)
    {
        if (frame.usesCookBytes)
            return;
    }

    for (u32 p = 0; p < u32(m_pageSlots.size()); ++p)
    {
        const PageSlot& slot = m_pageSlots[p];
        if (slot.runtime || (!slot.pinned && !slot.cpuSourced))
            continue;
        if (slot.state != PageState::Resident)
            return;
    }

    for (PageSlot& slot : m_pageSlots)
    {
        if (!slot.runtime && slot.cpuSourced)
            slot.cpuSourced = false;
    }

    m_dag->ReleaseLevelPageBytes();
    m_cookBytesReleased = true;
    Msg("* [GeoResidency] released the level page cook arena after the initial geometry uploads completed");
}

void GeometryResidencyManager::CompleteFrames(IRenderBackend* backend)
{
    while (!m_frames.empty()) {
        FrameRecord& frame = m_frames.front();
        const IRenderBackend::SubmissionLeaseState state = backend && frame.lease
            ? backend->PollSubmissionLease(frame.lease)
            : IRenderBackend::SubmissionLeaseState::Unknown;

        R_ASSERT2(state != IRenderBackend::SubmissionLeaseState::Unknown,
            "Geometry snapshot lost its submission lease");
        if (state == IRenderBackend::SubmissionLeaseState::Open
            || state == IRenderBackend::SubmissionLeaseState::Pending)
            break;

        if (state == IRenderBackend::SubmissionLeaseState::Complete) {
            for (u32 page : frame.uploadedPages) {
                if (page >= u32(m_pageSlots.size()))
                    continue;
                PageSlot& slot = m_pageSlots[page];
                if (slot.state != PageState::Uploading || slot.publishedSnapshot != frame.id)
                    continue;
                slot.state = PageState::Resident;
                slot.vertexStaging.clear();
                slot.vertexStaging.shrink_to_fit();
                slot.payloadStaging.clear();
                slot.payloadStaging.shrink_to_fit();
            }
            m_settledSnapshot = frame.id;
        } else {
            R_ASSERT2(!frame.vertexCopySource && !frame.payloadCopySource,
                "A failed geometry arena migration cannot be published");
            for (u32 page : frame.uploadedPages) {
                if (page >= u32(m_pageSlots.size()))
                    continue;
                PageSlot& slot = m_pageSlots[page];
                if (slot.state != PageState::Uploading || slot.publishedSnapshot != frame.id)
                    continue;
                auto vertices = std::move(slot.vertexStaging);
                auto payload = std::move(slot.payloadStaging);
                const bool cpuSourced = slot.cpuSourced;
                RetirePage(page);
                slot.vertexStaging = std::move(vertices);
                slot.payloadStaging = std::move(payload);
                slot.cpuSourced = cpuSourced;
                slot.state = PageState::Staged;
                m_uploadQueue.push_back(page);
                m_stats.uploadsDiscarded++;
            }
            DeactivateIncompleteGroups();
            MarkAllTablesDirty();
            m_tablesInitialized = false;
            m_settledSnapshot = frame.id;
            m_stats.failedSnapshots++;
        }

        if (backend && frame.lease)
            backend->ReleaseSubmissionLease(frame.lease);
        m_frames.erase(m_frames.begin());
    }

    for (auto it = m_retiring.begin(); it != m_retiring.end();) {
        if (it->retireAfter > m_settledSnapshot) {
            ++it;
            continue;
        }
        m_vertexArena.Release(it->vertexBlock, it->vertexBlocks, it->pinned);
        m_payloadArena.Release(it->payloadBlock, it->payloadBlocks, it->pinned);
        it = m_retiring.erase(it);
    }
}

void GeometryResidencyManager::BeginFrame(IRenderBackend* backend)
{
    if (!m_active)
        return;

    ++m_frameIndex;
    m_cutChangedGroups.clear();
    CompleteFrames(backend);

    m_mappingGeneration++;
    m_snapshotId++;
    m_frameLease = backend && backend->SupportsSubmissionLeases() ? backend->OpenSubmissionLease() : 0;

    m_stats.uploadsRecorded = 0;
    m_stats.uploadKiB = 0;
    m_stats.allocationDeferrals = 0;
    m_stats.budgetDeferrals = 0;
    m_stats.dependencyStalls = 0;
    m_stats.readThrottles = 0;
    m_stats.uploadThrottles = 0;
    m_stats.reservationFailures = 0;
    m_pressureVertexBytes = 0;
    m_pressurePayloadBytes = 0;
}

void GeometryResidencyManager::ClearDemand()
{
    m_views.clear();
    m_shadowPending.clear();
    for (GroupSlot& slot : m_groupSlots)
    {
        slot.desired = false;
        slot.previousWorldBounds = slot.worldBounds;
        slot.previousWorldBoundsValid = slot.worldBoundsValid;
        slot.worldBoundsValid = false;
    }
    m_stats.desiredGroups = 0;
}

void GeometryResidencyManager::AddDemandView(const GeometryDemandView& view)
{
    if (view.valid && (view.errorScale > 0.0f || view.errorBound > 0.0f))
        m_views.push_back(view);
}

void GeometryResidencyManager::AddShadowDemand(const GeometryDemandView& view)
{
    if (view.valid && view.errorBound > 0.0f)
        m_shadowPending.push_back(view);
}

void GeometryResidencyManager::PromoteShadowDemand()
{
    m_shadowActive.swap(m_shadowPending);
    m_shadowPending.clear();
}

bool GeometryResidencyManager::GroupDesiredForInstance(const GroupSlot& group,
    const Fvector& center, float radius, float scaleBound) const
{
    const float error = group.error * scaleBound;
    if (!(error > 0.0f))
        return false;
    for (u32 pass = 0; pass < 2u; ++pass)
    {
        for (const auto& view : (pass == 0u ? m_views : m_shadowActive))
        {
            bool outside = false;
            for (u32 p = 0; p < view.planeCount; ++p)
            {
                const auto& plane = view.planes[p];
                outside |= plane.x * center.x + plane.y * center.y + plane.z * center.z + plane.w > radius;
            }
            if (outside)
                continue;
            if (view.orthographicBounds)
            {
                Fvector local;
                view.worldToBounds.transform_tiny(local, center);
                if (local.x + radius < view.boundsMinimum.x || local.x - radius > view.boundsMaximum.x
                    || local.y + radius < view.boundsMinimum.y || local.y - radius > view.boundsMaximum.y
                    || local.z + radius < view.boundsMinimum.z || local.z - radius > view.boundsMaximum.z)
                    continue;
            }
            if (view.boundsRadius > 0.0f)
            {
                const float reach = view.boundsRadius + radius;
                if (center.distance_to_sqr(view.boundsCenter) > reach * reach)
                    continue;
            }
            if (view.errorBound > 0.0f)
            {
                if (error > view.errorBound)
                    return true;
                continue;
            }
            Fvector delta;
            delta.sub(center, view.origin);
            const float distance = std::max(view.minDistance, view.direction.dotproduct(delta) - radius);
            if (error * view.errorScale / distance > 1.0f)
                return true;
        }
    }
    return false;
}

void GeometryResidencyManager::AddInstanceDemand(u32 assetMember, const Fmatrix& world,
    float scaleBound, bool plain)
{
    if (!m_active || m_fineGroupCount == 0)
        return;
    if (assetMember >= u32(m_memberGroupCount.size()))
        return;

    m_cookDemandStarted = true;

    const u32 first = m_memberGroupOffset[assetMember];
    const u32 count = m_memberGroupCount[assetMember];
    for (u32 i = 0; i < count; ++i) {
        const u32 group = m_memberGroups[first + i];
        if (group >= m_groupCount)
            continue;
        GroupSlot& slot = m_groupSlots[group];
        Fvector center;
        world.transform_tiny(center, Fvector().set(slot.sphere[0], slot.sphere[1], slot.sphere[2]));
        const float radius = slot.sphere[3] * scaleBound;
        Fbox bounds;
        bounds.vMin.set(center.x - radius, center.y - radius, center.z - radius);
        bounds.vMax.set(center.x + radius, center.y + radius, center.z + radius);
        if (slot.worldBoundsValid)
            slot.worldBounds.merge(bounds);
        else
            slot.worldBounds = bounds;
        slot.worldBoundsValid = true;
        if (slot.desired || slot.pinned)
            continue;
        if (!plain && !GroupDesiredForInstance(slot, center, radius, scaleBound))
            continue;
        slot.desired = true;
        slot.lastDesiredFrame = m_frameIndex;
        m_stats.desiredGroups++;
    }
}

void GeometryResidencyManager::ResolveDemand()
{
    if (!m_active)
        return;

    if (m_io) {
        for (u32 p = 0; p < u32(m_pageSlots.size()); ++p) {
            PageSlot& slot = m_pageSlots[p];
            if (slot.state != PageState::Reading)
                continue;
            if (!m_io->IsRequestSettled(slot.vertexRead) || !m_io->IsRequestSettled(slot.payloadRead))
                continue;

            resources::AsyncIORequest vertex;
            resources::AsyncIORequest payload;
            const bool vertexOk = m_io->TryTakeResult(slot.vertexRead, vertex)
                && vertex.status == resources::IOStatus::Complete;
            const bool payloadOk = m_io->TryTakeResult(slot.payloadRead, payload)
                && payload.status == resources::IOStatus::Complete;
            slot.vertexRead = 0;
            slot.payloadRead = 0;

            const GPUClusterPage& source = m_dag->Pages()[p];
            if (!vertexOk || !payloadOk)
            {
                RecordPageReadFailure(p, "the store read did not complete");
                continue;
            }
            if (vertex.contentTag != m_storeIdentity || payload.contentTag != m_storeIdentity)
            {
                RecordPageReadFailure(p, "the completed request carried a foreign store tag");
                continue;
            }
            if (vertex.buffer.size() != source.vertexBytes
                || payload.buffer.size() != source.payloadBytes)
            {
                RecordPageReadFailure(p, "the store returned a short page");
                continue;
            }

            slot.vertexStaging = std::move(vertex.buffer);
            slot.payloadStaging = std::move(payload.buffer);
            slot.payloadStaging.resize(size_t(source.payloadBytes) + CLUSTER_PAYLOAD_TAIL_PAD, 0);
            slot.cpuSourced = false;
            slot.readAttempts = 0;
            slot.retryFrame = 0;
            slot.state = PageState::Staged;
            m_uploadQueue.push_back(p);
        }
    }

    RefreshUnreachableGroups();

    const auto& dependencies = m_dag->GroupDependencies();
    m_demandClosure.clear();
    for (u32 group = 0; group < m_groupCount; ++group)
    {
        if (m_groupSlots[group].desired)
            m_demandClosure.push_back(group);
    }
    for (size_t index = 0; index < m_demandClosure.size(); ++index)
    {
        const auto& group = m_groupSlots[m_demandClosure[index]];
        for (u32 i = 0; i < group.dependencyCount; ++i)
        {
            const u32 dependency = dependencies[group.firstDependency + i];
            auto& parent = m_groupSlots[dependency];
            if (parent.desired)
                continue;
            parent.desired = true;
            parent.lastDesiredFrame = m_frameIndex;
            m_demandClosure.push_back(dependency);
            ++m_stats.desiredGroups;
        }
    }
    const auto& memberPages = m_dag->GroupMemberPageList();
    for (auto& group : m_groupSlots)
    {
        if (!group.requested || group.desired || group.pinned)
            continue;
        group.requested = false;
        for (u32 i = 0; i < group.memberPageCount; ++i)
        {
            const u32 page = memberPages[group.firstMemberPage + i];
            auto& slot = m_pageSlots[page];
            R_ASSERT(slot.refs != 0);
            if (--slot.refs == 0)
                RetirePage(page);
        }
    }

    const u32 maxReads = u32(std::max(1, ps_r_geo_page_reads));
    u32 inFlight = 0;
    for (const PageSlot& slot : m_pageSlots) {
        if (slot.state == PageState::Reading)
            ++inFlight;
    }

    m_stats.activatingGroups = 0;
    for (u32 g = 0; g < m_groupCount; ++g) {
        GroupSlot& slot = m_groupSlots[g];
        if (slot.resident || !slot.desired || m_groupUnreachable[g])
            continue;

        bool dependenciesReady = true;
        for (u32 i = 0; i < slot.dependencyCount; ++i)
            dependenciesReady &= m_groupSlots[dependencies[slot.firstDependency + i]].resident;
        if (!dependenciesReady)
        {
            ++m_stats.dependencyStalls;
            ++m_stats.budgetDeferrals;
            continue;
        }
        if (!ReserveGroupPages(g))
        {
            ++m_stats.reservationFailures;
            ++m_stats.allocationDeferrals;
            continue;
        }
        if (!slot.requested)
        {
            for (u32 i = 0; i < slot.memberPageCount; ++i)
                ++m_pageSlots[memberPages[slot.firstMemberPage + i]].refs;
            slot.requested = true;
        }
        m_stats.activatingGroups++;

        for (u32 i = 0; i < slot.memberPageCount; ++i) {
            const u32 page = memberPages[slot.firstMemberPage + i];
            if (page >= u32(m_pageSlots.size()))
                continue;
            if (m_pageSlots[page].state != PageState::Absent)
                continue;
            if (!m_pageSlots[page].runtime && m_dag->LevelPageBytesResident())
            {
                StageFromCook(page);
                continue;
            }
            if (inFlight >= maxReads)
            {
                m_stats.readThrottles++;
                m_stats.budgetDeferrals++;
                break;
            }
            IssuePageRead(page);
            if (m_pageSlots[page].state == PageState::Reading)
                ++inFlight;
        }
    }

    m_stats.readingPages = 0;
    m_stats.uploadingPages = 0;
    m_stats.residentPages = 0;
    m_stats.stagingBytes = 0;
    for (const PageSlot& slot : m_pageSlots) {
        if (slot.state == PageState::Reading)
            m_stats.readingPages++;
        else if (slot.state == PageState::Uploading)
            m_stats.uploadingPages++;
        else if (slot.state == PageState::Resident)
            m_stats.residentPages++;
        m_stats.stagingBytes += slot.vertexStaging.capacity() + slot.payloadStaging.capacity();
    }

    m_stats.residentGroups = 0;
    for (const GroupSlot& slot : m_groupSlots) {
        if (slot.resident)
            m_stats.residentGroups++;
    }
    m_stats.retiringPages = u32(m_retiring.size());
    m_stats.liveSnapshots = u32(m_frames.size());
    const auto bytes = [](nvrhi::IBuffer* buffer) -> u64
    {
        return buffer ? buffer->getDesc().byteSize : 0;
    };
    m_stats.retiringArenaBytes = bytes(m_vertexCopySource) + bytes(m_payloadCopySource);
    for (const auto& frame : m_frames)
        m_stats.retiringArenaBytes += bytes(frame.vertexCopySource) + bytes(frame.payloadCopySource);
    m_stats.reservedHeadroomBytes = m_vertexArena.ReservedBytes() + m_payloadArena.ReservedBytes();
    m_stats.mappingGeneration = m_mappingGeneration;
    m_stats.cutRevision = m_cutRevision;
}

void GeometryResidencyManager::RecordUploads(nvrhi::ICommandList* cmdList)
{
    if (!m_active || !cmdList)
        return;

    if (!m_tablesInitialized) {
        if (!m_clusterGroupTable.empty()) {
            cmdList->writeBuffer(m_clusterGroupBuffer, m_clusterGroupTable.data(),
                m_clusterGroupTable.size() * sizeof(u32));
        }
        if (!m_gpuPages.empty())
            cmdList->writeBuffer(m_pageTableBuffer, m_gpuPages.data(),
                m_gpuPages.size() * sizeof(GPUClusterPage));
        cmdList->writeBuffer(m_groupBitsBuffer, m_groupBits.data(), m_groupBits.size() * sizeof(u32));
        m_dirtyPages.clear();
        m_dirtyGroupWords.clear();
        m_tablesInitialized = true;
    }

    const u64 uploadBudget = u64(std::max(256, ps_r_geo_page_upload)) * 1024ull;
    u64 uploaded = 0;

    FrameRecord frame;
    frame.id = m_snapshotId;
    frame.vertexCopySource = std::move(m_vertexCopySource);
    frame.payloadCopySource = std::move(m_payloadCopySource);
    const auto migrate = [&](nvrhi::IBuffer* source, nvrhi::IBuffer* destination,
        xr_vector<ArenaCopyRange>& ranges)
    {
        if (!source)
        {
            ranges.clear();
            return;
        }
        const u64 limit = std::min(source->getDesc().byteSize, destination->getDesc().byteSize);
        for (const ArenaCopyRange& range : ranges)
        {
            if (range.offset >= limit)
                continue;
            const u64 bytes = std::min(range.bytes, limit - range.offset);
            if (bytes != 0)
                cmdList->copyBuffer(destination, range.offset, source, range.offset, bytes);
        }
        ranges.clear();
    };
    migrate(frame.vertexCopySource, m_vertexArena.buffer, m_vertexCopyRanges);
    migrate(frame.payloadCopySource, m_payloadArena.buffer, m_payloadCopyRanges);
    frame.lease = m_frameLease;

    m_uploadDeferred.clear();
    for (u32 page : m_uploadQueue) {
        if (page >= u32(m_pageSlots.size()))
            continue;
        PageSlot& slot = m_pageSlots[page];
        if (slot.state != PageState::Staged)
            continue;

        u32 vertexSize = 0;
        u32 payloadSize = 0;
        const u8* vertexBytes = nullptr;
        const u8* payloadBytes = nullptr;
        if (slot.cpuSourced) {
            vertexBytes = CookVertexBytes(page, vertexSize);
            payloadBytes = CookPayloadBytes(page, payloadSize);
            if (!vertexBytes || !payloadBytes)
                FATAL_F("[GeoResidency] cook bytes for page %u are no longer retained", page);
        } else {
            vertexBytes = slot.vertexStaging.data();
            vertexSize = u32(slot.vertexStaging.size());
            payloadBytes = slot.payloadStaging.data();
            payloadSize = u32(slot.payloadStaging.size());
        }

        const u64 bytes = u64(vertexSize) + payloadSize;
        if (!slot.pinned && uploaded != 0 && uploaded + bytes > uploadBudget)
        {
            m_uploadDeferred.push_back(page);
            m_stats.uploadThrottles++;
            m_stats.budgetDeferrals++;
            continue;
        }

        if (!AllocatePage(page))
        {
            m_uploadDeferred.push_back(page);
            m_stats.allocationDeferrals++;
            RecordAllocationPressure(page);
            continue;
        }

        if (vertexSize)
            cmdList->writeBuffer(m_vertexArena.buffer, vertexBytes, vertexSize,
                u64(slot.vertexBlock) * GEOMETRY_PAGE_BLOCK_BYTES);
        if (payloadSize)
            cmdList->writeBuffer(m_payloadArena.buffer, payloadBytes, payloadSize,
                u64(slot.payloadBlock) * GEOMETRY_PAGE_BLOCK_BYTES);

        slot.state = PageState::Uploading;
        PublishPage(page);
        frame.uploadedPages.push_back(page);
        frame.usesCookBytes |= slot.cpuSourced && !slot.runtime;
        uploaded += bytes;
        m_stats.uploadsRecorded++;
        m_stats.uploadKiB += u32(bytes / 1024ull);
    }
    m_uploadQueue = m_uploadDeferred;
    EvictUnderPressure();

    bool activated = true;
    while (activated && !m_pendingActivation.empty())
    {
        activated = false;
        size_t remaining = 0;
        for (u32 group : m_pendingActivation)
        {
            R_ASSERT(group < m_groupCount);
            if (TryActivateGroup(group))
                activated = true;
            else
                m_pendingActivation[remaining++] = group;
        }
        m_pendingActivation.resize(remaining);
    }

    m_stats.blockedGroups = 0;
    for (u32 g = 0; g < m_groupCount; ++g) {
        GroupSlot& slot = m_groupSlots[g];
        if (slot.resident || !slot.requested)
            continue;
        if (!TryActivateGroup(g))
            m_stats.blockedGroups++;
    }

    UploadDirtyTables(cmdList);
    UploadShadowCuts(cmdList);

    if (frame.lease || !frame.uploadedPages.empty())
        m_frames.push_back(std::move(frame));

    ReleaseCookBytesIfConfirmed();

    m_stats.vertexUsedBytes = m_vertexArena.UsedBytes();
    m_stats.payloadUsedBytes = m_payloadArena.UsedBytes();
    m_stats.liveSnapshots = u32(m_frames.size());
}

void GeometryResidencyManager::UploadDirtyTables(nvrhi::ICommandList* cmdList)
{
    for (u32 page : m_dirtyPages) {
        if (page >= u32(m_gpuPages.size()))
            continue;
        cmdList->writeBuffer(m_pageTableBuffer, &m_gpuPages[page], sizeof(GPUClusterPage),
            u64(page) * sizeof(GPUClusterPage));
    }
    m_dirtyPages.clear();

    for (u32 word : m_dirtyGroupWords) {
        if (word >= u32(m_groupBits.size()))
            continue;
        cmdList->writeBuffer(m_groupBitsBuffer, &m_groupBits[word], sizeof(u32),
            u64(word) * sizeof(u32));
    }
    m_dirtyGroupWords.clear();
}

bool GeometryResidencyManager::ReserveGroupPages(u32 group)
{
    const auto& slot = m_groupSlots[group];
    const auto& pages = m_dag->GroupMemberPageList();
    m_reservedPages.clear();
    for (u32 i = 0; i < slot.memberPageCount; ++i)
    {
        const u32 page = pages[slot.firstMemberPage + i];
        if (m_pageSlots[page].vertexBlock != GEOMETRY_PAGE_SLOT_INVALID)
            continue;
        if (AllocatePage(page))
        {
            m_reservedPages.push_back(page);
            continue;
        }
        RecordAllocationPressure(page);
        for (u32 reserved : m_reservedPages)
        {
            auto& allocated = m_pageSlots[reserved];
            m_vertexArena.Release(allocated.vertexBlock, allocated.vertexBlocks, allocated.pinned);
            m_payloadArena.Release(allocated.payloadBlock, allocated.payloadBlocks, allocated.pinned);
            allocated.vertexBlock = allocated.payloadBlock = GEOMETRY_PAGE_SLOT_INVALID;
            allocated.vertexBlocks = allocated.payloadBlocks = 0;
        }
        return false;
    }
    return true;
}

void GeometryResidencyManager::EvictUnderPressure()
{
    if (m_fineGroupCount == 0)
        return;

    const u64 vertexHighWater = (m_vertexArena.FineBudgetBytes() * 15ull) / 16ull;
    const u64 payloadHighWater = (m_payloadArena.FineBudgetBytes() * 15ull) / 16ull;

    size_t scanned = 0;
    u64 retiringVertexBytes = 0;
    u64 retiringPayloadBytes = 0;
    const auto absorbRetiring = [&]()
    {
        for (; scanned < m_retiring.size(); ++scanned)
        {
            const RetiringPage& retiring = m_retiring[scanned];
            if (retiring.pinned)
                continue;
            retiringVertexBytes += u64(retiring.vertexBlocks) * GEOMETRY_PAGE_BLOCK_BYTES;
            retiringPayloadBytes += u64(retiring.payloadBlocks) * GEOMETRY_PAGE_BLOCK_BYTES;
        }
    };
    absorbRetiring();

    for (u32 pass = 0; pass < GEOMETRY_EVICTION_LIMIT; ++pass)
    {
        const u64 vertexCommitted = m_vertexArena.FineUsedBytes()
            - std::min(m_vertexArena.FineUsedBytes(), retiringVertexBytes);
        const u64 payloadCommitted = m_payloadArena.FineUsedBytes()
            - std::min(m_payloadArena.FineUsedBytes(), retiringPayloadBytes);
        const bool overHighWater = vertexCommitted > vertexHighWater || payloadCommitted > payloadHighWater;
        const bool starved = retiringVertexBytes < m_pressureVertexBytes
            || retiringPayloadBytes < m_pressurePayloadBytes;
        if (!overHighWater && !starved)
            break;

        u32 victim = UINT32_MAX;
        u32 oldest = UINT32_MAX;
        for (u32 g = 0; g < m_groupCount; ++g)
        {
            const GroupSlot& slot = m_groupSlots[g];
            if (!slot.resident || slot.pinned || slot.desired || slot.dependentResidents != 0)
                continue;
            if (slot.lastDesiredFrame <= oldest)
            {
                oldest = slot.lastDesiredFrame;
                victim = g;
            }
        }
        if (victim == UINT32_MAX)
            break;
        EvictGroup(victim);
        absorbRetiring();
    }
}

void GeometryResidencyManager::ReserveRuntimeArena(Arena& arena, u64 bytes, bool force,
    nvrhi::BufferHandle& source, xr_vector<ArenaCopyRange>& ranges)
{
    const u64 required = AlignBytes(bytes);
    if (required == 0)
        return;

    const u64 credited = force ? 0ull : arena.ReservedBytes();
    if (required <= credited)
        return;

    const u64 deficit = required - credited;
    const u64 minimum = arena.bytes + deficit;
    if (minimum > GEOMETRY_ARENA_MAX_BYTES)
        FATAL_F("[GeoResidency] pinned runtime geometry needs %llu arena bytes beyond the shader address range",
            (unsigned long long)minimum);

    const u64 headroom = std::min(std::max(deficit, arena.bytes / 8ull), GEOMETRY_ARENA_HEADROOM_MAX);
    const u64 target = AlignBytes(std::min(minimum + headroom, GEOMETRY_ARENA_MAX_BYTES));
    const u32 added = BlocksFor(target) - arena.blockCount;

    if (!source)
    {
        source = arena.buffer;
        arena.CollectLiveRanges(ranges);
    }
    auto desc = arena.buffer->getDesc();
    desc.byteSize = target;
    arena.buffer = m_device->GetNVRHIDevice()->createBuffer(desc);
    R_ASSERT(arena.buffer);
    arena.Grow(target);
    arena.reservedBlocks += added;
}

nvrhi::IBuffer* GeometryResidencyManager::GetArenaCopySource(bool vertices) const
{
    return vertices ? m_vertexCopySource.Get() : m_payloadCopySource.Get();
}

nvrhi::IBuffer* GeometryResidencyManager::GetShadowCutBuffer() const
{
    return m_shadowCutBuffers;
}

void GeometryResidencyManager::UploadShadowCuts(nvrhi::ICommandList* cmdList)
{
    if (m_cutStagingRevision != m_cutRevision)
    {
        m_cutStaging.clear();
        for (const auto& cut : m_cutBounds)
        {
            if (cut.revision != 0)
                m_cutStaging.push_back(cut);
        }
        std::sort(m_cutStaging.begin(), m_cutStaging.end(),
            [](const GeometryCutBounds& a, const GeometryCutBounds& b)
            {
                return a.revision < b.revision;
            });
        m_cutStagingRevision = m_cutRevision;
    }
    const u32 header[4] = { u32(m_cutStaging.size()), m_cutRevision, 0, 0 };
    cmdList->writeBuffer(m_shadowCutBuffers, header, sizeof(header));
    if (!m_cutStaging.empty())
        cmdList->writeBuffer(m_shadowCutBuffers, m_cutStaging.data(),
            m_cutStaging.size() * sizeof(GeometryCutBounds), sizeof(header));
}

void GeometryResidencyManager::EndFrame(IRenderBackend* backend)
{
    if (!m_active)
        return;

    if (m_frameLease) {
        bool tracked = false;
        for (const FrameRecord& frame : m_frames)
            tracked = tracked || frame.lease == m_frameLease;
        if (!tracked && backend)
            backend->ReleaseSubmissionLease(m_frameLease);
    }
    m_frameLease = 0;
}

bool GeometryResidencyManager::IsActive() const
{
    return m_active;
}

bool GeometryResidencyManager::IsStreaming() const
{
    return m_active && m_fineGroupCount != 0;
}

nvrhi::IBuffer* GeometryResidencyManager::GetPageTableBuffer() const
{
    return m_pageTableBuffer.Get();
}

nvrhi::IBuffer* GeometryResidencyManager::GetGroupBitsBuffer() const
{
    return m_groupBitsBuffer.Get();
}

nvrhi::IBuffer* GeometryResidencyManager::GetClusterGroupBuffer() const
{
    return m_clusterGroupBuffer.Get();
}

nvrhi::IBuffer* GeometryResidencyManager::GetVertexArenaBuffer() const
{
    return m_vertexArena.buffer.Get();
}

nvrhi::IBuffer* GeometryResidencyManager::GetPayloadArenaBuffer() const
{
    return m_payloadArena.buffer.Get();
}

u32 GeometryResidencyManager::GroupCount() const
{
    return m_groupCount;
}

u32 GeometryResidencyManager::MappingGeneration() const
{
    return m_mappingGeneration;
}

u32 GeometryResidencyManager::CutRevision() const
{
    return m_cutRevision;
}

const xr_vector<u32>& GeometryResidencyManager::CutChangedGroups() const
{
    return m_cutChangedGroups;
}

const GeometryResidencyStats& GeometryResidencyManager::Stats() const
{
    return m_stats;
}

u64 GeometryResidencyManager::Arena::UsedBytes() const
{
    return u64(usedBlocks) * GEOMETRY_PAGE_BLOCK_BYTES;
}

u64 GeometryResidencyManager::Arena::ReservedBytes() const
{
    return u64(reservedBlocks) * GEOMETRY_PAGE_BLOCK_BYTES;
}

u64 GeometryResidencyManager::Arena::FineUsedBytes() const
{
    return u64(usedBlocks - std::min(usedBlocks, pinnedBlocks)) * GEOMETRY_PAGE_BLOCK_BYTES;
}

u64 GeometryResidencyManager::Arena::FineBudgetBytes() const
{
    const u32 committed = std::min(blockCount, pinnedBlocks + reservedBlocks);
    return u64(blockCount - committed) * GEOMETRY_PAGE_BLOCK_BYTES;
}

} // namespace xray::render::fg
