#include "stdafx.h"
#include "GeometryBatch.h"

namespace xray::render {

GeometryCollector* g_geometryCollector = nullptr;

GeometryCollector::GeometryCollector() {
    m_batches.reserve(4096);
    Msg("* [GeometryCollector] Created");
}

GeometryCollector::~GeometryCollector() {
    Msg("* [GeometryCollector] Destroyed");
}

void GeometryCollector::BeginFrame() {
    m_batches.clear();

    m_stats = Stats{};
}

void GeometryCollector::EndFrame() {
    Sort();

    m_stats.numBatches = static_cast<u32>(m_batches.size());

    for (const auto& batch : m_batches) {
        m_stats.numTriangles += batch.indexCount / 3;
    }

    Msg("~ [GeometryCollector] Frame complete: %u batches, %u tris",
        m_stats.numBatches,
        m_stats.numTriangles);
}

void GeometryCollector::Submit(const GeometryBatch& batch) {
    VERIFY(batch.vertexBuffer.IsValid());
    VERIFY(batch.indexBuffer.IsValid());
    VERIFY(batch.indexCount > 0);
    VERIFY(batch.pipeline != nullptr);

    m_batches.push_back(batch);
}

void GeometryCollector::Sort() {

    std::sort(m_batches.begin(), m_batches.end(),
        [](const GeometryBatch& a, const GeometryBatch& b) {
            u64 keyA = ComputeSortKey(a);
            u64 keyB = ComputeSortKey(b);
            return keyA < keyB;
        });
}

u64 GeometryCollector::ComputeSortKey(const GeometryBatch& batch) {
    u64 key = 0;

    u64 pipelineHash = reinterpret_cast<u64>(batch.pipeline) >> 4;
    key |= (pipelineHash & 0xFFFF) << 48;

    key |= (static_cast<u64>(batch.materialID) & 0xFFFF) << 32;

    key |= (batch.albedoTexture.index & 0xFFFF) << 16;

    key |= (batch.normalTexture.index & 0xFFFF);

    return key;
}

}
