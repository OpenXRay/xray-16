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
    m_stats.numBatches = static_cast<u32>(m_batches.size());
}

void GeometryCollector::Submit(const GeometryBatch& batch) {
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

}
