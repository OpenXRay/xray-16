// xrRender/Geometry/GeometryBatch.cpp
#include "stdafx.h"
#include "GeometryBatch.h"

namespace xray::render {

GeometryCollector* g_geometryCollector = nullptr;

GeometryCollector::GeometryCollector() {
    m_batches.reserve(16384);
    Msg("* [GeometryCollector] Created");
}

GeometryCollector::~GeometryCollector() {
    Msg("* [GeometryCollector] Destroyed");
}

void GeometryCollector::BeginFrame() {
    m_batches.clear();
    if (m_batches.capacity() < 16384)
        m_batches.reserve(16384);
    m_stats = Stats{};
}

void GeometryCollector::EndFrame() {
    m_stats.numBatches = static_cast<u32>(m_batches.size());
}

void GeometryCollector::Submit(const GeometryBatch& batch) {
    VERIFY(batch.vertexBuffer != nullptr);
    VERIFY(batch.indexBuffer != nullptr);
    VERIFY(batch.indexCount > 0);

    m_batches.push_back(batch);
}

void GeometryCollector::SubmitStatic(const GeometryBatch& s) {
    VERIFY(s.vertexBuffer != nullptr);
    VERIFY(s.indexBuffer != nullptr);
    VERIFY(s.indexCount > 0);

    GeometryBatch& d = m_batches.emplace_back();
    d.vertexBuffer = s.vertexBuffer;
    d.indexBuffer = s.indexBuffer;
    d.indexCount = s.indexCount;
    d.startIndex = s.startIndex;
    d.baseVertex = s.baseVertex;
    d.vertexStride = s.vertexStride;
    d.materialID = s.materialID;
    d.bindlessMaterialID = s.bindlessMaterialID;
    d.albedoTexture = s.albedoTexture;
    d.normalTexture = s.normalTexture;
    d.materialTexture = s.materialTexture;
    d.worldMatrix = s.worldMatrix;
    d.worldBoundsCenter = s.worldBoundsCenter;
    d.worldBoundsRadius = s.worldBoundsRadius;
    d.pipeline = s.pipeline;
    d.bindingSet = s.bindingSet;
    d.materialPSO = s.materialPSO;
    d.renderPhase = s.renderPhase;
    d.visual = s.visual;
    d.renderable = s.renderable;
    d.isVisible = s.isVisible;
    d.isStatic = s.isStatic;
    d.isSkinned = s.isSkinned;
    d.skinningRenderMode = s.skinningRenderMode;
    d.skinnedPoolFormat = s.skinnedPoolFormat;
    d.skinnedPoolBaseVertex = s.skinnedPoolBaseVertex;
    d.skinnedPoolFirstIndex = s.skinnedPoolFirstIndex;
    d.isTerrain = s.isTerrain;
    d.terrainMaterialID = s.terrainMaterialID;
    d.cachedAlphaTest = s.cachedAlphaTest;
    d.cachedTransparent = s.cachedTransparent;
    d.sortFlagsValid = s.sortFlagsValid;
    d.ssa = s.ssa;
    d.megaBufferAlloc = s.megaBufferAlloc;
}

void GeometryCollector::Sort() {
    if (m_batches.empty())
        return;

    for (auto& batch : m_batches)
    {
        if (!batch.sortFlagsValid)
            batch.CacheSortFlags();
    }

    const u32 n = static_cast<u32>(m_batches.size());
    static thread_local xr_vector<u32> order;
    order.resize(n);
    for (u32 i = 0; i < n; ++i)
        order[i] = i;

    auto opaqueEnd = std::partition(order.begin(), order.end(),
        [&](u32 i) { return !m_batches[i].cachedTransparent && !m_batches[i].cachedAlphaTest; });
    auto arefEnd = std::partition(opaqueEnd, order.end(),
        [&](u32 i) { return !m_batches[i].cachedTransparent; });

    auto bySsaAsc = [&](u32 a, u32 b) { return m_batches[a].ssa < m_batches[b].ssa; };
    std::sort(arefEnd, order.end(), bySsaAsc);

    bool identity = true;
    for (u32 i = 0; i < n; ++i)
    {
        if (order[i] != i)
        {
            identity = false;
            break;
        }
    }
    if (identity)
        return;

    static thread_local xr_vector<GeometryBatch> sorted;
    sorted.clear();
    sorted.reserve(n);
    for (u32 i : order)
        sorted.push_back(std::move(m_batches[i]));
    m_batches.swap(sorted);
}

u64 GeometryCollector::ComputeSortKey(const GeometryBatch& batch) {
    u64 key = 0;

    if (batch.pipeline) {
        u64 pipelineHash = reinterpret_cast<u64>(batch.pipeline) >> 4;
        key |= (pipelineHash & 0xFFFF) << 48;
    }

    key |= (static_cast<u64>(batch.materialID) & 0xFFFF) << 32;
    key |= (batch.albedoTexture.index & 0xFFFF) << 16;
    key |= (batch.normalTexture.index & 0xFFFF);

    return key;
}

} // namespace xray::render
