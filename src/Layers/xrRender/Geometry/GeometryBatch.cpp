// xrRender/Geometry/GeometryBatch.cpp
#include "stdafx.h"
#include "GeometryBatch.h"

namespace xray::render {

// Global geometry collector instance (to be initialized by renderer)
GeometryCollector* g_geometryCollector = nullptr;

GeometryCollector::GeometryCollector() {
    m_batches.reserve(4096);  // Pre-allocate for typical scene
    Msg("* [GeometryCollector] Created");
}

GeometryCollector::~GeometryCollector() {
    Msg("* [GeometryCollector] Destroyed");
}

void GeometryCollector::BeginFrame() {
    // Clear previous frame's batches
    m_batches.clear();

    // Reset statistics
    m_stats = Stats{};
}

void GeometryCollector::EndFrame() {
    // Update statistics
    m_stats.numBatches = static_cast<u32>(m_batches.size() + m_staticBatches.size());
}

void GeometryCollector::Submit(const GeometryBatch& batch) {
    VERIFY(batch.indexCount > 0);
    R_ASSERT2(!(batch.isStatic && batch.isSkinned), "Static skinned geometry is not supported");

    if (batch.isStatic)
        m_staticBatches.push_back(batch);
    else
        m_batches.push_back(batch);
}

void GeometryCollector::EndStaticBuild() {
    m_staticTransparentIndices.clear();

    for (u32 i = 0; i < m_staticBatches.size(); ++i) {
        const GeometryBatch& batch = m_staticBatches[i];
        if (!batch.isSkinned && !batch.isTerrain && batch.IsStrictB2F())
            m_staticTransparentIndices.push_back(i);
    }
    m_staticBuildComplete = true;
}

void GeometryCollector::ClearStatic() {
    m_staticBatches.clear();
    m_staticTransparentIndices.clear();
    m_staticBuildComplete = false;
}

} // namespace xray::render
