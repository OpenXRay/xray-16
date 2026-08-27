#include "stdafx.h"
#include "ClusterDAG.h"
#include "xrRender_console.h"

#include "../../../Externals/meshoptimizer/src/meshoptimizer.h"
#define CLUSTERLOD_IMPLEMENTATION
#include "clusterlod.h"

#include <atomic>
#include <thread>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cstdio>

namespace xray::render::fg
{

namespace
{

constexpr u32 kClusterMaxTris = 128;
constexpr float kErrorCap = 1e30f;
constexpr float kDilate = 0.5f;
constexpr u32 kMaxComponentTris = 200000;
constexpr float kMaxComponentExtent = 48.0f;
constexpr float kGapCut = 0.75f;

struct RangeInfo {
    ClusterMeshKey key;
    u32 flags;
    Fvector aabbMin;
    Fvector aabbMax;
    Fvector centroid;
};

struct BakeUnitDesc {
    xr_vector<u32> members;
    u32 flags;
    bool isComponent;
    u64 totalIndices;
};

struct BakeResult {
    xr_vector<ClusterMetaProto> protos;
    xr_vector<u32> indices;
    bool baked;
};

struct BakeContext {
    BakeResult* result;
    xr_vector<clodBounds> groups;
    const u32* absoluteOfLocal;
    const u32* memberOfLocal;
    u32 memberCount;
    u32 protoFlags;
};

u64 HashPosition(const Fvector& p)
{
    const u8* bytes = reinterpret_cast<const u8*>(&p);
    u64 h = 14695981039346656037ull;
    for (u32 i = 0; i < 12; ++i) {
        h ^= bytes[i];
        h *= 1099511628211ull;
    }
    return h;
}

void EmitProto(BakeResult& out, const BakeContext& ctx, const clodCluster& c,
    const clodBounds& parent, int depth, u32 member, const u32* indices, u32 indexCount)
{
    ClusterMetaProto p = {};
    p.sphere[0] = c.bounds.center[0];
    p.sphere[1] = c.bounds.center[1];
    p.sphere[2] = c.bounds.center[2];
    p.sphere[3] = c.bounds.radius;

    p.lodParent[0] = parent.center[0];
    p.lodParent[1] = parent.center[1];
    p.lodParent[2] = parent.center[2];
    p.lodParent[3] = parent.radius;
    p.parentError = parent.error;

    if (c.refined >= 0) {
        const clodBounds& self = ctx.groups[c.refined];
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

    p.depth = u32(depth < 0 ? 0 : depth);
    p.ibFirst = u32(out.indices.size());
    p.indexCount = indexCount;
    p.flags = ctx.protoFlags;
    p.member = member;

    out.indices.insert(out.indices.end(), indices, indices + indexCount);
    out.protos.push_back(p);
}

int OutputGroupCB(void* ctxv, clodGroup group, const clodCluster* clusters, size_t clusterCount)
{
    BakeContext* ctx = static_cast<BakeContext*>(ctxv);

    clodBounds g = group.simplified;
    if (!(g.error < kErrorCap))
        g.error = kErrorCap;

    const int groupId = int(ctx->groups.size());
    ctx->groups.push_back(g);
    const clodBounds& parent = ctx->groups[groupId];

    BakeResult& out = *ctx->result;

    for (size_t i = 0; i < clusterCount; ++i) {
        const clodCluster& c = clusters[i];

        if (!ctx->absoluteOfLocal) {
            EmitProto(out, *ctx, c, parent, group.depth, 0, c.indices, u32(c.index_count));
            continue;
        }

        static thread_local xr_vector<u32> s_translated;
        static thread_local xr_vector<u32> s_triMember;
        s_translated.resize(c.index_count);
        s_triMember.resize(c.index_count / 3);

        for (size_t k = 0; k < c.index_count; ++k)
            s_translated[k] = ctx->absoluteOfLocal[c.indices[k]];
        for (size_t t = 0; t < c.index_count / 3; ++t)
            s_triMember[t] = ctx->memberOfLocal[c.indices[t * 3]];

        for (u32 m = 0; m < ctx->memberCount; ++m) {
            static thread_local xr_vector<u32> s_memberIndices;
            s_memberIndices.clear();
            for (size_t t = 0; t < c.index_count / 3; ++t) {
                if (s_triMember[t] != m)
                    continue;
                s_memberIndices.push_back(s_translated[t * 3 + 0]);
                s_memberIndices.push_back(s_translated[t * 3 + 1]);
                s_memberIndices.push_back(s_translated[t * 3 + 2]);
            }
            if (s_memberIndices.empty())
                continue;
            EmitProto(out, *ctx, c, parent, group.depth, m,
                s_memberIndices.data(), u32(s_memberIndices.size()));
        }
    }

    return groupId;
}

bool ValidateRange(const ClusterMeshKey& key, const u32* indices)
{
    if (key.indexCount < 3 || key.indexCount % 3 != 0 || key.vertexCount == 0)
        return false;
    for (u32 i = 0; i < key.indexCount; ++i) {
        if (indices[i] >= key.vertexCount)
            return false;
    }
    return true;
}

void FillAttributes(const bindless::UnifiedVertex* verts, u32 count, float* attrs)
{
    for (u32 v = 0; v < count; ++v) {
        const bindless::UnifiedVertex& src = verts[v];
        float* a = attrs + size_t(v) * 7;
        a[0] = float((src.normal >> 16) & 0xFF) / 255.0f;
        a[1] = float((src.normal >> 8) & 0xFF) / 255.0f;
        a[2] = float(src.normal & 0xFF) / 255.0f;
        a[3] = src.texcoord0.x;
        a[4] = src.texcoord0.y;
        a[5] = src.texcoord1.x;
        a[6] = src.texcoord1.y;
    }
}

clodConfig MakeConfig()
{
    clodConfig cfg = clodDefaultConfig(kClusterMaxTris);
    cfg.optimize_bounds = true;
    cfg.simplify_fallback_sloppy = false;
    cfg.simplify_prune = true;
    return cfg;
}

void BakeSingleMesh(
    const ClusterMeshKey& key,
    u32 rangeFlags,
    const bindless::UnifiedVertex* megaVertices,
    const u32* megaIndices,
    BakeResult& result)
{
    result.baked = false;

    const bindless::UnifiedVertex* verts = megaVertices + key.vertexOffset;
    const u32* indices = megaIndices + key.indexOffset;

    xr_vector<float> attrs;
    attrs.resize(size_t(key.vertexCount) * 7);
    FillAttributes(verts, key.vertexCount, attrs.data());

    static const float s_weights[3] = {0.5f, 0.5f, 0.5f};

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

    BakeContext ctx = {};
    ctx.result = &result;
    ctx.groups.reserve(64);
    ctx.protoFlags = (rangeFlags & CLUSTER_RANGE_FLAG_AT) ? CLUSTER_PROTO_FLAG_AT : 0;

    clodBuild(MakeConfig(), mesh, &ctx, &OutputGroupCB);
    result.baked = !result.protos.empty();
}

void BakeComponent(
    const xr_vector<RangeInfo>& ranges,
    const xr_vector<u32>& members,
    const bindless::UnifiedVertex* megaVertices,
    const u32* megaIndices,
    const std::unordered_set<u64>& crossUnitPins,
    BakeResult& result,
    std::atomic<u32>& pinnedCounter)
{
    result.baked = false;

    u32 totalVerts = 0;
    u32 totalIndices = 0;
    for (u32 m : members) {
        totalVerts += ranges[m].key.vertexCount;
        totalIndices += ranges[m].key.indexCount;
    }

    xr_vector<bindless::UnifiedVertex> verts;
    xr_vector<u32> indices;
    xr_vector<u32> absoluteOfLocal;
    xr_vector<u32> memberOfLocal;
    xr_vector<unsigned char> locks;
    verts.reserve(totalVerts);
    indices.reserve(totalIndices);
    absoluteOfLocal.reserve(totalVerts);
    memberOfLocal.reserve(totalVerts);
    locks.resize(totalVerts, 0);

    u32 pinned = 0;

    for (u32 mi = 0; mi < u32(members.size()); ++mi) {
        const ClusterMeshKey& key = ranges[members[mi]].key;
        const bindless::UnifiedVertex* src = megaVertices + key.vertexOffset;
        const u32* srcIdx = megaIndices + key.indexOffset;
        const u32 base = u32(verts.size());

        verts.insert(verts.end(), src, src + key.vertexCount);
        for (u32 v = 0; v < key.vertexCount; ++v) {
            absoluteOfLocal.push_back(key.vertexOffset + v);
            memberOfLocal.push_back(mi);
        }
        for (u32 i = 0; i < key.indexCount; ++i)
            indices.push_back(base + srcIdx[i]);

        std::unordered_map<u64, u32> firstAtPos;
        firstAtPos.reserve(key.vertexCount);
        for (u32 v = 0; v < key.vertexCount; ++v) {
            const bindless::UnifiedVertex& vert = src[v];
            const u64 h = HashPosition(vert.position);

            auto it = firstAtPos.find(h);
            if (it == firstAtPos.end()) {
                firstAtPos.emplace(h, v);
            } else {
                const bindless::UnifiedVertex& canon = src[it->second];
                if (vert.texcoord0.x != canon.texcoord0.x || vert.texcoord0.y != canon.texcoord0.y ||
                    vert.texcoord1.x != canon.texcoord1.x || vert.texcoord1.y != canon.texcoord1.y)
                    locks[base + v] |= meshopt_SimplifyVertex_Protect;
            }

            if (crossUnitPins.count(h)) {
                locks[base + v] |= meshopt_SimplifyVertex_Lock;
                pinned++;
            }
        }
    }

    pinnedCounter.fetch_add(pinned, std::memory_order_relaxed);

    xr_vector<float> attrs;
    attrs.resize(size_t(verts.size()) * 7);
    FillAttributes(verts.data(), u32(verts.size()), attrs.data());

    static const float s_weights[3] = {0.5f, 0.5f, 0.5f};

    clodMesh mesh = {};
    mesh.indices = indices.data();
    mesh.index_count = indices.size();
    mesh.vertex_count = verts.size();
    mesh.vertex_positions = reinterpret_cast<const float*>(verts.data());
    mesh.vertex_positions_stride = sizeof(bindless::UnifiedVertex);
    mesh.vertex_attributes = attrs.data();
    mesh.vertex_attributes_stride = 7 * sizeof(float);
    mesh.attribute_weights = s_weights;
    mesh.attribute_count = 3;
    mesh.attribute_protect_mask = 0;
    mesh.vertex_lock = locks.data();

    BakeContext ctx = {};
    ctx.result = &result;
    ctx.groups.reserve(256);
    ctx.absoluteOfLocal = absoluteOfLocal.data();
    ctx.memberOfLocal = memberOfLocal.data();
    ctx.memberCount = u32(members.size());
    ctx.protoFlags = 0;

    clodBuild(MakeConfig(), mesh, &ctx, &OutputGroupCB);
    result.baked = !result.protos.empty();
}

bool IsSelfLoop(const ClusterMetaProto& p)
{
    if (!(p.selfError > 0.0f) || !(p.parentError < kErrorCap))
        return false;
    return memcmp(p.lodSelf, p.lodParent, sizeof(p.lodSelf)) == 0 && p.selfError == p.parentError;
}

constexpr u32 kCacheMagic = 0x464C4356;
constexpr u32 kCacheVersion = 1;

#pragma pack(push, 4)
struct CacheHeader {
    u32 magic;
    u32 version;
    u64 paramsHash;
    u64 geomStamp;
    u64 rangeSetHash;
    u32 recordCount;
    u32 memberCount;
    u32 protoCount;
    u32 pad;
    u64 indexCount;
    u32 reserved[4];
};
#pragma pack(pop)

u64 Fnv1a64(const void* data, size_t len, u64 h = 14695981039346656037ull)
{
    const u8* bytes = static_cast<const u8*>(data);
    for (size_t i = 0; i < len; ++i) {
        h ^= bytes[i];
        h *= 1099511628211ull;
    }
    return h;
}

u64 ComputeParamsHash()
{
    const u32 words[4] = {kCacheVersion, kClusterMaxTris,
        u32(std::max(ps_r_cluster_tris, int(kClusterMaxTris))), u32(ps_r_cluster_merge != 0)};
    return Fnv1a64(words, sizeof(words));
}

u64 ComputeRangeSetHash(const xr_vector<ClusterBakeRange>& ranges)
{
    xr_vector<ClusterBakeRange> sorted = ranges;
    std::sort(sorted.begin(), sorted.end(),
        [](const ClusterBakeRange& a, const ClusterBakeRange& b) { return a.key < b.key; });
    sorted.erase(std::unique(sorted.begin(), sorted.end(),
        [](const ClusterBakeRange& a, const ClusterBakeRange& b) { return a.key == b.key; }),
        sorted.end());

    u64 h = 14695981039346656037ull;
    for (const ClusterBakeRange& r : sorted) {
        h = Fnv1a64(&r.key, sizeof(r.key), h);
        h = Fnv1a64(&r.flags, sizeof(r.flags), h);
    }
    return h;
}

struct UnionFind {
    xr_vector<u32> parent;

    void Init(u32 n) {
        parent.resize(n);
        for (u32 i = 0; i < n; ++i)
            parent[i] = i;
    }
    u32 Find(u32 x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]];
            x = parent[x];
        }
        return x;
    }
    void Union(u32 a, u32 b) {
        a = Find(a);
        b = Find(b);
        if (a != b)
            parent[std::max(a, b)] = std::min(a, b);
    }
};

void ComputeAABB(const ClusterMeshKey& key, const bindless::UnifiedVertex* megaVertices,
    Fvector& outMin, Fvector& outMax)
{
    const bindless::UnifiedVertex* verts = megaVertices + key.vertexOffset;
    outMin.set(flt_max, flt_max, flt_max);
    outMax.set(-flt_max, -flt_max, -flt_max);
    for (u32 v = 0; v < key.vertexCount; ++v) {
        outMin.min(verts[v].position);
        outMax.max(verts[v].position);
    }
}

void UnitAABB(const xr_vector<RangeInfo>& ranges, const xr_vector<u32>& members,
    Fvector& outMin, Fvector& outMax)
{
    outMin.set(flt_max, flt_max, flt_max);
    outMax.set(-flt_max, -flt_max, -flt_max);
    for (u32 m : members) {
        outMin.min(ranges[m].aabbMin);
        outMax.max(ranges[m].aabbMax);
    }
}

u64 UnitTris(const xr_vector<RangeInfo>& ranges, const xr_vector<u32>& members)
{
    u64 t = 0;
    for (u32 m : members)
        t += ranges[m].key.indexCount / 3;
    return t;
}

void SplitComponentRec(const xr_vector<RangeInfo>& ranges, xr_vector<u32> members,
    xr_vector<xr_vector<u32>>& out, u32& splitCounter)
{
    Fvector mn, mx;
    UnitAABB(ranges, members, mn, mx);
    const float extent = std::max(mx.x - mn.x, std::max(mx.y - mn.y, mx.z - mn.z));
    const u64 tris = UnitTris(ranges, members);

    if (members.size() <= 1 || (tris <= kMaxComponentTris && extent <= kMaxComponentExtent)) {
        out.push_back(std::move(members));
        return;
    }

    splitCounter++;

    int axis = 0;
    if (mx.y - mn.y > mx.x - mn.x) axis = 1;
    if (mx.z - mn.z > (axis ? mx.y - mn.y : mx.x - mn.x)) axis = 2;

    std::sort(members.begin(), members.end(), [&](u32 a, u32 b) {
        return ranges[a].centroid[axis] < ranges[b].centroid[axis];
    });

    size_t cut = members.size() / 2;
    float bestGap = kGapCut;
    for (size_t i = 1; i < members.size(); ++i) {
        const float gap = ranges[members[i]].centroid[axis] - ranges[members[i - 1]].centroid[axis];
        if (gap > bestGap) {
            bestGap = gap;
            cut = i;
        }
    }
    if (cut == 0 || cut == members.size())
        cut = members.size() / 2;

    xr_vector<u32> left(members.begin(), members.begin() + cut);
    xr_vector<u32> right(members.begin() + cut, members.end());
    SplitComponentRec(ranges, std::move(left), out, splitCounter);
    SplitComponentRec(ranges, std::move(right), out, splitCounter);
}

} // namespace

void ClusterDAG::Clear()
{
    m_records.clear();
    m_memberKeys.clear();
    m_lookup.clear();
    m_protos.clear();
    m_bakedIndices.clear();
    m_stats = {};
    m_megaIndexBase = 0;
}

const ClusterUnitRecord* ClusterDAG::FindRecord(const ClusterMeshKey& key, u32& outMember) const
{
    auto it = std::lower_bound(m_lookup.begin(), m_lookup.end(), key,
        [](const LookupEntry& e, const ClusterMeshKey& k) { return e.key < k; });
    if (it == m_lookup.end() || !(it->key == key))
        return nullptr;
    outMember = it->member;
    return &m_records[it->record];
}

void ClusterDAG::BuildLookup()
{
    m_lookup.clear();
    m_lookup.reserve(m_memberKeys.size());
    for (u32 r = 0; r < u32(m_records.size()); ++r) {
        const ClusterUnitRecord& rec = m_records[r];
        for (u32 m = 0; m < rec.memberCount; ++m) {
            LookupEntry e;
            e.key = m_memberKeys[rec.firstMember + m];
            e.record = r;
            e.member = m;
            m_lookup.push_back(e);
        }
    }
    std::sort(m_lookup.begin(), m_lookup.end(),
        [](const LookupEntry& a, const LookupEntry& b) { return a.key < b.key; });
}

void ClusterDAG::Bake(
    const xr_vector<ClusterBakeRange>& ranges,
    const bindless::UnifiedVertex* megaVertices,
    const u32* megaIndices)
{
    Clear();

    CTimer timer;
    timer.Start();

    const u32 minTris = u32(std::max(ps_r_cluster_tris, int(kClusterMaxTris)));
    const bool mergeEnabled = ps_r_cluster_merge != 0;

    xr_vector<RangeInfo> infos;
    infos.reserve(ranges.size());
    {
        xr_vector<ClusterBakeRange> sorted = ranges;
        std::sort(sorted.begin(), sorted.end(),
            [](const ClusterBakeRange& a, const ClusterBakeRange& b) { return a.key < b.key; });
        sorted.erase(std::unique(sorted.begin(), sorted.end(),
            [](const ClusterBakeRange& a, const ClusterBakeRange& b) { return a.key == b.key; }),
            sorted.end());

        for (const ClusterBakeRange& r : sorted) {
            if (!ValidateRange(r.key, megaIndices + r.key.indexOffset))
                continue;
            RangeInfo info;
            info.key = r.key;
            info.flags = r.flags;
            if (!mergeEnabled)
                info.flags &= ~CLUSTER_RANGE_FLAG_MERGEABLE;
            infos.push_back(info);
        }
    }

    m_stats.eligibleMeshes = u32(infos.size());
    if (infos.empty()) {
        Msg("* [ClusterDAG] no eligible meshes (of %u ranges)", u32(ranges.size()));
        return;
    }

    xr_vector<u32> mergeSet;
    for (u32 i = 0; i < u32(infos.size()); ++i) {
        if (infos[i].flags & CLUSTER_RANGE_FLAG_MERGEABLE) {
            ComputeAABB(infos[i].key, megaVertices, infos[i].aabbMin, infos[i].aabbMax);
            infos[i].centroid.add(infos[i].aabbMin, infos[i].aabbMax);
            infos[i].centroid.mul(0.5f);
            mergeSet.push_back(i);
        }
    }

    xr_vector<xr_vector<u32>> componentMembers;
    xr_vector<u32> orphans;

    if (!mergeSet.empty()) {
        std::sort(mergeSet.begin(), mergeSet.end(), [&](u32 a, u32 b) {
            if (infos[a].aabbMin.x != infos[b].aabbMin.x)
                return infos[a].aabbMin.x < infos[b].aabbMin.x;
            return infos[a].key < infos[b].key;
        });

        UnionFind uf;
        uf.Init(u32(mergeSet.size()));

        for (u32 i = 0; i < u32(mergeSet.size()); ++i) {
            const RangeInfo& a = infos[mergeSet[i]];
            for (u32 j = i + 1; j < u32(mergeSet.size()); ++j) {
                const RangeInfo& b = infos[mergeSet[j]];
                if (b.aabbMin.x > a.aabbMax.x + kDilate)
                    break;
                if (a.aabbMin.y > b.aabbMax.y + kDilate || b.aabbMin.y > a.aabbMax.y + kDilate)
                    continue;
                if (a.aabbMin.z > b.aabbMax.z + kDilate || b.aabbMin.z > a.aabbMax.z + kDilate)
                    continue;
                uf.Union(i, j);
            }
        }

        std::unordered_map<u32, u32> rootToComponent;
        xr_vector<xr_vector<u32>> rawComponents;
        for (u32 i = 0; i < u32(mergeSet.size()); ++i) {
            const u32 root = uf.Find(i);
            auto it = rootToComponent.find(root);
            if (it == rootToComponent.end()) {
                it = rootToComponent.emplace(root, u32(rawComponents.size())).first;
                rawComponents.emplace_back();
            }
            rawComponents[it->second].push_back(mergeSet[i]);
        }

        for (xr_vector<u32>& comp : rawComponents) {
            if (comp.size() == 1) {
                const u32 idx = comp[0];
                if (infos[idx].key.indexCount >= 3 * minTris)
                    infos[idx].flags &= ~CLUSTER_RANGE_FLAG_MERGEABLE;
                else
                    orphans.push_back(idx);
                continue;
            }
            SplitComponentRec(infos, std::move(comp), componentMembers, m_stats.capSplits);
        }
    }

    for (u32 orphan : orphans) {
        const RangeInfo& o = infos[orphan];
        float bestGrowth = flt_max;
        s32 bestHost = -1;
        for (u32 c = 0; c < u32(componentMembers.size()); ++c) {
            if (UnitTris(infos, componentMembers[c]) + o.key.indexCount / 3 > kMaxComponentTris)
                continue;
            Fvector mn, mx;
            UnitAABB(infos, componentMembers[c], mn, mx);
            Fvector nmn = mn, nmx = mx;
            nmn.min(o.aabbMin);
            nmx.max(o.aabbMax);
            const float newExtent = std::max(nmx.x - nmn.x, std::max(nmx.y - nmn.y, nmx.z - nmn.z));
            if (newExtent > kMaxComponentExtent)
                continue;
            const float oldVol = (mx.x - mn.x) * (mx.y - mn.y) * (mx.z - mn.z);
            const float newVol = (nmx.x - nmn.x) * (nmx.y - nmn.y) * (nmx.z - nmn.z);
            const float growth = newVol - oldVol;
            if (growth < bestGrowth) {
                bestGrowth = growth;
                bestHost = s32(c);
            }
        }
        if (bestHost >= 0) {
            componentMembers[bestHost].push_back(orphan);
            m_stats.orphansAttached++;
        }
    }

    for (xr_vector<u32>& comp : componentMembers)
        std::sort(comp.begin(), comp.end(),
            [&](u32 a, u32 b) { return infos[a].key < infos[b].key; });

    std::unordered_set<u64> crossUnitPins;
    if (componentMembers.size() > 1) {
        std::unordered_map<u64, u32> posOwner;
        for (u32 c = 0; c < u32(componentMembers.size()); ++c) {
            for (u32 m : componentMembers[c]) {
                const ClusterMeshKey& key = infos[m].key;
                const bindless::UnifiedVertex* verts = megaVertices + key.vertexOffset;
                for (u32 v = 0; v < key.vertexCount; ++v) {
                    const u64 h = HashPosition(verts[v].position);
                    auto it = posOwner.find(h);
                    if (it == posOwner.end())
                        posOwner.emplace(h, c);
                    else if (it->second != c && it->second != 0xFFFFFFFFu) {
                        crossUnitPins.insert(h);
                        it->second = 0xFFFFFFFFu;
                    }
                }
            }
        }
    }

    xr_vector<BakeUnitDesc> units;
    units.reserve(componentMembers.size() + infos.size());

    for (xr_vector<u32>& comp : componentMembers) {
        BakeUnitDesc unit;
        unit.totalIndices = 0;
        for (u32 m : comp)
            unit.totalIndices += infos[m].key.indexCount;
        unit.members = std::move(comp);
        unit.flags = 0;
        unit.isComponent = true;
        m_stats.componentMembers += u32(unit.members.size());
        units.push_back(std::move(unit));
    }
    m_stats.components = u32(units.size());

    for (u32 i = 0; i < u32(infos.size()); ++i) {
        if (infos[i].flags & CLUSTER_RANGE_FLAG_MERGEABLE)
            continue;
        if (infos[i].key.indexCount < 3 * minTris)
            continue;
        BakeUnitDesc unit;
        unit.members.push_back(i);
        unit.flags = infos[i].flags;
        unit.isComponent = false;
        unit.totalIndices = infos[i].key.indexCount;
        units.push_back(std::move(unit));
    }

    if (units.empty()) {
        Msg("* [ClusterDAG] no units after partition (%u meshes)", m_stats.eligibleMeshes);
        return;
    }

    std::sort(units.begin(), units.end(), [&](const BakeUnitDesc& a, const BakeUnitDesc& b) {
        if (a.totalIndices != b.totalIndices)
            return a.totalIndices > b.totalIndices;
        return infos[a.members[0]].key < infos[b.members[0]].key;
    });

    xr_vector<BakeResult> results(units.size());
    std::atomic<u32> pinnedCounter{0};

    const u32 hw = std::max(1u, std::thread::hardware_concurrency());
    const u32 threadCount = std::min(8u, std::min(hw, u32(units.size())));

    std::atomic<u32> cursor{0};
    auto worker = [&]() {
        for (;;) {
            const u32 i = cursor.fetch_add(1, std::memory_order_relaxed);
            if (i >= units.size())
                return;
            const BakeUnitDesc& unit = units[i];
            if (unit.isComponent)
                BakeComponent(infos, unit.members, megaVertices, megaIndices,
                    crossUnitPins, results[i], pinnedCounter);
            else
                BakeSingleMesh(infos[unit.members[0]].key, unit.flags,
                    megaVertices, megaIndices, results[i]);
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

    m_stats.pinnedVerts = pinnedCounter.load();

    for (size_t i = 0; i < units.size(); ++i) {
        BakeResult& r = results[i];
        if (!r.baked)
            continue;

        ClusterUnitRecord rec;
        rec.firstMember = u32(m_memberKeys.size());
        rec.memberCount = u32(units[i].members.size());
        rec.firstProto = u32(m_protos.size());
        rec.firstIndex = u32(m_bakedIndices.size());
        rec.isComponent = units[i].isComponent ? 1 : 0;

        for (u32 m : units[i].members)
            m_memberKeys.push_back(infos[m].key);

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
            m_memberKeys.resize(rec.firstMember);
            continue;
        }

        m_records.push_back(rec);
        m_stats.bakedMeshes += rec.memberCount;

        r.protos.clear();
        r.protos.shrink_to_fit();
        r.indices.clear();
        r.indices.shrink_to_fit();
    }

    BuildLookup();

    m_stats.clusters = u32(m_protos.size());
    m_stats.bakedIndexCount = m_bakedIndices.size();
    m_stats.bakeMs = u32(timer.GetElapsed_ms());

    RunDiagnostics();

    Msg("* [ClusterDAG] baked %u/%u meshes: %u clusters, %llu indices, %u ms",
        m_stats.bakedMeshes, m_stats.eligibleMeshes, m_stats.clusters,
        (unsigned long long)m_stats.bakedIndexCount, m_stats.bakeMs);
    Msg("* [ClusterDAG] %u components (%u members, %u splits, %u orphans attached, %u pinned verts), %u self-loops dropped, %u holes",
        m_stats.components, m_stats.componentMembers, m_stats.capSplits,
        m_stats.orphansAttached, m_stats.pinnedVerts, m_stats.droppedSelfLoops, m_stats.holes);
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

bool ClusterDAG::TryLoadCache(const char* path, u64 geomStamp, const xr_vector<ClusterBakeRange>& ranges)
{
    Clear();

    if (!path || !path[0])
        return false;

    CTimer timer;
    timer.Start();

    FILE* f = fopen(path, "rb");
    if (!f)
        return false;

    bool ok = false;
    CacheHeader hdr = {};

    do {
        if (fread(&hdr, sizeof(hdr), 1, f) != 1)
            break;
        if (hdr.magic != kCacheMagic || hdr.version != kCacheVersion)
            break;
        if (hdr.paramsHash != ComputeParamsHash())
            break;
        if (hdr.geomStamp != geomStamp)
            break;
        if (hdr.rangeSetHash != ComputeRangeSetHash(ranges))
            break;
        if (hdr.recordCount == 0 || hdr.protoCount == 0 || hdr.indexCount == 0)
            break;

        m_records.resize(hdr.recordCount);
        m_memberKeys.resize(hdr.memberCount);
        m_protos.resize(hdr.protoCount);
        m_bakedIndices.resize(size_t(hdr.indexCount));

        if (fread(m_records.data(), sizeof(ClusterUnitRecord), hdr.recordCount, f) != hdr.recordCount)
            break;
        if (fread(m_memberKeys.data(), sizeof(ClusterMeshKey), hdr.memberCount, f) != hdr.memberCount)
            break;
        if (fread(m_protos.data(), sizeof(ClusterMetaProto), hdr.protoCount, f) != hdr.protoCount)
            break;
        if (fread(m_bakedIndices.data(), sizeof(u32), size_t(hdr.indexCount), f) != size_t(hdr.indexCount))
            break;

        ok = true;
        for (const ClusterUnitRecord& rec : m_records) {
            if (u64(rec.firstMember) + rec.memberCount > hdr.memberCount ||
                u64(rec.firstProto) + rec.protoCount > hdr.protoCount ||
                u64(rec.firstIndex) + rec.indexTotal > hdr.indexCount) {
                ok = false;
                break;
            }
        }
    } while (false);

    fclose(f);

    if (!ok) {
        Clear();
        return false;
    }

    BuildLookup();

    for (const ClusterUnitRecord& rec : m_records) {
        m_stats.bakedMeshes += rec.memberCount;
        if (rec.isComponent) {
            m_stats.components++;
            m_stats.componentMembers += rec.memberCount;
        }
    }
    m_stats.eligibleMeshes = m_stats.bakedMeshes;
    m_stats.clusters = u32(m_protos.size());
    m_stats.bakedIndexCount = m_bakedIndices.size();

    RunDiagnostics();

    m_stats.bakeMs = u32(timer.GetElapsed_ms());

    Msg("* [ClusterDAG] cache hit: %u meshes, %u clusters, %llu indices, %u components, %u holes, %u ms",
        m_stats.bakedMeshes, m_stats.clusters, (unsigned long long)m_stats.bakedIndexCount,
        m_stats.components, m_stats.holes, m_stats.bakeMs);
    return true;
}

void ClusterDAG::SaveCache(const char* path, u64 geomStamp, const xr_vector<ClusterBakeRange>& ranges) const
{
    if (!path || !path[0] || m_records.empty() || m_bakedIndices.empty())
        return;

    IWriter* w = FS.w_open(path);
    if (!w) {
        Msg("! [ClusterDAG] failed to open cache for write: %s", path);
        return;
    }

    CacheHeader hdr = {};
    hdr.magic = kCacheMagic;
    hdr.version = kCacheVersion;
    hdr.paramsHash = ComputeParamsHash();
    hdr.geomStamp = geomStamp;
    hdr.rangeSetHash = ComputeRangeSetHash(ranges);
    hdr.recordCount = u32(m_records.size());
    hdr.memberCount = u32(m_memberKeys.size());
    hdr.protoCount = u32(m_protos.size());
    hdr.indexCount = m_bakedIndices.size();

    w->w(&hdr, sizeof(hdr));
    w->w(m_records.data(), u32(m_records.size() * sizeof(ClusterUnitRecord)));
    w->w(m_memberKeys.data(), u32(m_memberKeys.size() * sizeof(ClusterMeshKey)));
    w->w(m_protos.data(), u32(m_protos.size() * sizeof(ClusterMetaProto)));
    w->w(m_bakedIndices.data(), u32(m_bakedIndices.size() * sizeof(u32)));
    FS.w_close(w);

    Msg("* [ClusterDAG] cache saved: %s", path);
}

void ClusterDAG::RunDiagnostics()
{
    xr_vector<ClusterBoundsKey> selfKeys;

    for (const ClusterUnitRecord& rec : m_records) {
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
        selfKeys.erase(std::unique(selfKeys.begin(), selfKeys.end()), selfKeys.end());

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
