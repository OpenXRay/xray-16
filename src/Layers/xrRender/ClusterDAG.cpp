#include "stdafx.h"
#include "ClusterDAG.h"
#include "xrRender_console.h"

#include "../../../Externals/meshoptimizer/src/meshoptimizer.h"
#define CLUSTERLOD_IMPLEMENTATION
#include "clusterlod.h"

#include <atomic>
#include <thread>
#include <algorithm>

namespace xray::render::fg
{

namespace
{

constexpr u32 kClusterMaxTris = 128;
constexpr float kErrorCap = 1e30f;

struct BakeUnit {
    ClusterMeshKey key;
    bool valid;
};

struct BakeResult {
    xr_vector<ClusterMetaProto> protos;
    xr_vector<u32> indices;
    bool baked;
};

struct BakeContext {
    BakeResult* result;
    xr_vector<clodBounds> groups;
};

int OutputGroupCB(void* ctxv, clodGroup group, const clodCluster* clusters, size_t clusterCount)
{
    BakeContext* ctx = static_cast<BakeContext*>(ctxv);

    clodBounds g = group.simplified;
    if (!(g.error < kErrorCap))
        g.error = kErrorCap;

    const int groupId = int(ctx->groups.size());
    ctx->groups.push_back(g);

    BakeResult& out = *ctx->result;
    for (size_t i = 0; i < clusterCount; ++i) {
        const clodCluster& c = clusters[i];

        ClusterMetaProto p = {};
        p.sphere[0] = c.bounds.center[0];
        p.sphere[1] = c.bounds.center[1];
        p.sphere[2] = c.bounds.center[2];
        p.sphere[3] = c.bounds.radius;

        const clodBounds& parent = ctx->groups[groupId];
        p.lodParent[0] = parent.center[0];
        p.lodParent[1] = parent.center[1];
        p.lodParent[2] = parent.center[2];
        p.lodParent[3] = parent.radius;
        p.parentError = parent.error;

        if (c.refined >= 0) {
            const clodBounds& self = ctx->groups[c.refined];
            p.lodSelf[0] = self.center[0];
            p.lodSelf[1] = self.center[1];
            p.lodSelf[2] = self.center[2];
            p.lodSelf[3] = self.radius;
            p.selfError = self.error;
        } else {
            p.lodSelf[0] = c.bounds.center[0];
            p.lodSelf[1] = c.bounds.center[1];
            p.lodSelf[2] = c.bounds.center[2];
            p.lodSelf[3] = c.bounds.radius;
            p.selfError = 0.0f;
        }

        p.depth = u32(group.depth < 0 ? 0 : group.depth);
        p.ibFirst = u32(out.indices.size());
        p.indexCount = u32(c.index_count);
        p.flags = 0;

        out.indices.insert(out.indices.end(), c.indices, c.indices + c.index_count);
        out.protos.push_back(p);
    }

    return groupId;
}

bool ValidateUnit(const ClusterMeshKey& key, const u32* indices)
{
    if (key.indexCount < 3 || key.indexCount % 3 != 0 || key.vertexCount == 0)
        return false;
    for (u32 i = 0; i < key.indexCount; ++i) {
        if (indices[i] >= key.vertexCount)
            return false;
    }
    return true;
}

void BakeOneUnit(
    const ClusterMeshKey& key,
    const bindless::UnifiedVertex* megaVertices,
    const u32* megaIndices,
    BakeResult& result)
{
    result.baked = false;

    const bindless::UnifiedVertex* verts = megaVertices + key.vertexOffset;
    const u32* indices = megaIndices + key.indexOffset;

    if (!ValidateUnit(key, indices))
        return;

    xr_vector<float> attrs;
    attrs.resize(size_t(key.vertexCount) * 7);
    for (u32 v = 0; v < key.vertexCount; ++v) {
        const bindless::UnifiedVertex& src = verts[v];
        float* a = &attrs[size_t(v) * 7];
        a[0] = float((src.normal >> 16) & 0xFF) / 255.0f;
        a[1] = float((src.normal >> 8) & 0xFF) / 255.0f;
        a[2] = float(src.normal & 0xFF) / 255.0f;
        a[3] = src.texcoord0.x;
        a[4] = src.texcoord0.y;
        a[5] = src.texcoord1.x;
        a[6] = src.texcoord1.y;
    }

    static const float s_weights[3] = {0.5f, 0.5f, 0.5f};

    clodConfig cfg = clodDefaultConfig(kClusterMaxTris);
    cfg.optimize_bounds = true;
    cfg.simplify_fallback_sloppy = false;
    cfg.simplify_prune = true;

    clodMesh mesh = {};
    mesh.indices = indices;
    mesh.index_count = key.indexCount;
    mesh.vertex_count = key.vertexCount;
    mesh.vertex_positions = reinterpret_cast<const float*>(verts);
    mesh.vertex_positions_stride = sizeof(bindless::UnifiedVertex);
    mesh.vertex_attributes = attrs.data();
    mesh.vertex_attributes_stride = 7 * sizeof(float);
    mesh.attribute_weights = s_weights;
    mesh.attribute_count = 3;
    mesh.attribute_protect_mask = 0x78;

    BakeContext ctx;
    ctx.result = &result;
    ctx.groups.reserve(64);

    clodBuild(cfg, mesh, &ctx, &OutputGroupCB);
    result.baked = !result.protos.empty();
}

bool IsSelfLoop(const ClusterMetaProto& p)
{
    if (!(p.selfError > 0.0f) || !(p.parentError < kErrorCap))
        return false;
    return memcmp(p.lodSelf, p.lodParent, sizeof(p.lodSelf)) == 0 && p.selfError == p.parentError;
}

} // namespace

void ClusterDAG::Clear()
{
    m_records.clear();
    m_protos.clear();
    m_bakedIndices.clear();
    m_stats = {};
    m_megaIndexBase = 0;
}

const ClusterMeshRecord* ClusterDAG::FindRecord(const ClusterMeshKey& key) const
{
    auto it = std::lower_bound(m_records.begin(), m_records.end(), key,
        [](const ClusterMeshRecord& r, const ClusterMeshKey& k) { return r.key < k; });
    if (it == m_records.end() || !(it->key == key))
        return nullptr;
    return &*it;
}

void ClusterDAG::Bake(
    const xr_vector<ClusterMeshKey>& ranges,
    const bindless::UnifiedVertex* megaVertices,
    const u32* megaIndices)
{
    Clear();

    CTimer timer;
    timer.Start();

    const u32 minTris = u32(std::max(ps_r_cluster_tris, int(kClusterMaxTris)));

    xr_vector<BakeUnit> units;
    units.reserve(ranges.size());
    {
        xr_vector<ClusterMeshKey> sorted = ranges;
        std::sort(sorted.begin(), sorted.end());
        sorted.erase(std::unique(sorted.begin(), sorted.end(),
            [](const ClusterMeshKey& a, const ClusterMeshKey& b) { return a == b; }), sorted.end());

        for (const ClusterMeshKey& key : sorted) {
            if (key.indexCount < 3 * minTris)
                continue;
            BakeUnit unit;
            unit.key = key;
            unit.valid = true;
            units.push_back(unit);
        }
    }

    m_stats.eligibleMeshes = u32(units.size());
    if (units.empty()) {
        Msg("* [ClusterDAG] no eligible meshes (of %u ranges)", u32(ranges.size()));
        return;
    }

    std::sort(units.begin(), units.end(), [](const BakeUnit& a, const BakeUnit& b) {
        if (a.key.indexCount != b.key.indexCount)
            return a.key.indexCount > b.key.indexCount;
        return a.key < b.key;
    });

    xr_vector<BakeResult> results(units.size());

    const u32 hw = std::max(1u, std::thread::hardware_concurrency());
    const u32 threadCount = std::min(8u, std::min(hw, u32(units.size())));

    std::atomic<u32> cursor{0};
    auto worker = [&]() {
        for (;;) {
            const u32 i = cursor.fetch_add(1, std::memory_order_relaxed);
            if (i >= units.size())
                return;
            BakeOneUnit(units[i].key, megaVertices, megaIndices, results[i]);
        }
    };

    if (threadCount > 1) {
        xr_vector<std::thread> threads;
        threads.reserve(threadCount);
        for (u32 t = 0; t < threadCount; ++t)
            threads.emplace_back(worker);
        for (auto& t : threads)
            t.join();
    } else {
        worker();
    }

    for (size_t i = 0; i < units.size(); ++i) {
        BakeResult& r = results[i];
        if (!r.baked)
            continue;

        ClusterMeshRecord rec;
        rec.key = units[i].key;
        rec.firstProto = u32(m_protos.size());
        rec.firstIndex = u32(m_bakedIndices.size());

        u32 nonLoops = 0;
        for (const ClusterMetaProto& p : r.protos) {
            if (!IsSelfLoop(p))
                nonLoops++;
        }

        u32 kept = 0;
        for (const ClusterMetaProto& p : r.protos) {
            if (nonLoops > 0 && IsSelfLoop(p)) {
                m_stats.droppedSelfLoops++;
                continue;
            }
            m_protos.push_back(p);
            kept++;
        }

        rec.protoCount = kept;
        rec.indexTotal = u32(r.indices.size());
        m_bakedIndices.insert(m_bakedIndices.end(), r.indices.begin(), r.indices.end());

        if (kept == 0) {
            m_protos.resize(rec.firstProto);
            m_bakedIndices.resize(rec.firstIndex);
            continue;
        }

        m_records.push_back(rec);
        m_stats.bakedMeshes++;

        r.protos.clear();
        r.protos.shrink_to_fit();
        r.indices.clear();
        r.indices.shrink_to_fit();
    }

    std::sort(m_records.begin(), m_records.end(),
        [](const ClusterMeshRecord& a, const ClusterMeshRecord& b) { return a.key < b.key; });

    m_stats.clusters = u32(m_protos.size());
    m_stats.bakedIndexCount = m_bakedIndices.size();
    m_stats.bakeMs = u32(timer.GetElapsed_ms());

    RunDiagnostics();

    Msg("* [ClusterDAG] baked %u/%u meshes: %u clusters, %llu indices, %u self-loops dropped, %u holes, %u ms",
        m_stats.bakedMeshes, m_stats.eligibleMeshes, m_stats.clusters,
        (unsigned long long)m_stats.bakedIndexCount, m_stats.droppedSelfLoops, m_stats.holes, m_stats.bakeMs);
    Msg("* [ClusterDAG] parentError histogram: inf=%u >100=%u 10-100=%u 1-10=%u 0.1-1=%u <0.1=%u",
        m_stats.histInf, m_stats.hist100, m_stats.hist10, m_stats.hist1, m_stats.hist01, m_stats.histSmall);
    {
        string512 levels;
        levels[0] = 0;
        for (u32 d = 0; d <= m_stats.maxDepth && d < 16; ++d) {
            string64 one;
            xr_sprintf(one, "%sL%u=%u", d ? " " : "", d, m_stats.levelCounts[d]);
            xr_strcat(levels, one);
        }
        Msg("* [ClusterDAG] levels: %s", levels);
    }
}

void ClusterDAG::RunDiagnostics()
{
    xr_vector<ClusterBoundsKey> selfKeys;

    for (const ClusterMeshRecord& rec : m_records) {
        selfKeys.clear();
        selfKeys.reserve(rec.protoCount);

        for (u32 i = 0; i < rec.protoCount; ++i) {
            const ClusterMetaProto& p = m_protos[rec.firstProto + i];
            if (p.selfError > 0.0f) {
                ClusterBoundsKey k;
                k.x = p.lodSelf[0]; k.y = p.lodSelf[1]; k.z = p.lodSelf[2];
                k.r = p.lodSelf[3]; k.e = p.selfError;
                selfKeys.push_back(k);
            }

            const u32 depth = std::min(p.depth, 15u);
            m_stats.levelCounts[depth]++;
            m_stats.maxDepth = std::max(m_stats.maxDepth, p.depth);

            if (!(p.parentError < kErrorCap)) m_stats.histInf++;
            else if (p.parentError > 100.0f) m_stats.hist100++;
            else if (p.parentError > 10.0f) m_stats.hist10++;
            else if (p.parentError > 1.0f) m_stats.hist1++;
            else if (p.parentError > 0.1f) m_stats.hist01++;
            else m_stats.histSmall++;
        }

        std::sort(selfKeys.begin(), selfKeys.end());

        for (u32 i = 0; i < rec.protoCount; ++i) {
            const ClusterMetaProto& p = m_protos[rec.firstProto + i];
            if (!(p.parentError < kErrorCap))
                continue;
            ClusterBoundsKey k;
            k.x = p.lodParent[0]; k.y = p.lodParent[1]; k.z = p.lodParent[2];
            k.r = p.lodParent[3]; k.e = p.parentError;
            if (!std::binary_search(selfKeys.begin(), selfKeys.end(), k))
                m_stats.holes++;
        }
    }
}

} // namespace xray::render::fg
