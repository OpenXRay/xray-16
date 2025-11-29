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
    // Sort for optimal rendering
    Sort();

    // Update statistics
    m_stats.numBatches = static_cast<u32>(m_batches.size());

    for (const auto& batch : m_batches) {
        m_stats.numTriangles += batch.indexCount / 3;
    }
}

void GeometryCollector::Submit(const GeometryBatch& batch) {
    VERIFY(batch.vertexBuffer != nullptr);  // nvrhi::BufferHandle is a smart pointer
    VERIFY(batch.indexBuffer != nullptr);
    VERIFY(batch.indexCount > 0);
    // NOTE: pipeline can be nullptr during collection, will be set later from visual->shader

    m_batches.push_back(batch);
}

void GeometryCollector::Sort() {
    // ═══════════════════════════════════════════════════════
    //  RENDER ORDER SORTING (using SSA + shader flags)
    // ═══════════════════════════════════════════════════════
    // Proper render order for forward rendering:
    //   1. Opaque (iPriority == 0) - SSA descending (front-to-back for early-Z)
    //   2. Alpha-tested (iPriority == 1) - SSA descending (after opaque fills depth)
    //   3. Transparent (bStrictB2F) - SSA ascending (back-to-front for blending)
    //
    // SSA = R / distSQ (larger = closer/bigger = more visually important)
    // SSA descending = front-to-back, SSA ascending = back-to-front

    std::sort(m_batches.begin(), m_batches.end(),
        [](const GeometryBatch& a, const GeometryBatch& b) {
            // Get sorting properties using member functions
            bool aB2F = a.IsStrictB2F();
            bool bB2F = b.IsStrictB2F();
            bool aAref = a.IsAlphaTested();
            bool bAref = b.IsAlphaTested();

            // 1. Transparent (bStrictB2F) batches render LAST
            if (aB2F != bB2F) {
                return !aB2F;  // Non-B2F comes before B2F
            }

            // 2. For transparent batches: sort by SSA ascending (back-to-front)
            if (aB2F && bB2F) {
                return a.ssa < b.ssa;  // Ascending SSA = back-to-front
            }

            // 3. Alpha-tested renders AFTER opaque
            //    This ensures opaque geometry fills depth buffer first,
            //    so clip() in alpha-tested shaders shows correct background.
            if (aAref != bAref) {
                return !aAref;  // Non-aref (opaque) comes before aref
            }

            // 4. Within same category: sort by SSA descending (front-to-back)
            //    This matches vanilla's cmp_ssa: return lhs.ssa > rhs.ssa
            return a.ssa > b.ssa;  // Descending SSA = front-to-back
        });
}

u64 GeometryCollector::ComputeSortKey(const GeometryBatch& batch) {
    // Compute sort key (higher bits = more important)
    u64 key = 0;

    // Bits 48-63: Pipeline (most important - avoid PSO changes)
    if (batch.pipeline) {
        u64 pipelineHash = reinterpret_cast<u64>(batch.pipeline) >> 4;
        key |= (pipelineHash & 0xFFFF) << 48;
    }

    // Bits 32-47: Material ID
    key |= (static_cast<u64>(batch.materialID) & 0xFFFF) << 32;

    // Bits 16-31: Albedo texture
    key |= (batch.albedoTexture.index & 0xFFFF) << 16;

    // Bits 0-15: Normal texture
    key |= (batch.normalTexture.index & 0xFFFF);

    return key;
}

} // namespace xray::render
