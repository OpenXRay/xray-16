#include "stdafx.h"
#include "ClusterDAG.h"
#include "ClusterDeviation.h"
#include "ClusterBasis.h"
#include "Bindless/VertexConverter.h"
#include "xrRender_console.h"

#include "../../../Externals/meshoptimizer/src/meshoptimizer.h"
#define CLUSTERLOD_IMPLEMENTATION
#include "clusterlod.h"

#include <atomic>
#include <thread>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <cstdio>

namespace xray::render::fg
{

namespace
{

constexpr u32 kClusterMaxTris = CLUSTER_MAX_TRIANGLES;
static_assert(CLUSTER_MAX_VERTICES == CLUSTER_MAX_TRIANGLES,
    "clodDefaultConfig derives max_vertices from max_triangles");
constexpr float kErrorCap = 1e30f;
constexpr float kDilate = 0.5f;
constexpr u32 kMaxComponentTris = 200000;
constexpr float kMaxComponentExtent = 48.0f;
constexpr float kTerrainMaxComponentExtent = 128.0f;
constexpr float kGapCut = 0.75f;
constexpr u32 kSharedPosition = 0xFFFFFFFFu;
constexpr float kDirectionWeight = 0.5f;
constexpr float kDirectionMinLenSq = 1e-8f;
constexpr float kDeviationRaiseTol = 1e-3f;
constexpr float kDiagRelTol = 1e-4f;
constexpr float kDiagAbsTol = 1e-3f;
constexpr char kBakePolicyTag[] = "clod:err=maxchild+step;attr=ntb9+uv0;seam=uv0uv1;locks=member+unit;pos=zeronorm;basis=onb-duff;pack=page-u8tri-u16remap";

float DomainExtentCap(u32 rangeFlags)
{
    return (rangeFlags & CLUSTER_RANGE_FLAG_TERRAIN) ? kTerrainMaxComponentExtent : kMaxComponentExtent;
}

struct RangeInfo {
    ClusterMeshKey key;
    u32 flags;
    bool floatBasis;
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

struct SimplifyStats {
    u32 groups = 0;
    u32 stalled = 0;
    u32 raised = 0;
    u32 metricRejected = 0;
    float maxRaise = 0.0f;
};

struct BakeResult {
    xr_vector<ClusterMetaProto> protos;
    xr_vector<ClusterGroupProto> groups;
    xr_vector<u32> indices;
    SimplifyStats simplify;
    u32 pinnedVerts = 0;
    u32 memberPinnedVerts = 0;
    u32 memberSplitViolations = 0;
    u32 reclusterSplits = 0;
    bool baked = false;
};

struct BakeContext {
    BakeResult* result;
    xr_vector<clodBounds> groups;
    const bindless::UnifiedVertex* verts;
    const Fvector3* floatNormals;
    const u32* absoluteOfLocal;
    const u32* memberOfLocal;
    u32 memberCount;
    u32 protoFlags;
};

u64 HashPosition(const Fvector& p)
{
    u32 words[3];
    memcpy(words, &p, sizeof(words));
    for (u32 i = 0; i < 3; ++i) {
        if (words[i] == 0x80000000u)
            words[i] = 0u;
    }
    const u8* bytes = reinterpret_cast<const u8*>(words);
    u64 h = 14695981039346656037ull;
    for (u32 i = 0; i < sizeof(words); ++i) {
        h ^= bytes[i];
        h *= 1099511628211ull;
    }
    return h;
}

float EstimateUVWorldScale(const bindless::UnifiedVertex* verts, const u32* indices, u32 indexCount)
{
    xr_vector<float> ratios;
    ratios.reserve(indexCount);
    for (u32 i = 0; i + 2 < indexCount; i += 3) {
        const u32 idx[3] = { indices[i], indices[i + 1], indices[i + 2] };
        for (u32 e = 0; e < 3; ++e) {
            const bindless::UnifiedVertex& a = verts[idx[e]];
            const bindless::UnifiedVertex& b = verts[idx[(e + 1) % 3]];
            const float dp = a.position.distance_to(b.position);
            const float du = _sqrt(_sqr(a.texcoord0.x - b.texcoord0.x) + _sqr(a.texcoord0.y - b.texcoord0.y));
            if (du > 1e-5f && dp > 1e-5f)
                ratios.push_back(dp / du);
        }
    }
    if (ratios.empty())
        return 1.0f;
    std::nth_element(ratios.begin(), ratios.begin() + ratios.size() / 2, ratios.end());
    return clampr(ratios[ratios.size() / 2], 0.01f, 1000.0f);
}

void MeasureBounds(const bindless::UnifiedVertex* verts, const u32* indices, u32 indexCount,
    float* center, float* radius, float* extent)
{
    Fvector lo, hi;
    lo.set(flt_max, flt_max, flt_max);
    hi.set(-flt_max, -flt_max, -flt_max);
    for (u32 i = 0; i < indexCount; ++i) {
        const Fvector& v = verts[indices[i]].position;
        lo.min(v);
        hi.max(v);
    }
    Fvector c;
    c.add(lo, hi).mul(0.5f);
    float r2 = 0.0f;
    for (u32 i = 0; i < indexCount; ++i)
        r2 = std::max(r2, c.distance_to_sqr(verts[indices[i]].position));
    center[0] = c.x;
    center[1] = c.y;
    center[2] = c.z;
    *radius = _sqrt(r2);
    extent[0] = (hi.x - lo.x) * 0.5f;
    extent[1] = (hi.y - lo.y) * 0.5f;
    extent[2] = (hi.z - lo.z) * 0.5f;
}

u32 CountUniqueIndices(const u32* indices, u32 indexCount)
{
    static thread_local std::unordered_set<u32> s_unique;
    s_unique.clear();
    for (u32 i = 0; i < indexCount; ++i)
        s_unique.insert(indices[i]);
    return u32(s_unique.size());
}

void EmitProtoChunk(BakeResult& out, const BakeContext& ctx, const clodCluster& c,
    const clodBounds& parent, int depth, u32 member, u32 owningGroup,
    const u32* indices, u32 indexCount, const u32* boundsIndices)
{
    ClusterMetaProto p = {};
    MeasureBounds(ctx.verts, boundsIndices, indexCount, p.sphere, &p.sphere[3], p.extent);

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
    p.owningGroup = owningGroup;
    p.refinedGroup = (c.refined >= 0) ? u32(c.refined) : UINT32_MAX;

    out.indices.insert(out.indices.end(), indices, indices + indexCount);
    out.protos.push_back(p);
}

void EmitProto(BakeResult& out, const BakeContext& ctx, const clodCluster& c,
    const clodBounds& parent, int depth, u32 member, u32 owningGroup,
    const u32* indices, u32 indexCount, const u32* boundsIndices)
{
    const u32 triangleTotal = indexCount / 3;
    if (triangleTotal <= CLUSTER_MAX_TRIANGLES &&
        CountUniqueIndices(indices, indexCount) <= CLUSTER_MAX_VERTICES) {
        EmitProtoChunk(out, ctx, c, parent, depth, member, owningGroup,
            indices, indexCount, boundsIndices);
        return;
    }

    static thread_local std::unordered_set<u32> s_chunkUnique;
    u32 first = 0;
    while (first < triangleTotal) {
        s_chunkUnique.clear();
        u32 last = first;
        while (last < triangleTotal && last - first < CLUSTER_MAX_TRIANGLES) {
            const u32 a = indices[last * 3 + 0];
            const u32 b = indices[last * 3 + 1];
            const u32 d = indices[last * 3 + 2];
            u32 added = 0;
            added += s_chunkUnique.count(a) ? 0u : 1u;
            if (b != a)
                added += s_chunkUnique.count(b) ? 0u : 1u;
            if (d != a && d != b)
                added += s_chunkUnique.count(d) ? 0u : 1u;
            if (last > first && s_chunkUnique.size() + added > CLUSTER_MAX_VERTICES)
                break;
            s_chunkUnique.insert(a);
            s_chunkUnique.insert(b);
            s_chunkUnique.insert(d);
            ++last;
        }
        EmitProtoChunk(out, ctx, c, parent, depth, member, owningGroup,
            indices + first * 3, (last - first) * 3, boundsIndices + first * 3);
        out.reclusterSplits++;
        first = last;
    }
}

int OutputGroupCB(void* ctxv, clodGroup group, const clodCluster* clusters, size_t clusterCount)
{
    BakeContext* ctx = static_cast<BakeContext*>(ctxv);

    const bool terminal = !(group.simplified.error < kErrorCap);
    clodBounds g = group.simplified;
    if (terminal)
        g.error = kErrorCap;

    const int groupId = int(ctx->groups.size());
    ctx->groups.push_back(g);
    const clodBounds& parent = ctx->groups[groupId];

    BakeResult& out = *ctx->result;

    ClusterGroupProto gp = {};
    gp.sphere[0] = g.center[0];
    gp.sphere[1] = g.center[1];
    gp.sphere[2] = g.center[2];
    gp.sphere[3] = g.radius;
    gp.error = g.error;
    gp.unit = 0;
    gp.localGroup = u32(groupId);
    gp.depth = u32(group.depth < 0 ? 0 : group.depth);
    gp.flags = terminal ? CLUSTER_GROUP_FLAG_TERMINAL : 0u;
    gp.reserved = 0;
    out.groups.push_back(gp);

    for (size_t i = 0; i < clusterCount; ++i) {
        const clodCluster& c = clusters[i];

        if (!ctx->absoluteOfLocal) {
            EmitProto(out, *ctx, c, parent, group.depth, 0, u32(groupId),
                c.indices, u32(c.index_count), c.indices);
            continue;
        }

        static thread_local xr_vector<u32> s_translated;
        static thread_local xr_vector<u32> s_triMember;
        const size_t triCount = c.index_count / 3;
        s_translated.resize(c.index_count);
        s_triMember.resize(triCount);

        for (size_t k = 0; k < c.index_count; ++k)
            s_translated[k] = ctx->absoluteOfLocal[c.indices[k]];

        for (size_t t = 0; t < triCount; ++t) {
            const u32 m0 = ctx->memberOfLocal[c.indices[t * 3 + 0]];
            const u32 m1 = ctx->memberOfLocal[c.indices[t * 3 + 1]];
            const u32 m2 = ctx->memberOfLocal[c.indices[t * 3 + 2]];
            if (m0 != m1 || m0 != m2)
                out.memberSplitViolations++;
            s_triMember[t] = m0;
        }

        for (u32 m = 0; m < ctx->memberCount; ++m) {
            static thread_local xr_vector<u32> s_memberIndices;
            static thread_local xr_vector<u32> s_memberLocal;
            s_memberIndices.clear();
            s_memberLocal.clear();
            for (size_t t = 0; t < triCount; ++t) {
                if (s_triMember[t] != m)
                    continue;
                for (u32 e = 0; e < 3; ++e) {
                    s_memberIndices.push_back(s_translated[t * 3 + e]);
                    s_memberLocal.push_back(c.indices[t * 3 + e]);
                }
            }
            if (s_memberIndices.empty())
                continue;
            EmitProto(out, *ctx, c, parent, group.depth, m, u32(groupId),
                s_memberIndices.data(), u32(s_memberIndices.size()), s_memberLocal.data());
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

void DecodeDirection(u32 packed, float* out)
{
    const float x = float((packed >> 16) & 0xFFu) * (2.0f / 255.0f) - 1.0f;
    const float y = float((packed >> 8) & 0xFFu) * (2.0f / 255.0f) - 1.0f;
    const float z = float(packed & 0xFFu) * (2.0f / 255.0f) - 1.0f;
    const float lenSq = x * x + y * y + z * z;
    if (lenSq > kDirectionMinLenSq) {
        const float inv = 1.0f / _sqrt(lenSq);
        out[0] = x * inv;
        out[1] = y * inv;
        out[2] = z * inv;
        return;
    }
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
}

void FillAttributes(const bindless::UnifiedVertex* verts, const Fvector3* floatNormals,
    u32 count, float* attrs)
{
    for (u32 v = 0; v < count; ++v) {
        const bindless::UnifiedVertex& src = verts[v];
        float* a = attrs + size_t(v) * kClusterAttrStride;
        if ((src.flags & bindless::UNIFIED_VERTEX_FLAG_FLOAT_BASIS) && floatNormals) {
            const Fvector3 n = ClusterNormalizeBasisInput(floatNormals[v]);
            Fvector3 t;
            Fvector3 b;
            ClusterDeriveBasis(n, t, b);
            a[0] = n.x; a[1] = n.y; a[2] = n.z;
            a[3] = t.x; a[4] = t.y; a[5] = t.z;
            a[6] = b.x; a[7] = b.y; a[8] = b.z;
        } else {
            DecodeDirection(src.normal, a);
            DecodeDirection(src.tangent, a + 3);
            DecodeDirection(src.binormal, a + 6);
        }
        float* uv = a + kClusterAttrDirections;
        uv[0] = src.texcoord0.x;
        uv[1] = src.texcoord0.y;
        uv[2] = src.texcoord1.x;
        uv[3] = src.texcoord1.y;
    }
}

void FillAttributeWeights(float uvScale, float* weights)
{
    for (u32 i = 0; i < kClusterAttrDirections; ++i)
        weights[i] = kDirectionWeight;
    weights[kClusterAttrDirections + 0] = uvScale;
    weights[kClusterAttrDirections + 1] = uvScale;
}

float SimplifyErrorHook(void* ctxv, const clodMesh* mesh, const u32* source, size_t sourceCount,
    const u32* result, size_t resultCount, u32 method, float error)
{
    BakeContext* ctx = static_cast<BakeContext*>(ctxv);
    SimplifyStats& st = ctx->result->simplify;
    st.groups++;
    if (method & 4) {
        st.stalled++;
        return error;
    }

    float deviation = 0.0f;
    if (!ClusterMeshDeviation(
            mesh->vertex_positions, mesh->vertex_positions_stride / sizeof(float),
            mesh->vertex_attributes, mesh->vertex_attributes_stride / sizeof(float),
            mesh->attribute_weights, u32(mesh->attribute_count),
            source, sourceCount, result, resultCount, deviation)) {
        st.metricRejected++;
        return error;
    }

    if (deviation > error + kDeviationRaiseTol) {
        st.raised++;
        st.maxRaise = std::max(st.maxRaise, deviation - error);
    }
    return std::max(error, deviation);
}

clodConfig MakeBakeConfig(bool leavesOnly)
{
    clodConfig cfg = clodDefaultConfig(kClusterMaxTris);
    cfg.optimize_bounds = true;
    cfg.simplify_permissive = false;
    cfg.simplify_fallback_permissive = false;
    cfg.simplify_fallback_sloppy = false;
    cfg.simplify_prune = false;
    if (leavesOnly)
        cfg.simplify_ratio = 1.0f;
    return cfg;
}

clodConfig MakeConfig(bool leavesOnly, BakeContext& ctx)
{
    clodConfig cfg = MakeBakeConfig(leavesOnly);
    cfg.simplify_error_hook = &SimplifyErrorHook;
    cfg.simplify_error_context = &ctx;
    return cfg;
}

void FinishBake(BakeResult& result)
{
    if (result.simplify.metricRejected || result.memberSplitViolations) {
        result.protos.clear();
        result.indices.clear();
        result.groups.clear();
    }
    result.baked = !result.protos.empty();
}

void BakeSingleMesh(
    const ClusterMeshKey& key,
    u32 rangeFlags,
    const bindless::UnifiedVertex* megaVertices,
    const Fvector3* megaFloatNormals,
    const u32* megaIndices,
    BakeResult& result)
{
    result.baked = false;

    const bindless::UnifiedVertex* verts = megaVertices + key.vertexOffset;
    const Fvector3* normals = megaFloatNormals ? megaFloatNormals + key.vertexOffset : nullptr;
    const u32* indices = megaIndices + key.indexOffset;

    xr_vector<float> attrs;
    attrs.resize(size_t(key.vertexCount) * kClusterAttrStride);
    FillAttributes(verts, normals, key.vertexCount, attrs.data());

    const float uvScale = EstimateUVWorldScale(verts, indices, key.indexCount);
    float weights[kClusterAttrWeighted];
    FillAttributeWeights(uvScale, weights);

    clodMesh mesh = {};
    mesh.indices = indices;
    mesh.index_count = key.indexCount;
    mesh.vertex_count = key.vertexCount;
    mesh.vertex_positions = reinterpret_cast<const float*>(verts);
    mesh.vertex_positions_stride = sizeof(bindless::UnifiedVertex);
    mesh.vertex_attributes = attrs.data();
    mesh.vertex_attributes_stride = kClusterAttrStride * sizeof(float);
    mesh.attribute_weights = weights;
    mesh.attribute_count = kClusterAttrWeighted;
    mesh.attribute_protect_mask = kClusterAttrProtectMask;

    BakeContext ctx = {};
    ctx.result = &result;
    ctx.groups.reserve(64);
    ctx.verts = verts;
    ctx.floatNormals = normals;
    ctx.protoFlags = ((rangeFlags & CLUSTER_RANGE_FLAG_AT) ? CLUSTER_PROTO_FLAG_AT : 0) |
                     ((rangeFlags & CLUSTER_RANGE_FLAG_TERRAIN) ? CLUSTER_PROTO_FLAG_TERRAIN : 0);

    clodBuild(MakeConfig((rangeFlags & CLUSTER_RANGE_FLAG_AT) != 0, ctx), mesh, &ctx, &OutputGroupCB);
    FinishBake(result);
}

void BakeLeafOnly(
    const ClusterMeshKey& key,
    u32 rangeFlags,
    const bindless::UnifiedVertex* megaVertices,
    const Fvector3* megaFloatNormals,
    const u32* megaIndices,
    BakeResult& result)
{
    result.baked = false;

    const bindless::UnifiedVertex* verts = megaVertices + key.vertexOffset;
    const Fvector3* normals = megaFloatNormals ? megaFloatNormals + key.vertexOffset : nullptr;
    const u32* indices = megaIndices + key.indexOffset;

    xr_vector<float> attrs;
    attrs.resize(size_t(key.vertexCount) * kClusterAttrStride);
    FillAttributes(verts, normals, key.vertexCount, attrs.data());

    const float uvScale = EstimateUVWorldScale(verts, indices, key.indexCount);
    float weights[kClusterAttrWeighted];
    FillAttributeWeights(uvScale, weights);

    clodMesh mesh = {};
    mesh.indices = indices;
    mesh.index_count = key.indexCount;
    mesh.vertex_count = key.vertexCount;
    mesh.vertex_positions = reinterpret_cast<const float*>(verts);
    mesh.vertex_positions_stride = sizeof(bindless::UnifiedVertex);
    mesh.vertex_attributes = attrs.data();
    mesh.vertex_attributes_stride = kClusterAttrStride * sizeof(float);
    mesh.attribute_weights = weights;
    mesh.attribute_count = kClusterAttrWeighted;
    mesh.attribute_protect_mask = kClusterAttrProtectMask;

    BakeContext ctx = {};
    ctx.result = &result;
    ctx.groups.reserve(16);
    ctx.verts = verts;
    ctx.floatNormals = normals;
    ctx.protoFlags = ((rangeFlags & CLUSTER_RANGE_FLAG_AT) ? CLUSTER_PROTO_FLAG_AT : 0) |
                     ((rangeFlags & CLUSTER_RANGE_FLAG_TERRAIN) ? CLUSTER_PROTO_FLAG_TERRAIN : 0);

    clodBuild(MakeBakeConfig(true), mesh, &ctx, &OutputGroupCB);
    result.baked = !result.protos.empty();
}

void BakeComponent(
    const xr_vector<RangeInfo>& ranges,
    const xr_vector<u32>& members,
    const bindless::UnifiedVertex* megaVertices,
    const Fvector3* megaFloatNormals,
    const u32* megaIndices,
    const std::unordered_set<u64>& crossUnitPins,
    BakeResult& result)
{
    result.baked = false;

    u32 totalVerts = 0;
    u32 totalIndices = 0;
    for (u32 m : members) {
        totalVerts += ranges[m].key.vertexCount;
        totalIndices += ranges[m].key.indexCount;
    }

    xr_vector<bindless::UnifiedVertex> verts;
    xr_vector<Fvector3> normals;
    xr_vector<u32> indices;
    xr_vector<u32> absoluteOfLocal;
    xr_vector<u32> memberOfLocal;
    xr_vector<u64> posHash;
    xr_vector<unsigned char> locks;
    verts.reserve(totalVerts);
    if (megaFloatNormals)
        normals.reserve(totalVerts);
    indices.reserve(totalIndices);
    absoluteOfLocal.reserve(totalVerts);
    memberOfLocal.reserve(totalVerts);
    posHash.reserve(totalVerts);
    locks.resize(totalVerts, 0);

    std::unordered_map<u64, u32> posOwner;
    posOwner.reserve(totalVerts);

    u32 pinned = 0;

    for (u32 mi = 0; mi < u32(members.size()); ++mi) {
        const ClusterMeshKey& key = ranges[members[mi]].key;
        const bindless::UnifiedVertex* src = megaVertices + key.vertexOffset;
        const u32* srcIdx = megaIndices + key.indexOffset;
        const u32 base = u32(verts.size());

        verts.insert(verts.end(), src, src + key.vertexCount);
        if (megaFloatNormals)
            normals.insert(normals.end(), megaFloatNormals + key.vertexOffset,
                megaFloatNormals + key.vertexOffset + key.vertexCount);
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
            posHash.push_back(h);

            auto it = firstAtPos.find(h);
            if (it == firstAtPos.end()) {
                firstAtPos.emplace(h, v);
            } else {
                const bindless::UnifiedVertex& canon = src[it->second];
                if (vert.texcoord0.x != canon.texcoord0.x || vert.texcoord0.y != canon.texcoord0.y ||
                    vert.texcoord1.x != canon.texcoord1.x || vert.texcoord1.y != canon.texcoord1.y)
                    locks[base + v] |= meshopt_SimplifyVertex_Protect;
            }

            auto owner = posOwner.find(h);
            if (owner == posOwner.end())
                posOwner.emplace(h, mi);
            else if (owner->second != mi)
                owner->second = kSharedPosition;

            if (crossUnitPins.count(h)) {
                locks[base + v] |= meshopt_SimplifyVertex_Lock;
                pinned++;
            }
        }
    }

    u32 memberPinned = 0;
    for (u32 v = 0; v < u32(verts.size()); ++v) {
        if (posOwner[posHash[v]] != kSharedPosition)
            continue;
        if (!(locks[v] & meshopt_SimplifyVertex_Lock))
            memberPinned++;
        locks[v] |= meshopt_SimplifyVertex_Lock;
    }

    result.pinnedVerts = pinned;
    result.memberPinnedVerts = memberPinned;

    const float uvScale = EstimateUVWorldScale(verts.data(), indices.data(), u32(indices.size()));
    float weights[kClusterAttrWeighted];
    FillAttributeWeights(uvScale, weights);

    xr_vector<float> attrs;
    attrs.resize(size_t(verts.size()) * kClusterAttrStride);
    FillAttributes(verts.data(), normals.empty() ? nullptr : normals.data(),
        u32(verts.size()), attrs.data());

    clodMesh mesh = {};
    mesh.indices = indices.data();
    mesh.index_count = indices.size();
    mesh.vertex_count = verts.size();
    mesh.vertex_positions = reinterpret_cast<const float*>(verts.data());
    mesh.vertex_positions_stride = sizeof(bindless::UnifiedVertex);
    mesh.vertex_attributes = attrs.data();
    mesh.vertex_attributes_stride = kClusterAttrStride * sizeof(float);
    mesh.attribute_weights = weights;
    mesh.attribute_count = kClusterAttrWeighted;
    mesh.attribute_protect_mask = kClusterAttrProtectMask;
    mesh.vertex_lock = locks.data();

    BakeContext ctx = {};
    ctx.result = &result;
    ctx.groups.reserve(256);
    ctx.verts = verts.data();
    ctx.floatNormals = normals.empty() ? nullptr : normals.data();
    ctx.absoluteOfLocal = absoluteOfLocal.data();
    ctx.memberOfLocal = memberOfLocal.data();
    ctx.memberCount = u32(members.size());
    ctx.protoFlags = (ranges[members[0]].flags & CLUSTER_RANGE_FLAG_TERRAIN)
        ? CLUSTER_PROTO_FLAG_TERRAIN : 0;

    clodBuild(MakeConfig(false, ctx), mesh, &ctx, &OutputGroupCB);
    FinishBake(result);
}

bool IsSelfLoop(const ClusterMetaProto& p)
{
    if (!(p.selfError > 0.0f) || !(p.parentError < kErrorCap))
        return false;
    return memcmp(p.lodSelf, p.lodParent, sizeof(p.lodSelf)) == 0 && p.selfError == p.parentError;
}

bool FiniteSphere(const float* s)
{
    return std::isfinite(s[0]) && std::isfinite(s[1]) && std::isfinite(s[2]) &&
           std::isfinite(s[3]) && s[3] >= 0.0f;
}

float DiagTolerance(float scale)
{
    return kDiagAbsTol + kDiagRelTol * std::max(1.0f, scale);
}

bool SphereContains(const float* outer, const float* inner)
{
    const float dx = inner[0] - outer[0];
    const float dy = inner[1] - outer[1];
    const float dz = inner[2] - outer[2];
    const float dist = _sqrt(dx * dx + dy * dy + dz * dz);
    return dist + inner[3] <= outer[3] + DiagTolerance(dist + inner[3] + outer[3]);
}

bool ProtoStructureValid(const ClusterMetaProto& p, const ClusterUnitRecord& rec)
{
    if (p.member >= rec.memberCount)
        return false;
    if (p.indexCount < 3 || p.indexCount % 3 != 0 || p.indexCount > 3 * kClusterMaxTris)
        return false;
    if (u64(p.ibFirst) + p.indexCount > rec.indexTotal)
        return false;
    if (p.owningGroup < rec.firstGroup || p.owningGroup >= rec.firstGroup + rec.groupCount)
        return false;
    if (p.refinedGroup != UINT32_MAX &&
        (p.refinedGroup < rec.firstGroup || p.refinedGroup >= rec.firstGroup + rec.groupCount))
        return false;
    if (!FiniteSphere(p.sphere) || !FiniteSphere(p.lodSelf) || !FiniteSphere(p.lodParent))
        return false;
    if (!std::isfinite(p.extent[0]) || !std::isfinite(p.extent[1]) || !std::isfinite(p.extent[2]))
        return false;
    if (!(p.selfError >= 0.0f) || !(p.selfError <= kErrorCap))
        return false;
    if (!(p.parentError >= 0.0f) || !(p.parentError <= kErrorCap))
        return false;
    return true;
}

constexpr u32 kCacheMagic = 0x464C4356;
constexpr u32 kCacheVersion = 14;

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
    u32 groupCount;
    u64 indexCount;
    u32 payloadVersion;
    u32 maxVertices;
    u32 maxTriangles;
    u32 reserved;
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

u32 FloatBits(float v)
{
    u32 bits;
    memcpy(&bits, &v, sizeof(bits));
    return bits;
}

u64 ComputeParamsHash()
{
    const clodConfig full = MakeBakeConfig(false);
    const clodConfig leaves = MakeBakeConfig(true);

    const u32 policy =
        u32(full.partition_spatial) | (u32(full.partition_sort) << 1) |
        (u32(full.cluster_spatial) << 2) | (u32(full.simplify_permissive) << 3) |
        (u32(full.simplify_fallback_permissive) << 4) | (u32(full.simplify_fallback_sloppy) << 5) |
        (u32(full.simplify_prune) << 6) | (u32(full.simplify_regularize) << 7) |
        (u32(full.optimize_bounds) << 8) | (u32(full.optimize_clusters) << 9);

    const u32 words[] = {
        kCacheVersion, kClusterMaxTris,
        u32(std::max(ps_r_cluster_tris, int(kClusterMaxTris))), u32(ps_r_cluster_merge != 0),
        kMaxComponentTris, FloatBits(kMaxComponentExtent), FloatBits(kTerrainMaxComponentExtent),
        FloatBits(kDilate), FloatBits(kGapCut),
        u32(full.max_vertices), u32(full.min_triangles), u32(full.max_triangles),
        u32(full.partition_size), policy,
        FloatBits(full.simplify_ratio), FloatBits(leaves.simplify_ratio),
        FloatBits(full.simplify_threshold), FloatBits(full.cluster_fill_weight),
        FloatBits(full.cluster_split_factor), FloatBits(full.simplify_error_edge_limit),
        kClusterAttrDirections, kClusterAttrWeighted, kClusterAttrStride, kClusterAttrProtectMask,
        FloatBits(kDirectionWeight), FloatBits(kDeviationRaiseTol), FloatBits(kErrorCap),
        CLUSTER_PAYLOAD_VERSION, CLUSTER_MAX_VERTICES, CLUSTER_MAX_TRIANGLES,
        CLUSTER_PAGE_VERTEX_STRIDE, CLUSTER_PAGE_MAX_VERTICES, CLUSTER_PAGE_MAX_CLUSTERS,
        CLUSTER_PAGE_MAX_PAYLOAD_BYTES, u32(sizeof(ClusterMetaProto)), u32(sizeof(ClusterGroupProto)),
        u32(sizeof(ClusterUnitRecord))};

    return Fnv1a64(kBakePolicyTag, sizeof(kBakePolicyTag), Fnv1a64(words, sizeof(words)));
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
    const float extentCap = DomainExtentCap(ranges[members[0]].flags);

    if (members.size() <= 1 || (tris <= kMaxComponentTris && extent <= extentCap)) {
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
    m_groupProtos.clear();
    m_bakedIndices.clear();
    m_clusterMeta.clear();
    m_assetMembers.clear();
    m_assetNodes.clear();
    m_pages.clear();
    m_pageVertexData.clear();
    m_pagePayloadData.clear();
    m_runtimePageVertexData.clear();
    m_runtimePagePayloadData.clear();
    m_pageClass.clear();
    m_groupPinned.clear();
    m_groupMemberPages.clear();
    m_residencyStats = {};
    m_storePageCount = 0;
    m_storeVertexOrigin = 0;
    m_storePayloadOrigin = 0;
    m_storeIdentity = 0;
    m_storeVertexBytes = 0;
    m_storePayloadBytes = 0;
    m_clusterProto.clear();
    m_clusterOwningGroup.clear();
    m_clusterRefinedGroup.clear();
    m_groups.clear();
    m_groupMemberClusters.clear();
    m_groupReplacementClusters.clear();
    m_groupPages.clear();
    m_groupDependencies.clear();
    m_assetMemberRecord.clear();
    m_maxAssetNodeDepth = 0;
    m_stats = {};
    m_payloadStats = {};
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

bool ClusterDAG::FindAssetMember(const ClusterMeshKey& key, u32& outAssetMember) const
{
    auto it = std::lower_bound(m_lookup.begin(), m_lookup.end(), key,
        [](const LookupEntry& e, const ClusterMeshKey& k) { return e.key < k; });
    if (it == m_lookup.end() || !(it->key == key))
        return false;
    outAssetMember = m_records[it->record].firstMember + it->member;
    return true;
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
    const ClusterSourceView& source)
{
    Clear();

    CTimer timer;
    timer.Start();

    const bindless::UnifiedVertex* megaVertices = source.vertices;
    const Fvector3* megaFloatNormals = source.floatNormals;
    const u32* megaIndices = source.indices;

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
            if (!ValidateRange(r.key, megaIndices + r.key.indexOffset)) {
                m_stats.invalidRanges++;
                FATAL_F("[ClusterDAG] unsupported geometry range v=%u+%u i=%u+%u cannot be represented as clusters",
                    r.key.vertexOffset, r.key.vertexCount, r.key.indexOffset, r.key.indexCount);
            }
            RangeInfo info;
            info.key = r.key;
            info.flags = r.flags;
            info.floatBasis = (megaVertices[r.key.vertexOffset].flags
                & bindless::UNIFIED_VERTEX_FLAG_FLOAT_BASIS) != 0;
            for (u32 v = 1; v < r.key.vertexCount; ++v) {
                const bool basis = (megaVertices[r.key.vertexOffset + v].flags
                    & bindless::UNIFIED_VERTEX_FLAG_FLOAT_BASIS) != 0;
                if (basis != info.floatBasis)
                    FATAL_F("[ClusterDAG] geometry range v=%u+%u mixes packed and float vertex bases",
                        r.key.vertexOffset, r.key.vertexCount);
            }
            if (info.floatBasis && !megaFloatNormals)
                FATAL_F("[ClusterDAG] geometry range v=%u+%u needs float source normals that were not retained",
                    r.key.vertexOffset, r.key.vertexCount);
            if (!mergeEnabled)
                info.flags &= ~CLUSTER_RANGE_FLAG_MERGEABLE;
            if (info.flags & CLUSTER_RANGE_FLAG_TERRAIN)
                m_stats.terrainMeshes++;
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
                if ((a.flags ^ b.flags) & CLUSTER_RANGE_FLAG_TERRAIN)
                    continue;
                if (a.floatBasis != b.floatBasis)
                    continue;
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
            if ((o.flags ^ infos[componentMembers[c][0]].flags) & CLUSTER_RANGE_FLAG_TERRAIN)
                continue;
            if (o.floatBasis != infos[componentMembers[c][0]].floatBasis)
                continue;
            if (UnitTris(infos, componentMembers[c]) + o.key.indexCount / 3 > kMaxComponentTris)
                continue;
            Fvector mn, mx;
            UnitAABB(infos, componentMembers[c], mn, mx);
            Fvector nmn = mn, nmx = mx;
            nmn.min(o.aabbMin);
            nmx.max(o.aabbMax);
            const float newExtent = std::max(nmx.x - nmn.x, std::max(nmx.y - nmn.y, nmx.z - nmn.z));
            if (newExtent > DomainExtentCap(o.flags))
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
        } else {
            infos[orphan].flags &= ~CLUSTER_RANGE_FLAG_MERGEABLE;
            m_stats.orphanStandalone++;
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
                    else if (it->second != c && it->second != kSharedPosition) {
                        crossUnitPins.insert(h);
                        it->second = kSharedPosition;
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
        if (infos[comp[0]].flags & CLUSTER_RANGE_FLAG_TERRAIN)
            m_stats.terrainComponents++;
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
            m_stats.smallStandalone++;
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
                BakeComponent(infos, unit.members, megaVertices, megaFloatNormals, megaIndices,
                    crossUnitPins, results[i]);
            else
                BakeSingleMesh(infos[unit.members[0]].key, unit.flags,
                    megaVertices, megaFloatNormals, megaIndices, results[i]);
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

    xr_vector<u32> leafFallback;
    for (size_t i = 0; i < units.size(); ++i) {
        BakeResult& r = results[i];
        m_stats.simplifiedGroups += r.simplify.groups;
        m_stats.stalledGroups += r.simplify.stalled;
        m_stats.deviationRaised += r.simplify.raised;
        m_stats.metricRejectedGroups += r.simplify.metricRejected;
        m_stats.memberSplitViolations += r.memberSplitViolations;
        m_payloadStats.reclusterSplits += r.reclusterSplits;
        m_stats.pinnedVerts += r.pinnedVerts;
        m_stats.memberPinnedVerts += r.memberPinnedVerts;
        m_stats.deviationMaxRaise = std::max(m_stats.deviationMaxRaise, r.simplify.maxRaise);
        if (!r.baked) {
            m_stats.bakeFailed++;
            for (u32 m : units[i].members)
                leafFallback.push_back(m);
            continue;
        }

        ClusterUnitRecord rec;
        rec.firstMember = u32(m_memberKeys.size());
        rec.memberCount = u32(units[i].members.size());
        rec.firstProto = u32(m_protos.size());
        rec.firstIndex = u32(m_bakedIndices.size());
        rec.firstGroup = u32(m_groupProtos.size());
        rec.groupCount = u32(r.groups.size());
        rec.isComponent = units[i].isComponent ? 1 : 0;

        for (u32 m : units[i].members)
            m_memberKeys.push_back(infos[m].key);

        const u32 unitIndex = u32(m_records.size());
        for (ClusterGroupProto gp : r.groups) {
            gp.unit = unitIndex;
            m_groupProtos.push_back(gp);
        }

        u32 nonLoops = 0;
        for (const ClusterMetaProto& p : r.protos) {
            if (!IsSelfLoop(p))
                nonLoops++;
        }

        u32 kept = 0;
        for (ClusterMetaProto p : r.protos) {
            if (nonLoops > 0 && IsSelfLoop(p)) {
                m_stats.droppedSelfLoops++;
                continue;
            }
            p.owningGroup += rec.firstGroup;
            if (p.refinedGroup != UINT32_MAX)
                p.refinedGroup += rec.firstGroup;
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
            m_groupProtos.resize(rec.firstGroup);
            for (u32 m : units[i].members)
                leafFallback.push_back(m);
            continue;
        }

        m_records.push_back(rec);
        m_stats.bakedMeshes += rec.memberCount;

        r.protos.clear();
        r.protos.shrink_to_fit();
        r.groups.clear();
        r.groups.shrink_to_fit();
        r.indices.clear();
        r.indices.shrink_to_fit();
    }

    for (u32 m : leafFallback) {
        BakeResult leaf;
        BakeLeafOnly(infos[m].key, infos[m].flags, megaVertices, megaFloatNormals, megaIndices, leaf);
        if (!leaf.baked)
            FATAL_F("[ClusterDAG] exact leaf coverage failed for geometry range v=%u+%u i=%u+%u",
                infos[m].key.vertexOffset, infos[m].key.vertexCount,
                infos[m].key.indexOffset, infos[m].key.indexCount);

        ClusterUnitRecord rec;
        rec.firstMember = u32(m_memberKeys.size());
        rec.memberCount = 1;
        rec.firstProto = u32(m_protos.size());
        rec.firstIndex = u32(m_bakedIndices.size());
        rec.firstGroup = u32(m_groupProtos.size());
        rec.groupCount = u32(leaf.groups.size());
        rec.isComponent = 0;
        rec.protoCount = u32(leaf.protos.size());
        rec.indexTotal = u32(leaf.indices.size());
        m_memberKeys.push_back(infos[m].key);

        const u32 unitIndex = u32(m_records.size());
        for (ClusterGroupProto gp : leaf.groups) {
            gp.unit = unitIndex;
            m_groupProtos.push_back(gp);
        }
        for (ClusterMetaProto p : leaf.protos) {
            p.owningGroup += rec.firstGroup;
            if (p.refinedGroup != UINT32_MAX)
                p.refinedGroup += rec.firstGroup;
            m_protos.push_back(p);
        }

        m_bakedIndices.insert(m_bakedIndices.end(), leaf.indices.begin(), leaf.indices.end());
        m_records.push_back(rec);
        m_stats.bakedMeshes += 1;
        m_stats.exactLeafFallbacks++;
        m_payloadStats.reclusterSplits += leaf.reclusterSplits;
    }

    BuildLookup();

    m_stats.clusters = u32(m_protos.size());
    m_stats.bakedIndexCount = m_bakedIndices.size();
    m_stats.bakeMs = u32(timer.GetElapsed_ms());

    const bool consistent = RunDiagnostics();

    Msg("* [ClusterDAG] baked %u/%u meshes: %u clusters, %llu indices, %u ms (%u invalid ranges, %u bake failures, %u small standalone, %u orphan standalone)",
        m_stats.bakedMeshes, m_stats.eligibleMeshes, m_stats.clusters,
        (unsigned long long)m_stats.bakedIndexCount, m_stats.bakeMs,
        m_stats.invalidRanges, m_stats.bakeFailed, m_stats.smallStandalone, m_stats.orphanStandalone);
    Msg("* [ClusterDAG] %u components (%u members, %u splits, %u orphans attached, %u unit pins, %u member pins), %u self-loops dropped, %u holes",
        m_stats.components, m_stats.componentMembers, m_stats.capSplits,
        m_stats.orphansAttached, m_stats.pinnedVerts, m_stats.memberPinnedVerts,
        m_stats.droppedSelfLoops, m_stats.holes);
    Msg("* [ClusterDAG] terrain: %u meshes, %u components, %u clusters",
        m_stats.terrainMeshes, m_stats.terrainComponents, m_stats.terrainClusters);
    Msg("* [ClusterDAG] simplify: %u groups (%u stalled), deviation raised %u (max +%.3f m)",
        m_stats.simplifiedGroups, m_stats.stalledGroups,
        m_stats.deviationRaised, m_stats.deviationMaxRaise);
    if (!consistent)
        Msg("! [ClusterDAG] inconsistent representation: %u invalid protos, %u error inversions, %u lod-bound violations",
            m_stats.invalidProtos, m_stats.errorInversions, m_stats.lodViolations);
    if (m_stats.metricRejectedGroups || m_stats.memberSplitViolations)
        Msg("! [ClusterDAG] rejected units: %u appearance-metric refusals, %u member-split triangles",
            m_stats.metricRejectedGroups, m_stats.memberSplitViolations);
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
        if (hdr.payloadVersion != CLUSTER_PAYLOAD_VERSION)
            break;
        if (hdr.maxVertices != CLUSTER_MAX_VERTICES || hdr.maxTriangles != CLUSTER_MAX_TRIANGLES)
            break;
        if (hdr.paramsHash != ComputeParamsHash())
            break;
        if (hdr.geomStamp != geomStamp)
            break;
        if (hdr.rangeSetHash != ComputeRangeSetHash(ranges))
            break;
        if (hdr.recordCount == 0 || hdr.protoCount == 0 || hdr.indexCount == 0 || hdr.groupCount == 0)
            break;
        if (u64(hdr.recordCount) * sizeof(ClusterUnitRecord) > (1ull << 32) ||
            u64(hdr.memberCount) * sizeof(ClusterMeshKey) > (1ull << 32) ||
            u64(hdr.protoCount) * sizeof(ClusterMetaProto) > (1ull << 32) ||
            u64(hdr.groupCount) * sizeof(ClusterGroupProto) > (1ull << 32) ||
            hdr.indexCount > (1ull << 31))
            break;

        m_records.resize(hdr.recordCount);
        m_memberKeys.resize(hdr.memberCount);
        m_protos.resize(hdr.protoCount);
        m_groupProtos.resize(hdr.groupCount);
        m_bakedIndices.resize(size_t(hdr.indexCount));

        if (fread(m_records.data(), sizeof(ClusterUnitRecord), hdr.recordCount, f) != hdr.recordCount)
            break;
        if (fread(m_memberKeys.data(), sizeof(ClusterMeshKey), hdr.memberCount, f) != hdr.memberCount)
            break;
        if (fread(m_protos.data(), sizeof(ClusterMetaProto), hdr.protoCount, f) != hdr.protoCount)
            break;
        if (fread(m_groupProtos.data(), sizeof(ClusterGroupProto), hdr.groupCount, f) != hdr.groupCount)
            break;
        if (fread(m_bakedIndices.data(), sizeof(u32), size_t(hdr.indexCount), f) != size_t(hdr.indexCount))
            break;

        ok = true;
        for (u32 r = 0; r < hdr.recordCount && ok; ++r) {
            const ClusterUnitRecord& rec = m_records[r];
            if (u64(rec.firstMember) + rec.memberCount > hdr.memberCount ||
                u64(rec.firstProto) + rec.protoCount > hdr.protoCount ||
                u64(rec.firstGroup) + rec.groupCount > hdr.groupCount ||
                u64(rec.firstIndex) + rec.indexTotal > hdr.indexCount ||
                rec.memberCount == 0 || rec.protoCount == 0 || rec.groupCount == 0) {
                ok = false;
                break;
            }
            for (u32 g = 0; g < rec.groupCount; ++g) {
                if (m_groupProtos[rec.firstGroup + g].unit != r) {
                    ok = false;
                    break;
                }
            }
        }
        for (u32 i = 0; ok && i < hdr.indexCount; ++i) {
            if (m_bakedIndices[size_t(i)] == UINT32_MAX)
                ok = false;
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
        const bool terrain = rec.protoCount > 0 &&
            (m_protos[rec.firstProto].flags & CLUSTER_PROTO_FLAG_TERRAIN);
        if (terrain)
            m_stats.terrainMeshes += rec.memberCount;
        if (rec.isComponent) {
            m_stats.components++;
            m_stats.componentMembers += rec.memberCount;
            if (terrain)
                m_stats.terrainComponents++;
        }
    }
    m_stats.eligibleMeshes = m_stats.bakedMeshes;
    m_stats.clusters = u32(m_protos.size());
    m_stats.bakedIndexCount = m_bakedIndices.size();

    if (!RunDiagnostics()) {
        Msg("! [ClusterDAG] cache rejected: %u invalid protos, %u error inversions, %u lod-bound violations",
            m_stats.invalidProtos, m_stats.errorInversions, m_stats.lodViolations);
        Clear();
        return false;
    }

    m_stats.bakeMs = u32(timer.GetElapsed_ms());

    Msg("* [ClusterDAG] cache hit: %u meshes, %u clusters, %llu indices, %u components, %u holes, %u ms",
        m_stats.bakedMeshes, m_stats.clusters, (unsigned long long)m_stats.bakedIndexCount,
        m_stats.components, m_stats.holes, m_stats.bakeMs);
    return true;
}

void ClusterDAG::SaveCache(const char* path, u64 geomStamp, const xr_vector<ClusterBakeRange>& ranges) const
{
    if (!path || !path[0] || m_records.empty() || m_bakedIndices.empty() || m_groupProtos.empty())
        return;

    if (m_stats.invalidProtos || m_stats.errorInversions || m_stats.lodViolations ||
        m_stats.metricRejectedGroups || m_stats.memberSplitViolations) {
        Msg("! [ClusterDAG] cache not saved: representation failed validation");
        return;
    }

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
    hdr.groupCount = u32(m_groupProtos.size());
    hdr.indexCount = m_bakedIndices.size();
    hdr.payloadVersion = CLUSTER_PAYLOAD_VERSION;
    hdr.maxVertices = CLUSTER_MAX_VERTICES;
    hdr.maxTriangles = CLUSTER_MAX_TRIANGLES;

    w->w(&hdr, sizeof(hdr));
    w->w(m_records.data(), u32(m_records.size() * sizeof(ClusterUnitRecord)));
    w->w(m_memberKeys.data(), u32(m_memberKeys.size() * sizeof(ClusterMeshKey)));
    w->w(m_protos.data(), u32(m_protos.size() * sizeof(ClusterMetaProto)));
    w->w(m_groupProtos.data(), u32(m_groupProtos.size() * sizeof(ClusterGroupProto)));
    w->w(m_bakedIndices.data(), u32(m_bakedIndices.size() * sizeof(u32)));
    FS.w_close(w);

    Msg("* [ClusterDAG] cache saved: %s", path);
}

bool ClusterDAG::RunDiagnostics()
{
    bool valid = true;
    xr_vector<ClusterBoundsKey> selfKeys;

    for (const ClusterUnitRecord& rec : m_records) {
        if (rec.protoCount == 0 || rec.memberCount == 0 || rec.groupCount == 0 ||
            u64(rec.firstProto) + rec.protoCount > m_protos.size() ||
            u64(rec.firstMember) + rec.memberCount > m_memberKeys.size() ||
            u64(rec.firstGroup) + rec.groupCount > m_groupProtos.size() ||
            u64(rec.firstIndex) + rec.indexTotal > m_bakedIndices.size()) {
            m_stats.invalidProtos++;
            valid = false;
            continue;
        }

        selfKeys.clear();
        selfKeys.reserve(rec.protoCount);

        for (u32 i = 0; i < rec.protoCount; ++i) {
            const ClusterMetaProto& p = m_protos[rec.firstProto + i];
            {
                ClusterBoundsKey k;
                k.x = p.lodSelf[0]; k.y = p.lodSelf[1]; k.z = p.lodSelf[2];
                k.r = p.lodSelf[3]; k.e = p.selfError;
                selfKeys.push_back(k);
            }

            const u32 depth = std::min(p.depth, 15u);
            m_stats.levelCounts[depth]++;
            m_stats.maxDepth = std::max(m_stats.maxDepth, p.depth);
            if (p.flags & CLUSTER_PROTO_FLAG_TERRAIN)
                m_stats.terrainClusters++;

            if (!(p.parentError < kErrorCap)) m_stats.histInf++;
            else if (p.parentError > 100.0f) m_stats.hist100++;
            else if (p.parentError > 10.0f) m_stats.hist10++;
            else if (p.parentError > 1.0f) m_stats.hist1++;
            else if (p.parentError > 0.1f) m_stats.hist01++;
            else m_stats.histSmall++;

            if (!ProtoStructureValid(p, rec)) {
                m_stats.invalidProtos++;
                valid = false;
                continue;
            }

            if (p.selfError > p.parentError + DiagTolerance(std::max(p.selfError, p.parentError))) {
                m_stats.errorInversions++;
                valid = false;
            }

            if (!SphereContains(p.lodParent, p.lodSelf)) {
                m_stats.lodViolations++;
                valid = false;
            }
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

    return valid;
}

void ClusterDAG::BuildAssetMember(u32 recordIndex, u32 member)
{
    const ClusterUnitRecord& rec = m_records[recordIndex];
    xr_vector<ClusterBvhPrim> prims;
    xr_vector<GPUClusterMeta> scratch;
    ClusterAssetBvh bvh;

    const u32 assetMember = rec.firstMember + member;
    if (m_assetMembers.size() <= assetMember)
    {
        m_assetMembers.resize(assetMember + 1);
        m_assetMemberRecord.resize(assetMember + 1, UINT32_MAX);
    }
    GPUClusterAssetMember& am = m_assetMembers[assetMember];
    m_assetMemberRecord[assetMember] = recordIndex;

    am.firstCluster = u32(m_clusterMeta.size());
    am.clusterCount = 0;
    am.firstNode = 0;
    am.nodeCount = 0;
    am.firstPage = 0;

    prims.clear();
    Fvector lo, hi;
    lo.set(flt_max, flt_max, flt_max);
    hi.set(-flt_max, -flt_max, -flt_max);
    bool strict = true;

    for (u32 p = 0; p < rec.protoCount; ++p) {
        const ClusterMetaProto& proto = m_protos[rec.firstProto + p];
        if (proto.member != member)
            continue;

        GPUClusterMeta meta;
        for (u32 c = 0; c < 4; ++c) {
            meta.sphere[c] = proto.sphere[c];
            meta.lodSelf[c] = proto.lodSelf[c];
            meta.lodParent[c] = proto.lodParent[c];
        }
        meta.extent[0] = proto.extent[0];
        meta.extent[1] = proto.extent[1];
        meta.extent[2] = proto.extent[2];
        meta.selfError = proto.selfError;
        meta.parentError = (proto.parentError < kErrorCap) ? proto.parentError : kErrorCap;
        meta.page = 0;
        meta.payloadOffset = 0;
        meta.flags = ((proto.flags & CLUSTER_PROTO_FLAG_AT) ? CLUSTER_META_FLAG_AT : 0u)
            | ((proto.flags & CLUSTER_PROTO_FLAG_TERRAIN) ? CLUSTER_META_FLAG_TERRAIN : 0u)
            | (std::min(proto.depth, 15u) << CLUSTER_META_DEPTH_SHIFT)
            | (((proto.indexCount / 3u) - 1u) << CLUSTER_META_TRIANGLE_SHIFT);

        const u32 clusterIndex = u32(m_clusterMeta.size());
        m_clusterMeta.push_back(meta);
        m_clusterProto.push_back(rec.firstProto + p);
        m_clusterOwningGroup.push_back(proto.owningGroup);
        m_clusterRefinedGroup.push_back(proto.refinedGroup);
        ++am.clusterCount;

        ClusterBvhPrim prim;
        const Fvector c = Fvector().set(meta.sphere[0], meta.sphere[1], meta.sphere[2]);
        const Fvector h = Fvector().set(meta.extent[0], meta.extent[1], meta.extent[2]);
        prim.lo.sub(c, h);
        prim.hi.add(c, h);
        prim.centroid = c;
        prim.lodCenter.set(meta.lodParent[0], meta.lodParent[1], meta.lodParent[2]);
        prim.lodRadius = meta.lodParent[3];
        ClusterMergeSphere(prim.lodCenter, prim.lodRadius,
            Fvector().set(meta.lodSelf[0], meta.lodSelf[1], meta.lodSelf[2]), meta.lodSelf[3]);
        prim.selfError = meta.selfError;
        prim.parentError = meta.parentError;
        prim.key = (meta.flags >> 8) & 0xFu;
        prim.payload = clusterIndex;
        prims.push_back(prim);

        lo.min(prim.lo);
        hi.max(prim.hi);
        if (!(meta.flags & CLUSTER_META_FLAG_AT))
            strict = false;
    }

    if (am.clusterCount == 0) {
        am.sphere[0] = am.sphere[1] = am.sphere[2] = am.sphere[3] = 0.0f;
        am.extent[0] = am.extent[1] = am.extent[2] = 0.0f;
        return;
    }

    Fvector center;
    center.add(lo, hi).mul(0.5f);
    Fvector extent;
    extent.sub(hi, lo).mul(0.5f);
    float radius = 0.0f;
    for (const ClusterBvhPrim& prim : prims) {
        const Fvector pc = Fvector().set(
            (prim.lo.x + prim.hi.x) * 0.5f,
            (prim.lo.y + prim.hi.y) * 0.5f,
            (prim.lo.z + prim.hi.z) * 0.5f);
        const Fvector ph = Fvector().set(
            (prim.hi.x - prim.lo.x) * 0.5f,
            (prim.hi.y - prim.lo.y) * 0.5f,
            (prim.hi.z - prim.lo.z) * 0.5f);
        radius = std::max(radius, center.distance_to(pc) + ph.magnitude());
    }
    am.sphere[0] = center.x;
    am.sphere[1] = center.y;
    am.sphere[2] = center.z;
    am.sphere[3] = radius;
    am.extent[0] = extent.x;
    am.extent[1] = extent.y;
    am.extent[2] = extent.z;

    if (!BuildClusterAssetBVH8(prims.data(), u32(prims.size()), strict, bvh))
        FATAL_F("[ClusterDAG] asset hierarchy construction failed for member %u (%u clusters)",
            assetMember, am.clusterCount);

    am.firstNode = u32(m_assetNodes.size());
    am.nodeCount = u32(bvh.nodes.size());
    if (u64(m_assetNodes.size()) + bvh.nodes.size() > UINT32_MAX)
        FATAL("[ClusterDAG] asset hierarchy node count exceeds the addressable range");

    const u32 clusterBase = am.firstCluster;
    for (GPUClusterAssetNode node : bvh.nodes) {
        if (node.flags & CLUSTER_ASSET_NODE_LEAF)
            node.child += clusterBase;
        else
            node.child += am.firstNode;
        m_assetNodes.push_back(node);
    }

    scratch.clear();
    scratch.reserve(bvh.order.size());
    for (u32 src : bvh.order)
        scratch.push_back(m_clusterMeta[src]);
    for (u32 i = 0; i < u32(scratch.size()); ++i)
        m_clusterMeta[clusterBase + i] = scratch[i];

    xr_vector<u32> sideScratch;
    sideScratch.reserve(bvh.order.size());
    auto permute = [&](xr_vector<u32>& table) {
        sideScratch.clear();
        for (u32 src : bvh.order)
            sideScratch.push_back(table[src]);
        for (u32 i = 0; i < u32(sideScratch.size()); ++i)
            table[clusterBase + i] = sideScratch[i];
    };
    permute(m_clusterProto);
    permute(m_clusterOwningGroup);
    permute(m_clusterRefinedGroup);

    m_maxAssetNodeDepth = std::max(m_maxAssetNodeDepth, bvh.maxDepth);
}

namespace
{

u32 AlignUp(u32 value, u32 alignment)
{
    return (value + alignment - 1u) & ~(alignment - 1u);
}

u32 ClusterPayloadSize(u32 vertexCount, u32 triangleCount)
{
    return AlignUp(vertexCount * 2u, 4u) + AlignUp(triangleCount * 3u, 4u) + CLUSTER_PAYLOAD_TAIL_PAD;
}

void StoreBaseVertex(u8* dst, const bindless::UnifiedVertex& v, const Fvector3* floatNormal, u32 format)
{
    memcpy(dst + 0, &v.position, 12);
    if (format == CLUSTER_VERTEX_FLOAT_NORMAL) {
        if (!floatNormal)
            FATAL("[ClusterDAG] float-normal payload has no exact source normal");
        memcpy(dst + 12, floatNormal, 12);
    } else {
        memcpy(dst + 12, &v.normal, 4);
        memcpy(dst + 16, &v.tangent, 4);
        memcpy(dst + 20, &v.binormal, 4);
    }
    memcpy(dst + 24, &v.texcoord0, 8);
}

bool NonDefaultUV1(const bindless::UnifiedVertex& v)
{
    u32 words[2];
    memcpy(words, &v.texcoord1, sizeof(words));
    return words[0] != 0u || words[1] != 0u;
}

} // namespace

void ClusterDAG::PageBuilder::Flush()
{
    if (pageIndex == UINT32_MAX)
        return;

    const u32 vertexCount = u32(pageSourceVertices.size());
    u32 attributeMask = 0;
    for (u32 v = 0; v < vertexCount; ++v) {
        const bindless::UnifiedVertex& src = SourceVertex(pageSourceVertices[v]);
        if (NonDefaultUV1(src))
            attributeMask |= CLUSTER_PAGE_ATTR_UV1;
        if (src.color != bindless::VertexConverter::DEFAULT_COLOR)
            attributeMask |= CLUSTER_PAGE_ATTR_COLOR;
        if (src.flags != 0u)
            attributeMask |= CLUSTER_PAGE_ATTR_FLAGS;
    }

    u32 vertexBytes = vertexCount * CLUSTER_PAGE_VERTEX_STRIDE;
    if (attributeMask & CLUSTER_PAGE_ATTR_UV1)
        vertexBytes += vertexCount * CLUSTER_PAGE_UV1_STRIDE;
    if (attributeMask & CLUSTER_PAGE_ATTR_COLOR)
        vertexBytes += vertexCount * CLUSTER_PAGE_COLOR_STRIDE;
    if (attributeMask & CLUSTER_PAGE_ATTR_FLAGS)
        vertexBytes += vertexCount * CLUSTER_PAGE_FLAGS_STRIDE;

    xr_vector<u8>& vertexData = *vertexArena;
    xr_vector<u8>& payloadData = *payloadArena;
    const u64 vertexBase = vertexData.size();
    const u64 payloadBase = payloadData.size();
    if (vertexBase + AlignUp(vertexBytes, CLUSTER_PAGE_ALIGNMENT) > UINT32_MAX
        || payloadBase + pagePayload.size() + CLUSTER_PAGE_ALIGNMENT > UINT32_MAX)
        FATAL("[ClusterDAG] compact geometry arena exceeds the addressable range");

    vertexData.resize(size_t(vertexBase) + AlignUp(vertexBytes, CLUSTER_PAGE_ALIGNMENT), 0);
    u8* base = vertexData.data() + vertexBase;
    for (u32 v = 0; v < vertexCount; ++v) {
        const u32 absoluteIndex = pageSourceVertices[v];
        StoreBaseVertex(base + size_t(v) * CLUSTER_PAGE_VERTEX_STRIDE,
            SourceVertex(absoluteIndex), SourceNormal(absoluteIndex), pageFormat);
    }

    u32 cursor = vertexCount * CLUSTER_PAGE_VERTEX_STRIDE;
    if (attributeMask & CLUSTER_PAGE_ATTR_UV1) {
        for (u32 v = 0; v < vertexCount; ++v)
            memcpy(base + cursor + size_t(v) * CLUSTER_PAGE_UV1_STRIDE,
                &SourceVertex(pageSourceVertices[v]).texcoord1, CLUSTER_PAGE_UV1_STRIDE);
        cursor += vertexCount * CLUSTER_PAGE_UV1_STRIDE;
    }
    if (attributeMask & CLUSTER_PAGE_ATTR_COLOR) {
        for (u32 v = 0; v < vertexCount; ++v)
            memcpy(base + cursor + size_t(v) * CLUSTER_PAGE_COLOR_STRIDE,
                &SourceVertex(pageSourceVertices[v]).color, CLUSTER_PAGE_COLOR_STRIDE);
        cursor += vertexCount * CLUSTER_PAGE_COLOR_STRIDE;
    }
    if (attributeMask & CLUSTER_PAGE_ATTR_FLAGS) {
        for (u32 v = 0; v < vertexCount; ++v)
            memcpy(base + cursor + size_t(v) * CLUSTER_PAGE_FLAGS_STRIDE,
                &SourceVertex(pageSourceVertices[v]).flags, CLUSTER_PAGE_FLAGS_STRIDE);
    }

    payloadData.insert(payloadData.end(), pagePayload.begin(), pagePayload.end());
    payloadData.resize(AlignUp(u32(payloadData.size()), CLUSTER_PAGE_ALIGNMENT), 0);

    GPUClusterPage& page = dag->m_pages[pageIndex];
    page.vertexBase = u32(vertexBase);
    page.payloadBase = u32(payloadBase);
    page.vertexCount = vertexCount;
    page.vertexBytes = vertexBytes;
    page.payloadBytes = u32(pagePayload.size());
    page.attributeMask = attributeMask;
    page.vertexFormat = pageFormat;
    page.assetMember = pageAssetMember;
    dag->m_pageClass[pageIndex] = u8(pageClass);

    ClusterPayloadStats& st = dag->m_payloadStats;
    st.pages++;
    st.maxPageVertices = std::max(st.maxPageVertices, vertexCount);
    st.pageVertexSlots += vertexCount;
    st.vertexBaseBytes += u64(vertexCount) * CLUSTER_PAGE_VERTEX_STRIDE;
    st.vertexOptionalBytes += vertexBytes - u64(vertexCount) * CLUSTER_PAGE_VERTEX_STRIDE;
    st.paddingBytes += AlignUp(vertexBytes, CLUSTER_PAGE_ALIGNMENT) - vertexBytes;
    if (pageFormat == CLUSTER_VERTEX_FLOAT_NORMAL)
        st.floatNormalPages++;
    if (attributeMask & CLUSTER_PAGE_ATTR_UV1)
        st.uv1Pages++;
    if (attributeMask & CLUSTER_PAGE_ATTR_COLOR)
        st.colorPages++;
    if (attributeMask & CLUSTER_PAGE_ATTR_FLAGS)
        st.flagPages++;

    pageIndex = UINT32_MAX;
    pageClusters = 0;
    pagePayloadBytes = 0;
    pageSourceVertices.clear();
    pagePayload.clear();
    pageSlotOf.clear();
}

void ClusterDAG::PageBuilder::Cook(u32 clusterIndex, u32 assetMember, u32 memberVertexBase,
    const u32* indices, u32 indexCount, u32 cookClass)
{
    const u32 triangleCount = indexCount / 3u;
    absolute.resize(indexCount);
    for (u32 i = 0; i < indexCount; ++i)
        absolute[i] = memberVertexBase + indices[i];

    clusterVertices.resize(indexCount);
    clusterCorners.resize(indexCount);
    const u32 vertexCount = u32(clodLocalIndices(clusterVertices.data(), clusterCorners.data(),
        absolute.data(), indexCount));

    if (vertexCount == 0 || vertexCount > CLUSTER_MAX_VERTICES || triangleCount == 0
        || triangleCount > CLUSTER_MAX_TRIANGLES)
        FATAL_F("[ClusterDAG] cluster %u has %u unique vertices and %u triangles, outside the %ux%u payload bound",
            clusterIndex, vertexCount, triangleCount, CLUSTER_MAX_VERTICES, CLUSTER_MAX_TRIANGLES);

    const bool floatBasis = (SourceVertex(absolute[0]).flags
        & bindless::UNIFIED_VERTEX_FLAG_FLOAT_BASIS) != 0;
    for (u32 v = 1; v < vertexCount; ++v) {
        const bool basis = (SourceVertex(clusterVertices[v]).flags
            & bindless::UNIFIED_VERTEX_FLAG_FLOAT_BASIS) != 0;
        if (basis != floatBasis)
            FATAL_F("[ClusterDAG] cluster %u mixes packed and float vertex bases", clusterIndex);
    }
    const u32 format = floatBasis ? CLUSTER_VERTEX_FLOAT_NORMAL : CLUSTER_VERTEX_PACKED_BASIS;
    const u32 payloadSize = ClusterPayloadSize(vertexCount, triangleCount);

    if (pageIndex != UINT32_MAX) {
        const bool wouldOverflow = pageAssetMember != assetMember
            || pageFormat != format
            || pageClass != cookClass
            || pageClusters >= CLUSTER_PAGE_MAX_CLUSTERS
            || pageSourceVertices.size() + vertexCount > CLUSTER_PAGE_MAX_VERTICES
            || pagePayloadBytes + payloadSize > CLUSTER_PAGE_MAX_PAYLOAD_BYTES;
        if (wouldOverflow)
            Flush();
    }

    if (pageIndex == UINT32_MAX) {
        pageIndex = u32(dag->m_pages.size());
        dag->m_pages.push_back(GPUClusterPage{});
        dag->m_pageClass.push_back(u8(cookClass));
        pageFormat = format;
        pageAssetMember = assetMember;
        pageClass = cookClass;
    }

    const u32 payloadOffset = pagePayloadBytes;
    pagePayload.resize(size_t(payloadOffset) + payloadSize, 0);
    u8* payload = pagePayload.data() + payloadOffset;

    for (u32 v = 0; v < vertexCount; ++v) {
        const u32 absoluteIndex = clusterVertices[v];
        auto it = pageSlotOf.find(absoluteIndex);
        u32 slot;
        if (it == pageSlotOf.end()) {
            slot = u32(pageSourceVertices.size());
            pageSourceVertices.push_back(absoluteIndex);
            pageSlotOf.emplace(absoluteIndex, slot);
        } else {
            slot = it->second;
        }
        const u16 packed = u16(slot);
        memcpy(payload + size_t(v) * 2u, &packed, sizeof(packed));
    }

    const u32 triangleBase = AlignUp(vertexCount * 2u, 4u);
    memcpy(payload + triangleBase, clusterCorners.data(), triangleCount * 3u);

    pagePayloadBytes += payloadSize;
    pageClusters++;

    GPUClusterMeta& meta = dag->m_clusterMeta[clusterIndex];
    meta.page = pageIndex;
    meta.payloadOffset = payloadOffset;
    meta.flags = (meta.flags & ~(CLUSTER_META_COUNT_MASK << CLUSTER_META_VERTEX_SHIFT))
        | ((vertexCount - 1u) << CLUSTER_META_VERTEX_SHIFT);

    ClusterPayloadStats& st = dag->m_payloadStats;
    st.clusters++;
    st.maxClusterVertices = std::max(st.maxClusterVertices, vertexCount);
    st.maxClusterTriangles = std::max(st.maxClusterTriangles, triangleCount);
    st.payloadBytes += payloadSize;
    st.remapBytes += u64(vertexCount) * 2ull;
    st.triangleBytes += u64(triangleCount) * 3ull;
    st.clusterVertexReferences += vertexCount;
    st.replacedIndexBytes += u64(indexCount) * sizeof(u32);
}

const bindless::UnifiedVertex& ClusterDAG::PageBuilder::SourceVertex(u32 absoluteIndex) const
{
    return source->vertices[absoluteIndex - source->vertexBase];
}

const Fvector3* ClusterDAG::PageBuilder::SourceNormal(u32 absoluteIndex) const
{
    return source->floatNormals ? source->floatNormals + (absoluteIndex - source->vertexBase) : nullptr;
}

void ClusterDAG::ClassifyGroups()
{
    const u32 groupCount = u32(m_groupProtos.size());
    m_groupPinned.assign(groupCount, 1u);
    if (groupCount == 0)
        return;

    for (u32 c = 0; c < u32(m_clusterMeta.size()); ++c) {
        const u32 refined = m_clusterRefinedGroup[c];
        if (refined == UINT32_MAX)
            continue;
        if (refined >= groupCount)
            FATAL_F("[ClusterDAG] cluster %u references refined group %u of %u", c, refined, groupCount);
        m_groupPinned[refined] = 0u;
    }
}

void ClusterDAG::CookPages(const ClusterSourceView& source, u32 firstMember, u32 firstCluster, bool runtime)
{

    PageBuilder builder;
    builder.dag = this;
    builder.source = &source;
    builder.vertexArena = runtime ? &m_runtimePageVertexData : &m_pageVertexData;
    builder.payloadArena = runtime ? &m_runtimePagePayloadData : &m_pagePayloadData;

    xr_vector<u8> seen;
    u64 distinct = 0;
    auto mark = [&](u32 absolute) {
        const size_t word = absolute >> 3;
        if (word >= seen.size())
            seen.resize(word + 1u, 0);
        const u8 bit = u8(1u << (absolute & 7u));
        if (!(seen[word] & bit)) {
            seen[word] |= bit;
            ++distinct;
        }
    };

    for (u32 am = firstMember; am < u32(m_assetMembers.size()); ++am) {
        GPUClusterAssetMember& member = m_assetMembers[am];
        if (member.clusterCount == 0) {
            member.firstPage = u32(m_pages.size());
            continue;
        }
        if (member.firstCluster < firstCluster)
            continue;

        const u32 recordIndex = m_assetMemberRecord[am];
        const ClusterUnitRecord& rec = m_records[recordIndex];
        const u32 memberVertexBase = rec.isComponent ? 0u : m_memberKeys[am].vertexOffset;

        builder.Flush();
        member.firstPage = u32(m_pages.size());

        for (u32 pass = 0; pass < 2u; ++pass) {
            for (u32 c = 0; c < member.clusterCount; ++c) {
                const u32 clusterIndex = member.firstCluster + c;
                const u32 owning = m_clusterOwningGroup[clusterIndex];
                const bool pinned = owning < u32(m_groupPinned.size()) && m_groupPinned[owning] != 0u;
                if ((pass == 0u) != pinned)
                    continue;
                const u32 cookClass = runtime ? CLUSTER_PAGE_CLASS_RUNTIME
                    : (pinned ? CLUSTER_PAGE_CLASS_ROOT : CLUSTER_PAGE_CLASS_FINE);
                const ClusterMetaProto& proto = m_protos[m_clusterProto[clusterIndex]];
                const u32* indices = m_bakedIndices.data() + rec.firstIndex + proto.ibFirst;
                builder.Cook(clusterIndex, am, memberVertexBase, indices, proto.indexCount, cookClass);
                for (u32 i = 0; i < proto.indexCount; ++i)
                    mark(memberVertexBase + indices[i]);
            }
            builder.Flush();
        }
    }

    builder.Flush();
    m_payloadStats.distinctSourceVertices += distinct;
    m_payloadStats.pageTableBytes = u64(m_pages.size()) * sizeof(GPUClusterPage);
}

void ClusterDAG::BuildResidencyStats()
{
    ClusterResidencyStats stats;
    for (u32 p = 0; p < u32(m_pages.size()); ++p) {
        const GPUClusterPage& page = m_pages[p];
        const u32 cls = p < u32(m_pageClass.size()) ? m_pageClass[p] : CLUSTER_PAGE_CLASS_ROOT;
        stats.maxPageVertexBytes = std::max(stats.maxPageVertexBytes, page.vertexBytes);
        stats.maxPagePayloadBytes = std::max(stats.maxPagePayloadBytes, page.payloadBytes);
        if (cls == CLUSTER_PAGE_CLASS_RUNTIME) {
            stats.runtimePages++;
            stats.runtimeVertexBytes += page.vertexBytes;
            stats.runtimePayloadBytes += page.payloadBytes;
        } else if (cls == CLUSTER_PAGE_CLASS_FINE) {
            stats.finePages++;
            stats.fineVertexBytes += page.vertexBytes;
            stats.finePayloadBytes += page.payloadBytes;
        } else {
            stats.rootPages++;
            stats.rootVertexBytes += page.vertexBytes;
            stats.rootPayloadBytes += page.payloadBytes;
        }
    }
    for (u8 pinned : m_groupPinned) {
        if (pinned)
            stats.pinnedGroups++;
        else
            stats.fineGroups++;
    }
    m_residencyStats = stats;
}

void ClusterDAG::BuildGroupRecords()
{
    m_groups.clear();
    m_groupMemberClusters.clear();
    m_groupReplacementClusters.clear();
    m_groupPages.clear();
    m_groupDependencies.clear();
    m_groupMemberPages.clear();
    m_payloadStats.multiPageGroups = 0;

    if (m_groupProtos.empty())
        return;

    const u32 groupCount = u32(m_groupProtos.size());
    m_groups.resize(groupCount);

    xr_vector<u32> memberCounts(groupCount, 0);
    xr_vector<u32> replacementCounts(groupCount, 0);
    for (u32 c = 0; c < u32(m_clusterMeta.size()); ++c) {
        const u32 owning = m_clusterOwningGroup[c];
        if (owning >= groupCount)
            FATAL_F("[ClusterDAG] cluster %u references owning group %u of %u", c, owning, groupCount);
        memberCounts[owning]++;
        const u32 refined = m_clusterRefinedGroup[c];
        if (refined != UINT32_MAX) {
            if (refined >= groupCount)
                FATAL_F("[ClusterDAG] cluster %u references refined group %u of %u", c, refined, groupCount);
            replacementCounts[refined]++;
        }
    }

    u32 memberCursor = 0;
    u32 replacementCursor = 0;
    for (u32 g = 0; g < groupCount; ++g) {
        const ClusterGroupProto& gp = m_groupProtos[g];
        ClusterGroupRecord& rec = m_groups[g];
        rec.sphere[0] = gp.sphere[0];
        rec.sphere[1] = gp.sphere[1];
        rec.sphere[2] = gp.sphere[2];
        rec.sphere[3] = gp.sphere[3];
        rec.error = gp.error;
        rec.depth = gp.depth;
        rec.flags = gp.flags;
        rec.firstMember = memberCursor;
        rec.memberCount = 0;
        rec.firstReplacement = replacementCursor;
        rec.replacementCount = 0;
        rec.firstPage = 0;
        rec.pageCount = 0;
        rec.firstDependency = 0;
        rec.dependencyCount = 0;
        rec.firstMemberPage = 0;
        rec.memberPageCount = 0;
        rec.pinned = g < u32(m_groupPinned.size()) ? m_groupPinned[g] : 1u;
        memberCursor += memberCounts[g];
        replacementCursor += replacementCounts[g];
    }

    m_groupMemberClusters.resize(memberCursor);
    m_groupReplacementClusters.resize(replacementCursor);

    for (u32 c = 0; c < u32(m_clusterMeta.size()); ++c) {
        ClusterGroupRecord& owning = m_groups[m_clusterOwningGroup[c]];
        m_groupMemberClusters[owning.firstMember + owning.memberCount++] = c;
        const u32 refined = m_clusterRefinedGroup[c];
        if (refined != UINT32_MAX) {
            ClusterGroupRecord& replaced = m_groups[refined];
            m_groupReplacementClusters[replaced.firstReplacement + replaced.replacementCount++] = c;
        }
    }

    xr_vector<u32> scratch;
    for (u32 g = 0; g < groupCount; ++g) {
        ClusterGroupRecord& rec = m_groups[g];

        scratch.clear();
        for (u32 i = 0; i < rec.memberCount; ++i)
            scratch.push_back(m_clusterMeta[m_groupMemberClusters[rec.firstMember + i]].page);
        for (u32 i = 0; i < rec.replacementCount; ++i)
            scratch.push_back(m_clusterMeta[m_groupReplacementClusters[rec.firstReplacement + i]].page);
        std::sort(scratch.begin(), scratch.end());
        scratch.erase(std::unique(scratch.begin(), scratch.end()), scratch.end());
        rec.firstPage = u32(m_groupPages.size());
        rec.pageCount = u32(scratch.size());
        m_groupPages.insert(m_groupPages.end(), scratch.begin(), scratch.end());
        if (rec.pageCount > 1)
            m_payloadStats.multiPageGroups++;

        scratch.clear();
        for (u32 i = 0; i < rec.memberCount; ++i)
            scratch.push_back(m_clusterMeta[m_groupMemberClusters[rec.firstMember + i]].page);
        std::sort(scratch.begin(), scratch.end());
        scratch.erase(std::unique(scratch.begin(), scratch.end()), scratch.end());
        rec.firstMemberPage = u32(m_groupMemberPages.size());
        rec.memberPageCount = u32(scratch.size());
        m_groupMemberPages.insert(m_groupMemberPages.end(), scratch.begin(), scratch.end());

        scratch.clear();
        for (u32 i = 0; i < rec.replacementCount; ++i)
            scratch.push_back(m_clusterOwningGroup[m_groupReplacementClusters[rec.firstReplacement + i]]);
        std::sort(scratch.begin(), scratch.end());
        scratch.erase(std::unique(scratch.begin(), scratch.end()), scratch.end());
        rec.firstDependency = u32(m_groupDependencies.size());
        rec.dependencyCount = u32(scratch.size());
        m_groupDependencies.insert(m_groupDependencies.end(), scratch.begin(), scratch.end());
    }

    m_payloadStats.groups = groupCount;
    m_payloadStats.groupDependencies = u32(m_groupDependencies.size());
    BuildResidencyStats();
}

void ClusterDAG::BuildAssetTable(const ClusterSourceView& source)
{
    m_clusterMeta.clear();
    m_assetMembers.clear();
    m_assetNodes.clear();
    m_assetMemberRecord.clear();
    m_clusterProto.clear();
    m_clusterOwningGroup.clear();
    m_clusterRefinedGroup.clear();
    m_pages.clear();
    m_pageVertexData.clear();
    m_pagePayloadData.clear();
    m_runtimePageVertexData.clear();
    m_runtimePagePayloadData.clear();
    m_pageClass.clear();
    m_groupPinned.clear();
    m_groupMemberPages.clear();
    m_storePageCount = 0;
    m_storeIdentity = 0;
    m_maxAssetNodeDepth = 0;

    const u32 reclusterSplits = m_payloadStats.reclusterSplits;
    m_payloadStats = {};
    m_payloadStats.reclusterSplits = reclusterSplits;

    if (m_records.empty() || m_protos.empty())
        return;

    if (!source.vertices || !source.indices)
        FATAL("[ClusterDAG] compact geometry cooking needs the retained source vertices");

    m_clusterMeta.reserve(m_protos.size());
    m_clusterProto.reserve(m_protos.size());
    m_clusterOwningGroup.reserve(m_protos.size());
    m_clusterRefinedGroup.reserve(m_protos.size());
    m_assetMembers.resize(m_memberKeys.size());
    m_assetMemberRecord.resize(m_memberKeys.size(), UINT32_MAX);

    for (u32 r = 0; r < u32(m_records.size()); ++r)
    {
        const u32 memberCount = m_records[r].memberCount;
        for (u32 m = 0; m < memberCount; ++m)
            BuildAssetMember(r, m);
    }

    ClassifyGroups();

    CookPages(source, 0, 0, false);
    BuildGroupRecords();

    const ClusterPayloadStats& st = m_payloadStats;
    const u64 payloadPadding = st.payloadBytes - st.remapBytes - st.triangleBytes;
    Msg("* [ClusterDAG] asset table: %u members, %u clusters, %u hierarchy nodes, depth %u (%.2f MB metadata)",
        u32(m_assetMembers.size()), u32(m_clusterMeta.size()), u32(m_assetNodes.size()), m_maxAssetNodeDepth,
        (m_clusterMeta.size() * sizeof(GPUClusterMeta) + m_assetNodes.size() * sizeof(GPUClusterAssetNode)
            + m_assetMembers.size() * sizeof(GPUClusterAssetMember) + st.pageTableBytes) / (1024.0f * 1024.0f));
    Msg("* [ClusterDAG] compact payload v%u: %u pages, %u clusters, max %u verts/page, %u verts and %u tris/cluster, %u recluster splits",
        CLUSTER_PAYLOAD_VERSION, st.pages, st.clusters, st.maxPageVertices,
        st.maxClusterVertices, st.maxClusterTriangles, st.reclusterSplits);
    Msg("* [ClusterDAG] payload bytes: %.2f MB total (%.2f remap + %.2f triangles + %.2f padding), page table %.2f MB, replaces %.2f MB of cluster indices",
        st.payloadBytes / (1024.0f * 1024.0f), st.remapBytes / (1024.0f * 1024.0f),
        st.triangleBytes / (1024.0f * 1024.0f), payloadPadding / (1024.0f * 1024.0f),
        st.pageTableBytes / (1024.0f * 1024.0f), st.replacedIndexBytes / (1024.0f * 1024.0f));
    Msg("* [ClusterDAG] page vertices: %llu slots for %llu distinct source vertices (%llu cluster references), %.2f MB base + %.2f MB optional + %.2f MB alignment",
        (unsigned long long)st.pageVertexSlots, (unsigned long long)st.distinctSourceVertices,
        (unsigned long long)st.clusterVertexReferences,
        st.vertexBaseBytes / (1024.0f * 1024.0f), st.vertexOptionalBytes / (1024.0f * 1024.0f),
        st.paddingBytes / (1024.0f * 1024.0f));
    Msg("* [ClusterDAG] page attributes: %u float-normal, %u uv1, %u color, %u flag pages; %u groups, %u dependency links, %u spanning several pages",
        st.floatNormalPages, st.uv1Pages, st.colorPages, st.flagPages,
        st.groups, st.groupDependencies, st.multiPageGroups);

    m_bakedIndices.clear();
    m_bakedIndices.shrink_to_fit();
}

bool ClusterDAG::AppendRuntimeLeafAsset(
    const ClusterMeshKey& key,
    u32 rangeFlags,
    const ClusterSourceView& source,
    u32& outAssetMember)
{
    if (!source.vertices || !source.indices || key.vertexCount == 0
        || key.indexCount < 3 || key.indexCount % 3 != 0)
        return false;
    if (source.vertexBase != key.vertexOffset)
        return false;
    for (u32 i = 0; i < key.indexCount; ++i)
    {
        if (source.indices[i] >= key.vertexCount)
            return false;
    }

    const bool floatBasis = (source.vertices[0].flags & bindless::UNIFIED_VERTEX_FLAG_FLOAT_BASIS) != 0;
    if (floatBasis && !source.floatNormals)
        return false;
    for (u32 v = 1; v < key.vertexCount; ++v)
    {
        if (((source.vertices[v].flags & bindless::UNIFIED_VERTEX_FLAG_FLOAT_BASIS) != 0) != floatBasis)
            return false;
    }

    ClusterMeshKey local;
    local.vertexOffset = 0;
    local.indexOffset = 0;
    local.vertexCount = key.vertexCount;
    local.indexCount = key.indexCount;

    BakeResult leaf;
    BakeLeafOnly(local, rangeFlags, source.vertices, source.floatNormals, source.indices, leaf);
    if (!leaf.baked)
        return false;

    const u32 recordIndex = u32(m_records.size());
    const u32 firstMember = u32(m_memberKeys.size());
    const u32 firstCluster = u32(m_clusterMeta.size());

    ClusterUnitRecord rec;
    rec.firstMember = firstMember;
    rec.memberCount = 1;
    rec.firstProto = u32(m_protos.size());
    rec.protoCount = u32(leaf.protos.size());
    rec.firstIndex = u32(m_bakedIndices.size());
    rec.indexTotal = u32(leaf.indices.size());
    rec.firstGroup = u32(m_groupProtos.size());
    rec.groupCount = u32(leaf.groups.size());
    rec.isComponent = 0;

    m_memberKeys.push_back(key);
    for (ClusterGroupProto gp : leaf.groups) {
        gp.unit = recordIndex;
        m_groupProtos.push_back(gp);
    }
    for (ClusterMetaProto p : leaf.protos) {
        p.owningGroup += rec.firstGroup;
        if (p.refinedGroup != UINT32_MAX)
            p.refinedGroup += rec.firstGroup;
        m_protos.push_back(p);
    }
    m_bakedIndices.insert(m_bakedIndices.end(), leaf.indices.begin(), leaf.indices.end());
    m_records.push_back(rec);

    BuildLookup();
    BuildAssetMember(recordIndex, 0);
    ClassifyGroups();
    CookPages(source, firstMember, firstCluster, true);
    BuildGroupRecords();

    m_bakedIndices.resize(rec.firstIndex);
    m_bakedIndices.shrink_to_fit();

    outAssetMember = firstMember;
    m_stats.runtimeLeafAssets++;
    m_stats.clusters = u32(m_protos.size());
    m_payloadStats.reclusterSplits += leaf.reclusterSplits;
    return true;
}

bool ClusterDAG::WritePageStore(const char* path, u64 geomStamp, const xr_vector<ClusterBakeRange>& ranges)
{
    m_storePageCount = 0;
    m_storeIdentity = 0;
    m_storeVertexOrigin = 0;
    m_storePayloadOrigin = 0;
    m_storeVertexBytes = 0;
    m_storePayloadBytes = 0;

    if (!path || !path[0] || m_pages.empty())
        return false;

    u32 storePages = 0;
    for (u32 p = 0; p < u32(m_pages.size()); ++p) {
        if (p < u32(m_pageClass.size()) && m_pageClass[p] == CLUSTER_PAGE_CLASS_RUNTIME)
            continue;
        storePages = p + 1u;
    }
    if (storePages == 0)
        return false;

    ClusterPageStoreHeader header = {};
    header.magic = CLUSTER_PAGE_STORE_MAGIC;
    header.version = CLUSTER_PAGE_STORE_VERSION;
    header.payloadVersion = CLUSTER_PAYLOAD_VERSION;
    header.pageCount = storePages;
    header.paramsHash = ComputeParamsHash();
    header.geomStamp = geomStamp;
    header.rangeSetHash = ComputeRangeSetHash(ranges);
    header.vertexBytes = m_pageVertexData.size();
    header.payloadBytes = m_pagePayloadData.size();
    header.maxClusterVertices = CLUSTER_MAX_VERTICES;
    header.maxClusterTriangles = CLUSTER_MAX_TRIANGLES;
    header.vertexStride = CLUSTER_PAGE_VERTEX_STRIDE;
    header.pageAlignment = CLUSTER_PAGE_ALIGNMENT;
    header.maxPageVertices = CLUSTER_PAGE_MAX_VERTICES;
    header.maxPageClusters = CLUSTER_PAGE_MAX_CLUSTERS;
    header.maxPagePayloadBytes = CLUSTER_PAGE_MAX_PAYLOAD_BYTES;
    header.reserved = 0;

    u64 layout = Fnv1a64(m_pages.data(), size_t(storePages) * sizeof(GPUClusterPage));
    layout = Fnv1a64(m_pageClass.data(), std::min<size_t>(storePages, m_pageClass.size()), layout);
    layout = Fnv1a64(&header.vertexBytes, sizeof(header.vertexBytes), layout);
    layout = Fnv1a64(&header.payloadBytes, sizeof(header.payloadBytes), layout);
    header.layoutHash = layout;

    const u64 tableBytes = u64(storePages) * sizeof(GPUClusterPage);
    const u64 classBytes = AlignUp(u32(storePages), CLUSTER_PAGE_ALIGNMENT);
    m_storeVertexOrigin = sizeof(ClusterPageStoreHeader) + tableBytes + classBytes;
    m_storePayloadOrigin = m_storeVertexOrigin + header.vertexBytes;
    m_storePageCount = storePages;
    m_storeVertexBytes = header.vertexBytes;
    m_storePayloadBytes = header.payloadBytes;
    m_storeIdentity = Fnv1a64(&header.layoutHash, sizeof(header.layoutHash),
        Fnv1a64(&header.paramsHash, sizeof(header.paramsHash),
            Fnv1a64(&header.geomStamp, sizeof(header.geomStamp),
                Fnv1a64(&header.rangeSetHash, sizeof(header.rangeSetHash)))));

    bool reusable = false;
    if (FILE* existing = fopen(path, "rb")) {
        ClusterPageStoreHeader onDisk = {};
        if (fread(&onDisk, sizeof(onDisk), 1, existing) == 1) {
            reusable = onDisk.magic == header.magic && onDisk.version == header.version
                && onDisk.payloadVersion == header.payloadVersion
                && onDisk.pageCount == header.pageCount
                && onDisk.paramsHash == header.paramsHash
                && onDisk.geomStamp == header.geomStamp
                && onDisk.rangeSetHash == header.rangeSetHash
                && onDisk.layoutHash == header.layoutHash
                && onDisk.vertexBytes == header.vertexBytes
                && onDisk.payloadBytes == header.payloadBytes
                && onDisk.maxClusterVertices == header.maxClusterVertices
                && onDisk.maxClusterTriangles == header.maxClusterTriangles
                && onDisk.vertexStride == header.vertexStride
                && onDisk.pageAlignment == header.pageAlignment
                && onDisk.maxPageVertices == header.maxPageVertices
                && onDisk.maxPageClusters == header.maxPageClusters
                && onDisk.maxPagePayloadBytes == header.maxPagePayloadBytes;
        }
        if (reusable) {
            fseek(existing, 0, SEEK_END);
            const long size = ftell(existing);
            if (size < 0 || u64(size) < m_storePayloadOrigin + header.payloadBytes)
                reusable = false;
        }
        fclose(existing);
    }

    if (reusable) {
        Msg("* [ClusterDAG] page store reused: %s (%u pages, %.2f MB vertices + %.2f MB payload)",
            path, storePages, header.vertexBytes / (1024.0f * 1024.0f),
            header.payloadBytes / (1024.0f * 1024.0f));
        return true;
    }

    IWriter* w = FS.w_open(path);
    if (!w) {
        Msg("! [ClusterDAG] failed to open the page store for write: %s", path);
        m_storePageCount = 0;
        return false;
    }

    w->w(&header, sizeof(header));
    w->w(m_pages.data(), size_t(tableBytes));
    xr_vector<u8> classes(size_t(classBytes), 0);
    memcpy(classes.data(), m_pageClass.data(), std::min<size_t>(storePages, m_pageClass.size()));
    w->w(classes.data(), classes.size());
    if (!m_pageVertexData.empty())
        w->w(m_pageVertexData.data(), m_pageVertexData.size());
    if (!m_pagePayloadData.empty())
        w->w(m_pagePayloadData.data(), m_pagePayloadData.size());
    FS.w_close(w);

    Msg("* [ClusterDAG] page store written: %s (%u pages, %.2f MB vertices + %.2f MB payload)",
        path, storePages, header.vertexBytes / (1024.0f * 1024.0f),
        header.payloadBytes / (1024.0f * 1024.0f));
    return true;
}

void ClusterDAG::ReleaseLevelPageBytes()
{
    m_pageVertexData.clear();
    m_pageVertexData.shrink_to_fit();
    m_pagePayloadData.clear();
    m_pagePayloadData.shrink_to_fit();
}

void ClusterDAG::ReleaseRuntimePageBytes()
{
    m_runtimePageVertexData.clear();
    m_runtimePageVertexData.shrink_to_fit();
    m_runtimePagePayloadData.clear();
    m_runtimePagePayloadData.shrink_to_fit();
}

} // namespace xray::render::fg
