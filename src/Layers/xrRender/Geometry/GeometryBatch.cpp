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
}

void GeometryCollector::Submit(const GeometryBatch& batch) {
    VERIFY(batch.vertexBuffer != nullptr);
    VERIFY(batch.indexBuffer != nullptr);
    VERIFY(batch.indexCount > 0);

    m_batches.push_back(batch);
}

void GeometryCollector::Sort() {

    std::sort(m_batches.begin(), m_batches.end(),
        [](const GeometryBatch& a, const GeometryBatch& b) {
            bool aB2F = a.IsStrictB2F();
            bool bB2F = b.IsStrictB2F();
            bool aAref = a.IsAlphaTested();
            bool bAref = b.IsAlphaTested();

            if (aB2F != bB2F) {
                return !aB2F;
            }

            if (aB2F && bB2F) {
                return a.ssa < b.ssa;
            }

            if (aAref != bAref) {
                return !aAref;
            }

            return a.ssa > b.ssa;
        });
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

}
