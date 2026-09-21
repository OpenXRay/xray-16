#include "stdafx.h"
#include "FGDetailManager.h"
#include "FrameGraph/BindingSetBuilder.h"
#include "FrameGraph/PassResourceCache.h"
#include "FrameGraph/ShaderLoader.h"
#include "FrameGraphPasses/VisibilityPassSetup.h"
#include "RenderContext/RenderDevice.h"
#include "xrRender_console.h"
#include "xrCDB/Frustum.h"
#include "xrEngine/device.h"

extern ENGINE_API float ps_r3_grass_lod_close;
extern ENGINE_API float ps_r3_grass_lod_mid;

namespace xray::render::fg
{

u64 FGDetailManager::CombineContentHash(u64 hash, u64 value)
{
    hash ^= value + 0x9E3779B97F4A7C15ull + (hash << 6) + (hash >> 2);
    return hash;
}

u64 FGDetailManager::FinalizeContentHash(u64 hash)
{
    hash ^= hash >> 30;
    hash *= 0xBF58476D1CE4E5B9ull;
    hash ^= hash >> 27;
    hash *= 0x94D049BB133111EBull;
    hash ^= hash >> 31;
    return hash;
}

u64 FGDetailManager::ComposeContentSignature(u64 sourceId, const DetailCullingStats& stats,
    const DetailMembershipFingerprint& fingerprint)
{
    u64 hash = CombineContentHash(0x9E3779B97F4A7C15ull, sourceId);
    hash = CombineContentHash(hash, stats.visibleLOD0Count);
    hash = CombineContentHash(hash, stats.visibleLOD1Count);
    hash = CombineContentHash(hash, stats.visibleLOD2Count);
    hash = CombineContentHash(hash, stats.visibleBillboardCount);
    hash = CombineContentHash(hash, fingerprint.membership0);
    hash = CombineContentHash(hash, fingerprint.membership1);
    return FinalizeContentHash(hash);
}

void FGDetailManager::UpdateInstanceMemoryStats()
{
    instanceMemoryStats = {};
    if (GEnv.Backend)
        instanceMemoryStats.device = GEnv.Backend->GetMemoryBudget();
    for (auto it = m_instanceGenerations.begin(); it != m_instanceGenerations.end();)
    {
        if (auto source = it->lock())
        {
            instanceMemoryStats.residentInstances += source->instanceCount;
            instanceMemoryStats.residentBytes += source->bytes;
            instanceMemoryStats.residentChunks += u32(source->chunks.size());
            ++instanceMemoryStats.generations;
            ++it;
        }
        else
            it = m_instanceGenerations.erase(it);
    }
    if (generatedInstances)
        instanceMemoryStats.activeBytes = generatedInstances->bytes;
    for (const auto& frame : m_visibilityFrames)
        instanceMemoryStats.frameBytes += frame->bytes;
    instanceMemoryStats.frames = u32(m_visibilityFrames.size());
    if (generationWork)
        instanceMemoryStats.pendingBytes = generationWork->bytes;
}

void FGDetailManager::RequireInstanceMemory(u64 bytes, const char* purpose)
{
    UpdateInstanceMemoryStats();
    const auto& memory = instanceMemoryStats.device;
    if (!memory.budgetBytes)
        return;
    u64 used = memory.usageBytes;
    if (!memory.usageKnown)
        used = instanceMemoryStats.residentBytes + instanceMemoryStats.frameBytes + instanceMemoryStats.pendingBytes;
    const u64 limit = memory.budgetBytes - memory.budgetBytes / 20;
    const u64 available = used < limit ? limit - used : 0;
    if (bytes > available)
        FATAL_F("[DetailManager] %s needs %llu additional bytes; %llu bytes available below the GPU budget with 5%% headroom (budget=%llu usage=%llu usageKnown=%u). Density was not reduced.",
            purpose, static_cast<unsigned long long>(bytes), static_cast<unsigned long long>(available),
            static_cast<unsigned long long>(memory.budgetBytes), static_cast<unsigned long long>(used), u32(memory.usageKnown));
}

nvrhi::BufferHandle FGDetailManager::CreateInstanceBuffer(nvrhi::IDevice* device, u64 bytes, u32 stride,
    const char* name, bool readback, bool indirect)
{
    const u64 size = std::max(bytes, u64(stride ? stride : sizeof(u32)));
    const u64 range = instanceMemoryStats.device.bufferRangeBytes;
    if (!readback && range && size > range)
        FATAL_F("[DetailManager] %s needs a %llu-byte buffer view; native range is %llu bytes",
            name, static_cast<unsigned long long>(size), static_cast<unsigned long long>(range));
    nvrhi::BufferDesc desc;
    desc.byteSize = size;
    desc.structStride = readback ? 0 : stride;
    desc.debugName = name;
    desc.canHaveUAVs = !readback;
    desc.canHaveRawViews = !readback && !stride;
    desc.isDrawIndirectArgs = indirect;
    desc.cpuAccess = readback ? nvrhi::CpuAccessMode::Read : nvrhi::CpuAccessMode::None;
    desc.initialState = readback ? nvrhi::ResourceStates::CopyDest : nvrhi::ResourceStates::NonPixelShaderResource;
    desc.keepInitialState = true;
    auto buffer = device->createBuffer(desc);
    if (!buffer)
        FATAL_F("[DetailManager] native allocation failed for %s (%llu bytes); density was not reduced",
            name, static_cast<unsigned long long>(size));
    return buffer;
}

bool FGDetailManager::IsChunkVisible(const InstanceChunk& chunk, const DetailCullParams& params) const
{
    for (u32 i = 0; i < 5; ++i)
    {
        const auto& p = params.frustumPlanes[i];
        const float x = p.x * (p.x < 0.f ? chunk.bounds.vMax.x : chunk.bounds.vMin.x);
        const float y = p.y * (p.y < 0.f ? chunk.bounds.vMax.y : chunk.bounds.vMin.y);
        const float z = p.z * (p.z < 0.f ? chunk.bounds.vMax.z : chunk.bounds.vMin.z);
        const float tolerance = 0.0001f * (std::abs(x) + std::abs(y) + std::abs(z) + std::abs(p.w) + 1.f);
        if (x + y + z + p.w > tolerance)
            return false;
    }
    return true;
}

void FGDetailManager::AllocateGeneration(nvrhi::IDevice* device, GenerationWork& work)
{
    const auto* counts = static_cast<const u32*>(device->mapBuffer(work.countReadback, nvrhi::CpuAccessMode::Read));
    R_ASSERT2(counts, "[DetailManager] completed instance count readback could not be mapped");
    work.source = std::make_shared<InstanceGeneration>();
    auto& source = *work.source;
    source.id = ++m_instanceGenerationId;
    source.params = work.params;
    source.models = detailModelsBuffer;
    source.pulledVertices = pulledVertexBuffer;
    source.maxPulledIndexCount = maxPulledIndexCount;
    work.slots = slot_aabbs;
    work.slotOrder.reserve(slot_count);
    for (u32 slot = 0; slot < slot_count; ++slot)
    {
        const u32* count = counts + u64(slot) * 4;
        R_ASSERT2(u64(count[1]) + count[2] == count[0], "[DetailManager] instance category count overflow");
        if (count[0])
            work.slotOrder.push_back(slot);
    }
    const u32 width = dtH.x_size();
    const u32 tileWidth = (width + 31u) / 32u;
    std::sort(work.slotOrder.begin(), work.slotOrder.end(), [width, tileWidth](u32 a, u32 b)
    {
        const u32 tileA = (a / width / 32u) * tileWidth + (a % width / 32u);
        const u32 tileB = (b / width / 32u) * tileWidth + (b % width / 32u);
        return tileA != tileB ? tileA < tileB : a < b;
    });
    UpdateInstanceMemoryStats();
    const u64 nativeRange = instanceMemoryStats.device.bufferRangeBytes;
    const u64 chunkBytes = nativeRange ? std::min<u64>(16ull * 1024 * 1024, nativeRange) : 16ull * 1024 * 1024;
    const u32 chunkCapacity = u32(chunkBytes / sizeof(InstanceData));
    R_ASSERT2(chunkCapacity, "[DetailManager] native storage-buffer range cannot hold an instance");
    const float jitterPadding = std::max(work.params.detailDensity, 0.001f);
    for (u32 order = 0; order < work.slotOrder.size(); ++order)
    {
        const u32 slot = work.slotOrder[order];
        const u32* count = counts + u64(slot) * 4;
        if (nativeRange && u64(count[0]) * sizeof(InstanceData) > nativeRange)
            FATAL_F("[DetailManager] detail cell %u needs %llu source bytes beyond the native buffer range", slot,
                static_cast<unsigned long long>(u64(count[0]) * sizeof(InstanceData)));
        if (source.chunks.empty() || u64(source.chunks.back().instanceCount) + count[0] > chunkCapacity)
        {
            source.chunks.emplace_back();
            source.chunks.back().firstSlot = order;
            source.chunks.back().bounds.invalidate();
        }
        auto& chunk = source.chunks.back();
        auto& bounds = work.slots[slot];
        bounds.instance_chunk = u32(source.chunks.size() - 1);
        bounds.instance_base = chunk.instanceCount;
        bounds.instance_count = count[0];
        chunk.bounds.modify(bounds.aabb_min);
        chunk.bounds.modify(bounds.aabb_max);
        const auto& position = slotDataCPU[slot];
        const float minX = position.world_min_x - jitterPadding;
        const float minZ = position.world_min_z - jitterPadding;
        const float maxX = position.world_min_x + 2.f + jitterPadding;
        const float maxZ = position.world_min_z + 2.f + jitterPadding;
        if (!chunk.slotCount)
        {
            chunk.sourceMin.set(minX, minZ);
            chunk.sourceMax.set(maxX, maxZ);
        }
        else
        {
            chunk.sourceMin.x = std::min(chunk.sourceMin.x, minX);
            chunk.sourceMin.y = std::min(chunk.sourceMin.y, minZ);
            chunk.sourceMax.x = std::max(chunk.sourceMax.x, maxX);
            chunk.sourceMax.y = std::max(chunk.sourceMax.y, maxZ);
        }
        ++chunk.slotCount;
        chunk.instanceCount += count[0];
        chunk.wavingCount += count[1];
        chunk.staticCount += count[2];
        source.instanceCount += count[0];
    }
    device->unmapBuffer(work.countReadback);
    work.counts = nullptr;
    work.countReadback = nullptr;
    work.bytes = 0;
    if (source.chunks.empty())
        return;
    const u64 slotBytes = u64(slot_count) * sizeof(SlotAABB);
    const u64 localBytes = u64(slot_count) * sizeof(u32);
    const u64 orderBytes = u64(work.slotOrder.size()) * sizeof(u32);
    const u64 sourceBytes = source.instanceCount * sizeof(InstanceData) + slotBytes;
    RequireInstanceMemory(sourceBytes + localBytes + orderBytes + 2 * sizeof(u32), "immutable detail generation");
    source.slots = CreateInstanceBuffer(device, slotBytes, sizeof(SlotAABB), "DetailGenerationSlots");
    work.localCounters = CreateInstanceBuffer(device, localBytes, sizeof(u32), "DetailGenerationLocalCounters");
    work.emitSlots = CreateInstanceBuffer(device, orderBytes, sizeof(u32), "DetailGenerationEmitSlots");
    work.status = CreateInstanceBuffer(device, sizeof(u32), 0, "DetailGenerationStatus");
    work.statusReadback = CreateInstanceBuffer(device, sizeof(u32), 0, "DetailGenerationStatusReadback", true);
    work.bytes = localBytes + orderBytes + 2 * sizeof(u32);
    for (u32 i = 0; i < source.chunks.size(); ++i)
    {
        auto& chunk = source.chunks[i];
        string128 name;
        xr_sprintf(name, "DetailSourceChunk%llu.%u", static_cast<unsigned long long>(source.id), i);
        chunk.buffer = CreateInstanceBuffer(device, u64(chunk.instanceCount) * sizeof(InstanceData), sizeof(InstanceData), name);
    }
    nvrhi::BindlessLayoutDesc layout;
    layout.visibility = nvrhi::ShaderType::All;
    layout.firstSlot = 0;
    layout.maxCapacity = u32(source.chunks.size());
    layout.registerSpaces = { nvrhi::BindingLayoutItem::StructuredBuffer_SRV(2) };
    source.bindingLayout = device->createBindlessLayout(layout);
    R_ASSERT2(source.bindingLayout, "[DetailManager] source descriptor layout allocation failed");
    source.descriptorTable = device->createDescriptorTable(source.bindingLayout);
    R_ASSERT2(source.descriptorTable, "[DetailManager] source descriptor table allocation failed");
    device->resizeDescriptorTable(source.descriptorTable, u32(source.chunks.size()), false);
    for (u32 i = 0; i < source.chunks.size(); ++i)
        R_ASSERT2(device->writeDescriptorTable(source.descriptorTable,
            nvrhi::BindingSetItem::StructuredBuffer_SRV(i, source.chunks[i].buffer)),
            "[DetailManager] source descriptor publication failed");
    source.bytes = sourceBytes;
    m_instanceGenerations.emplace_back(work.source);
    work.stage = GenerationWork::Stage::EmitReady;
}

void FGDetailManager::AllocateVisibilityFrame(nvrhi::IDevice* device, VisibilityFrame& frame)
{
    auto& params = frame.cullParams;
    params = {};
    params.viewProj = Device.mFullTransform;
    params.cameraPos = Device.vCameraPosition;
    params.lodDistanceCloseSqr = ps_r3_grass_lod_close * ps_r3_grass_lod_close;
    params.lodDistanceMidSqr = ps_r3_grass_lod_mid * ps_r3_grass_lod_mid;
    CFrustum frustum;
    frustum.CreateFromMatrix(params.viewProj, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);
    for (u32 i = 0; i < 6; ++i)
    {
        if (i < u32(frustum.p_count))
            params.frustumPlanes[i].set(frustum.planes[i].n.x, frustum.planes[i].n.y, frustum.planes[i].n.z, frustum.planes[i].d);
        else
            params.frustumPlanes[i].set(0.f, 0.f, 0.f, -1000000.f);
    }
    u64 potential[VIS_KIND_COUNT] = {};
    frame.visibleChunks.clear();
    if (frame.source)
    {
        params.grassMode = frame.source->params.grassMode;
        params.detailDensity = frame.source->params.detailDensity;
        for (u32 i = 0; i < frame.source->chunks.size(); ++i)
        {
            const auto& chunk = frame.source->chunks[i];
            if (!IsChunkVisible(chunk, params))
                continue;
            frame.visibleChunks.push_back(i);
            potential[VIS_KIND_DECAL] += chunk.staticCount;
            if (!params.grassMode)
                potential[VIS_KIND_MESH] += chunk.wavingCount;
            else
            {
                const float dx = std::max({ chunk.sourceMin.x - params.cameraPos.x, 0.f, params.cameraPos.x - chunk.sourceMax.x });
                const float dz = std::max({ chunk.sourceMin.y - params.cameraPos.z, 0.f, params.cameraPos.z - chunk.sourceMax.y });
                const float distance = dx * dx + dz * dz;
                if (distance <= params.lodDistanceCloseSqr)
                    potential[0] += chunk.wavingCount;
                if (distance <= params.lodDistanceMidSqr)
                    potential[1] += chunk.wavingCount;
                potential[2] += chunk.wavingCount;
            }
        }
    }
    UpdateInstanceMemoryStats();
    const u64 range = instanceMemoryStats.device.bufferRangeBytes;
    const u64 maximum = range ? std::min<u64>(UINT32_MAX, range / 8) : UINT32_MAX;
    u32 desired[VIS_KIND_COUNT];
    u32 prepared[LOD_COUNT];
    u64 additional = 0;
    const auto additionalBytes = [](const nvrhi::BufferHandle& buffer, u64 bytes)
    {
        return !buffer || buffer->getDesc().byteSize < bytes ? bytes : 0ull;
    };
    for (u32 kind = 0; kind < VIS_KIND_COUNT; ++kind)
    {
        desired[kind] = u32(std::max<u64>(1, std::min(potential[kind], maximum)));
        additional += additionalBytes(frame.visible[kind], u64(desired[kind]) * 8);
        additional += additionalBytes(frame.drawArgs[kind], 5 * sizeof(u32));
        if (kind < LOD_COUNT)
        {
            prepared[kind] = std::min(desired[kind], PREPARED_BLADE_CAPACITY);
            additional += additionalBytes(frame.prepared[kind], u64(prepared[kind]) * 36);
        }
    }
    const u64 slotBytes = frame.source && !frame.source->chunks.empty() ? u64(slot_count) * sizeof(u32) : sizeof(u32);
    additional += additionalBytes(frame.visibleSlots, slotBytes);
    additional += additionalBytes(frame.visibleSlotCount, 4);
    additional += additionalBytes(frame.slotDispatch, 12);
    additional += additionalBytes(frame.swDispatch, 32);
    additional += additionalBytes(frame.packets, VIS_KIND_COUNT * 16);
    additional += additionalBytes(frame.workStatus, VISIBILITY_STATUS_BYTES);
    additional += additionalBytes(frame.readback, VISIBILITY_STATUS_BYTES);
    RequireInstanceMemory(additional, "detail visibility frame");
    const auto ensure = [&](nvrhi::BufferHandle& buffer, u64 bytes, u32 stride, const char* name,
        bool readback = false, bool indirect = false)
    {
        if (!buffer || buffer->getDesc().byteSize < bytes)
            buffer = CreateInstanceBuffer(device, bytes, stride, name, readback, indirect);
    };
    frame.bytes = 0;
    static constexpr const char* visibleNames[] = { "DetailVisibleLod0", "DetailVisibleLod1", "DetailVisibleLod2", "DetailVisibleMesh", "DetailVisibleDecal" };
    static constexpr const char* argsNames[] = { "DetailArgsLod0", "DetailArgsLod1", "DetailArgsLod2", "DetailArgsMesh", "DetailArgsDecal" };
    static constexpr const char* preparedNames[] = { "DetailPreparedLod0", "DetailPreparedLod1", "DetailPreparedLod2" };
    for (u32 kind = 0; kind < VIS_KIND_COUNT; ++kind)
    {
        ensure(frame.visible[kind], u64(desired[kind]) * 8, 8, visibleNames[kind]);
        ensure(frame.drawArgs[kind], 5 * sizeof(u32), 0, argsNames[kind], false, true);
        frame.visibleCapacity[kind] = u32(frame.visible[kind]->getDesc().byteSize / 8);
        frame.bytes += frame.visible[kind]->getDesc().byteSize + frame.drawArgs[kind]->getDesc().byteSize;
        if (kind < LOD_COUNT)
        {
            ensure(frame.prepared[kind], u64(prepared[kind]) * 36, 36, preparedNames[kind]);
            frame.preparedCapacity[kind] = u32(frame.prepared[kind]->getDesc().byteSize / 36);
            frame.bytes += frame.prepared[kind]->getDesc().byteSize;
            params.visibleBladeCapacity[kind] = frame.visibleCapacity[kind];
            params.preparedCapacity[kind] = frame.preparedCapacity[kind];
        }
    }
    ensure(frame.visibleSlots, slotBytes, sizeof(u32), "DetailVisibleSlots");
    ensure(frame.visibleSlotCount, 4, 0, "DetailVisibleSlotCount");
    ensure(frame.slotDispatch, 12, 0, "DetailSlotDispatch", false, true);
    ensure(frame.swDispatch, 32, 0, "DetailSwDispatch", false, true);
    ensure(frame.packets, VIS_KIND_COUNT * 16, 16, "DetailVisibilityPackets");
    ensure(frame.workStatus, VISIBILITY_STATUS_BYTES, 0, "DetailVisibilityStatus");
    ensure(frame.readback, VISIBILITY_STATUS_BYTES, 0, "DetailVisibilityReadback", true);
    frame.bytes += frame.visibleSlots->getDesc().byteSize + 4 + 12 + 32 + VIS_KIND_COUNT * 16 + VISIBILITY_STATUS_BYTES + VISIBILITY_STATUS_BYTES;
    params.totalSlotCount = slot_count;
    params.visibleBillboardCapacity = frame.visibleCapacity[VIS_KIND_MESH];
    params.visibleDecalCapacity = frame.visibleCapacity[VIS_KIND_DECAL];
}

void FGDetailManager::ProcessStatsReadback(nvrhi::IDevice* device)
{
    if (!device || !GEnv.Backend)
        return;
    for (const auto& frame : m_visibilityFrames)
    {
        if (!frame->lease)
            continue;
        GEnv.Backend->CloseSubmissionLease(frame->lease);
        const auto state = GEnv.Backend->PollSubmissionLease(frame->lease);
        if (state == IRenderBackend::SubmissionLeaseState::Failed || state == IRenderBackend::SubmissionLeaseState::Unknown)
            FATAL("[DetailManager] detail visibility submission failed");
        if (state != IRenderBackend::SubmissionLeaseState::Complete)
            continue;
        GEnv.Backend->ReleaseSubmissionLease(frame->lease);
        frame->lease = 0;
        if (!frame->statsRecorded)
            continue;
        const void* mapped = device->mapBuffer(frame->readback, nvrhi::CpuAccessMode::Read);
        R_ASSERT2(mapped, "[DetailManager] completed visibility readback could not be mapped");
        static_assert(sizeof(DetailCullingStats) == 32);
        std::memcpy(&frame->stats, mapped, sizeof(DetailCullingStats));
        DetailMembershipFingerprint fingerprint;
        std::memcpy(&fingerprint, static_cast<const u8*>(mapped) + sizeof(DetailCullingStats), sizeof(fingerprint));
        device->unmapBuffer(frame->readback);
        if (frame->stats.overflowFlags)
            FATAL_F("[DetailManager] visible work overflow: flags=%u packets=%u availableEntries=%u counts=%u/%u/%u/%u/%u capacities=%u/%u/%u/%u/%u. No aliased IDs or partial density were drawn.",
                frame->stats.overflowFlags, frame->stats.packetCount, passes::kVisIdEntryLimit - frame->entryBase,
                frame->stats.visibleLOD0Count, frame->stats.visibleLOD1Count, frame->stats.visibleLOD2Count,
                frame->stats.visibleBillboardCount, frame->stats.visibleDecalCount,
                frame->visibleCapacity[0], frame->visibleCapacity[1], frame->visibleCapacity[2],
                frame->visibleCapacity[3], frame->visibleCapacity[4]);
        frame->contentSignature = ComposeContentSignature(frame->source ? frame->source->id : 0, frame->stats, fingerprint);
        frame->statsReady = true;
        if (!m_completedVisibilityFrame || frame->id > m_completedVisibilityFrame->id)
        {
            m_completedVisibilityFrame = frame;
            cullingStats = frame->stats;
        }
    }
}

std::shared_ptr<const FGDetailManager::VisibilityFrame> FGDetailManager::GetCompletedVisibilityFrame() const
{
    return m_detailsEnabled ? m_completedVisibilityFrame : nullptr;
}

void FGDetailManager::PrepareFrame(nvrhi::IDevice* device, u32 entryBase, bool enabled)
{
    m_detailsEnabled = enabled;
    visibilityFrame.reset();
    ProcessStatsReadback(device);
    for (const auto& frame : m_visibilityFrames)
        if (!frame->lease && frame.use_count() == 1)
            frame->source.reset();
    if (!device || !slot_count || !slotDataBuffer)
        return;
    R_ASSERT2(GEnv.Backend && GEnv.Backend->SupportsSubmissionLeases(), "[DetailManager] source and visibility snapshots require native submission completion");
    R_ASSERT2(entryBase <= passes::kVisIdEntryLimit, "[DetailManager] rigid and skinned visibility entries exhaust the identifier range");
    const auto requested = [&](const InstanceGenParams& params)
    {
        return params.detailDensity == ps_current_detail_density && params.detailHeightMultiplier == ps_current_detail_height &&
            params.grassMode == (ps_r__detail_gpu ? 1u : 0u) && params.detailModelCount == detail_models.size();
    };
    const auto publish = [&]()
    {
        generatedInstances = generationWork->source;
        totalGeneratedInstances = generatedInstances->instanceCount;
        Msg("[DetailManager] published source generation %llu: %llu instances, %u chunks, %llu bytes, density=%.3f mode=%u",
            static_cast<unsigned long long>(generatedInstances->id), static_cast<unsigned long long>(totalGeneratedInstances),
            u32(generatedInstances->chunks.size()), static_cast<unsigned long long>(generatedInstances->bytes),
            generatedInstances->params.detailDensity, generatedInstances->params.grassMode);
        generationWork.reset();
    };
    if (generationWork && generationWork->lease)
    {
        auto& work = *generationWork;
        GEnv.Backend->CloseSubmissionLease(work.lease);
        const auto state = GEnv.Backend->PollSubmissionLease(work.lease);
        if (state == IRenderBackend::SubmissionLeaseState::Failed || state == IRenderBackend::SubmissionLeaseState::Unknown)
            FATAL("[DetailManager] source generation submission failed");
        if (state == IRenderBackend::SubmissionLeaseState::Complete)
        {
            GEnv.Backend->ReleaseSubmissionLease(work.lease);
            work.lease = 0;
            if (m_instancesNeedRegeneration || !requested(work.params))
                generationWork.reset();
            else if (work.stage == GenerationWork::Stage::CountPending)
            {
                AllocateGeneration(device, work);
                if (work.source->chunks.empty())
                    publish();
            }
            else if (work.stage == GenerationWork::Stage::EmitPending)
            {
                const auto* status = static_cast<const u32*>(device->mapBuffer(work.statusReadback, nvrhi::CpuAccessMode::Read));
                R_ASSERT2(status, "[DetailManager] completed generation status could not be mapped");
                const u32 error = *status;
                device->unmapBuffer(work.statusReadback);
                if (error)
                    FATAL_F("[DetailManager] source count/scatter mismatch (flags=%u); replacement generation was not published", error);
                work.nextChunk += work.submittedChunks;
                work.submittedChunks = 0;
                if (work.nextChunk == work.source->chunks.size())
                    publish();
                else
                    work.stage = GenerationWork::Stage::EmitReady;
            }
        }
    }
    if (generationWork && !generationWork->lease && (m_instancesNeedRegeneration || !requested(generationWork->params)))
        generationWork.reset();
    if (!enabled)
    {
        UpdateInstanceMemoryStats();
        return;
    }
    if (!generationWork && (m_instancesNeedRegeneration || !generatedInstances || !requested(generatedInstances->params)))
    {
        const u64 countBytes = u64(slot_count) * 16;
        RequireInstanceMemory(countBytes * 2, "detail generation counts");
        generationWork = std::make_shared<GenerationWork>();
        auto& work = *generationWork;
        work.params.heightmapWorldMinX = heightmapWorldMinX;
        work.params.heightmapWorldMinZ = heightmapWorldMinZ;
        work.params.heightmapTexelSize = heightmapTexelSize;
        work.params.detailHeightMultiplier = ps_current_detail_height;
        work.params.slotCount = slot_count;
        work.params.detailModelCount = u32(detail_models.size());
        work.params.detailDensity = ps_current_detail_density;
        work.params.grassMode = ps_r__detail_gpu ? 1u : 0u;
        work.counts = CreateInstanceBuffer(device, countBytes, 16, "DetailGenerationCounts");
        work.countReadback = CreateInstanceBuffer(device, countBytes, 0, "DetailGenerationCountReadback", true);
        work.bytes = countBytes * 2;
        m_instancesNeedRegeneration = false;
    }
    for (const auto& frame : m_visibilityFrames)
    {
        if (!frame->lease && frame.use_count() == 1)
        {
            visibilityFrame = frame;
            break;
        }
    }
    if (!visibilityFrame)
    {
        visibilityFrame = std::make_shared<VisibilityFrame>();
        m_visibilityFrames.push_back(visibilityFrame);
    }
    visibilityFrame->source = generatedInstances;
    visibilityFrame->entryBase = entryBase;
    visibilityFrame->id = ++m_visibilityFrameId;
    visibilityFrame->statsRecorded = false;
    visibilityFrame->statsReady = false;
    visibilityFrame->stats = {};
    visibilityFrame->contentSignature = 0;
    AllocateVisibilityFrame(device, *visibilityFrame);
    if (generatedInstances && !generatedInstances->chunks.empty())
        R_ASSERT2(CreateComputePipeline(GEnv.Render->GetRenderDevice()), "[DetailManager] source-compatible culling pipeline unavailable");
    UpdateInstanceMemoryStats();
}

void FGDetailManager::RecordGeneration(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device, GenerationWork& work)
{
    if (work.stage != GenerationWork::Stage::CountReady && work.stage != GenerationWork::Stage::EmitReady)
        return;
    const bool count = work.stage == GenerationWork::Stage::CountReady;
    auto* renderDevice = GEnv.Render->GetRenderDevice();
    auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection(count ? "detail_instance_count" : "detail_instance_gen", ".cs");
    R_ASSERT2(reflection && instanceCountPipeline && instanceGenPipeline && heightmapTexture,
        "[DetailManager] source generation pipeline or heightmap unavailable");
    work.lease = GEnv.Backend->OpenSubmissionLease();
    R_ASSERT2(work.lease, "[DetailManager] source generation submission lease unavailable");
    if (count)
        cmdList->clearBufferUInt(work.counts, 0u);
    else if (work.nextChunk == 0)
    {
        cmdList->writeBuffer(work.source->slots, work.slots.data(), work.slots.size() * sizeof(SlotAABB));
        cmdList->writeBuffer(work.emitSlots, work.slotOrder.data(), work.slotOrder.size() * sizeof(u32));
        cmdList->clearBufferUInt(work.localCounters, 0u);
        cmdList->clearBufferUInt(work.status, 0u);
    }
    const u32 dispatches = count ? 1u : std::min(u32(work.source->chunks.size()) - work.nextChunk, fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS);
    work.submittedChunks = count ? 0u : dispatches;
    for (u32 i = 0; i < dispatches; ++i)
    {
        InstanceGenParams params = work.params;
        if (!count)
        {
            const auto& chunk = work.source->chunks[work.nextChunk + i];
            params.slotOffset = chunk.firstSlot;
            params.slotCount = chunk.slotCount;
            params.instanceCapacity = chunk.instanceCount;
        }
        cmdList->writeBuffer(renderDevice->GetNativeBuffer(cachedInstanceGenParamsCB), &params, sizeof(params));
        framegraph::BindingSetBuilder bindings(*reflection, device, "Detail.Generate");
        bindings.ConstantBuffer("InstanceGenParams", renderDevice->GetNativeBuffer(cachedInstanceGenParamsCB))
            .BufferSRV("g_slot_data", slotDataBuffer)
            .Texture("g_heightmap", heightmapTexture)
            .BufferSRV("g_detail_models", detailModelsBuffer);
        if (count)
            bindings.BufferUAV("g_per_slot_counts", work.counts);
        else
            bindings.BufferSRV("g_slot_aabbs", work.source->slots)
                .BufferSRV("g_emit_slot_ids", work.emitSlots)
                .BufferUAV("g_instances", work.source->chunks[work.nextChunk + i].buffer)
                .BufferUAV("g_emit_status", work.status)
                .BufferUAV("g_local_counters", work.localCounters);
        auto set = device->createBindingSet(bindings.Build(), count ? instanceCountBindingLayout : instanceGenBindingLayout);
        R_ASSERT2(set, "[DetailManager] source generation binding set unavailable");
        nvrhi::ComputeState state;
        state.pipeline = count ? instanceCountPipeline : instanceGenPipeline;
        state.bindings = { set };
        cmdList->setComputeState(state);
        cmdList->dispatch(std::min(params.slotCount, 65535u), params.slotCount / 65535u + u32(params.slotCount % 65535u != 0), 1);
        if (!count)
        {
            cmdList->setBufferState(work.source->chunks[work.nextChunk + i].buffer, nvrhi::ResourceStates::NonPixelShaderResource);
        }
    }
    if (count)
    {
        cmdList->copyBuffer(work.countReadback, 0, work.counts, 0, u64(slot_count) * 16);
        work.stage = GenerationWork::Stage::CountPending;
    }
    else
    {
        cmdList->copyBuffer(work.statusReadback, 0, work.status, 0, sizeof(u32));
        work.stage = GenerationWork::Stage::EmitPending;
    }
}

void FGDetailManager::ScheduleStatsReadback(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device, VisibilityFrame& frame)
{
    R_ASSERT(device && frame.lease);
    cmdList->copyBuffer(frame.readback, 0, frame.workStatus, 0, VISIBILITY_STATUS_BYTES);
    frame.statsRecorded = true;
}

void FGDetailManager::RecordVisibilityWork(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device, VisibilityFrame& frame, bool slots)
{
    auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("detail_work_args", slots ? ".cs:main_slots" : ".cs");
    R_ASSERT2(reflection, "[DetailManager] visibility work shader unavailable");
    framegraph::BindingSetBuilder bindings(*reflection, device, "Detail.Work");
    bindings.BufferSRV("g_VisibleSlotCount", frame.visibleSlotCount).BufferUAV("g_WorkStatus", frame.workStatus);
    if (slots)
        bindings.BufferUAV("g_SlotDispatch", frame.slotDispatch);
    else
    {
        const u32 params[8] = { passes::kVisIdEntryLimit - frame.entryBase,
            frame.visibleCapacity[0], frame.visibleCapacity[1], frame.visibleCapacity[2],
            frame.visibleCapacity[3], frame.visibleCapacity[4], 0, 0 };
        auto cb = framegraph::GetPassResourceCache().GetOrCreateVolatileCB("Detail", "DetailWorkParams", sizeof(params), GEnv.Render->GetRenderDevice());
        cmdList->writeBuffer(cb, params, sizeof(params));
        bindings.ConstantBuffer("DetailWorkParams", cb)
            .BufferUAV("g_ArgsLod0", frame.drawArgs[0])
            .BufferUAV("g_ArgsLod1", frame.drawArgs[1])
            .BufferUAV("g_ArgsLod2", frame.drawArgs[2])
            .BufferUAV("g_ArgsMesh", frame.drawArgs[3])
            .BufferUAV("g_ArgsDecal", frame.drawArgs[4])
            .BufferUAV("g_SwArgs", frame.swDispatch)
            .BufferUAV("g_DetailPackets", frame.packets);
    }
    auto set = framegraph::GetPassResourceCache().GetOrCreateBindingSet(bindings.Build(),
        slots ? slotArgsBindingLayout : visibilityArgsBindingLayout, device);
    R_ASSERT2(set, "[DetailManager] visibility work binding set unavailable");
    nvrhi::ComputeState state;
    state.pipeline = slots ? slotArgsPipeline : visibilityArgsPipeline;
    state.bindings = { set };
    cmdList->setComputeState(state);
    cmdList->dispatch(1, 1, 1);
}

void FGDetailManager::DestroyInstanceStorage()
{
    if (GEnv.Backend)
    {
        if (generationWork && generationWork->lease)
            GEnv.Backend->CloseSubmissionLease(generationWork->lease);
        for (const auto& frame : m_visibilityFrames)
            if (frame->lease)
                GEnv.Backend->CloseSubmissionLease(frame->lease);
        if (generationWork || !m_visibilityFrames.empty())
            GEnv.Backend->WaitForIdle();
        if (generationWork && generationWork->lease)
            GEnv.Backend->ReleaseSubmissionLease(generationWork->lease);
        for (const auto& frame : m_visibilityFrames)
        {
            if (frame->lease)
                GEnv.Backend->ReleaseSubmissionLease(frame->lease);
            frame->lease = 0;
        }
    }
    visibilityFrame.reset();
    m_completedVisibilityFrame.reset();
    m_visibilityFrames.clear();
    generationWork.reset();
    generatedInstances.reset();
    m_instanceGenerations.clear();
    m_cullSourceLayout = nullptr;
    cullingStats = {};
    instanceMemoryStats = {};
    totalGeneratedInstances = 0;
    m_instancesNeedRegeneration = true;
    m_detailsEnabled = false;
}

}
