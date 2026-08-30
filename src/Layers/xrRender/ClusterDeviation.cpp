#include "stdafx.h"
#include "ClusterDeviation.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace xray::render::fg
{

namespace
{

constexpr u32 kMaxAttrs = 8;
constexpr u32 kMaxSubdiv = 8;
constexpr int kMaxCellsPerAxis = 64;
constexpr float kCandidateTol = 1e-3f;
constexpr float kMinSampleStep = 0.05f;
constexpr float kSampleDivisor = 40.0f;

struct V3 {
    float x, y, z;
};

V3 Sub(const V3& a, const V3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 Add(const V3& a, const V3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V3 Mul(const V3& a, float s) { return {a.x * s, a.y * s, a.z * s}; }
float Dot(const V3& a, const V3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3 Cross(const V3& a, const V3& b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
V3 Min3(const V3& a, const V3& b) { return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)}; }
V3 Max3(const V3& a, const V3& b) { return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)}; }
float Axis(const V3& v, int a) { return a == 0 ? v.x : (a == 1 ? v.y : v.z); }

struct Tri {
    V3 v[3];
    float attr[3][kMaxAttrs];
};

struct TriSet {
    xr_vector<Tri> tris;
    V3 bmin;
    V3 bmax;
};

struct Grid {
    V3 origin;
    float cell;
    float invCell;
    int n[3];
    xr_vector<u32> starts;
    xr_vector<u32> items;
};

struct Candidate {
    float dist;
    u32 tri;
    float bary[3];
};

void BuildSet(TriSet& set,
    const float* positions, size_t positionStride,
    const float* attributes, size_t attributeStride,
    const float* weights, u32 attrCount,
    const u32* indices, size_t count)
{
    set.tris.clear();
    set.bmin = {FLT_MAX, FLT_MAX, FLT_MAX};
    set.bmax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (size_t i = 0; i + 2 < count; i += 3) {
        Tri t;
        for (int k = 0; k < 3; ++k) {
            const size_t vi = indices[i + k];
            const float* p = positions + vi * positionStride;
            t.v[k] = {p[0], p[1], p[2]};
            const float* a = attributes + vi * attributeStride;
            for (u32 j = 0; j < attrCount; ++j)
                t.attr[k][j] = a[j] * weights[j];
        }
        const V3 n = Cross(Sub(t.v[1], t.v[0]), Sub(t.v[2], t.v[0]));
        if (Dot(n, n) <= 1e-24f)
            continue;
        for (int k = 0; k < 3; ++k) {
            set.bmin = Min3(set.bmin, t.v[k]);
            set.bmax = Max3(set.bmax, t.v[k]);
        }
        set.tris.push_back(t);
    }
}

int CellCoord(const Grid& g, float p, int axis)
{
    const int c = int(std::floor((p - Axis(g.origin, axis)) * g.invCell));
    return std::max(0, std::min(g.n[axis] - 1, c));
}

void BuildGrid(Grid& g, const TriSet& set)
{
    const V3 size = Sub(set.bmax, set.bmin);
    const float extent = std::max(size.x, std::max(size.y, size.z));
    const int per = std::max(1, std::min(24, int(std::cbrt(float(set.tris.size())))));
    g.cell = std::max(extent / float(per), 1e-3f);
    g.invCell = 1.0f / g.cell;
    g.origin = set.bmin;
    for (int a = 0; a < 3; ++a)
        g.n[a] = std::max(1, std::min(kMaxCellsPerAxis, int(Axis(size, a) * g.invCell) + 1));

    const size_t total = size_t(g.n[0]) * g.n[1] * g.n[2];
    g.starts.assign(total + 1, 0);

    auto forEachCell = [&](const Tri& t, auto&& fn) {
        V3 lo = Min3(t.v[0], Min3(t.v[1], t.v[2]));
        V3 hi = Max3(t.v[0], Max3(t.v[1], t.v[2]));
        int c0[3], c1[3];
        for (int a = 0; a < 3; ++a) {
            c0[a] = CellCoord(g, Axis(lo, a), a);
            c1[a] = CellCoord(g, Axis(hi, a), a);
        }
        for (int z = c0[2]; z <= c1[2]; ++z)
            for (int y = c0[1]; y <= c1[1]; ++y)
                for (int x = c0[0]; x <= c1[0]; ++x)
                    fn(size_t(z) * g.n[1] * g.n[0] + size_t(y) * g.n[0] + x);
    };

    for (const Tri& t : set.tris)
        forEachCell(t, [&](size_t c) { g.starts[c + 1]++; });
    for (size_t c = 0; c < total; ++c)
        g.starts[c + 1] += g.starts[c];

    g.items.resize(g.starts[total]);
    xr_vector<u32> cursor(g.starts.begin(), g.starts.end() - 1);
    for (u32 i = 0; i < u32(set.tris.size()); ++i)
        forEachCell(set.tris[i], [&](size_t c) { g.items[cursor[c]++] = i; });
}

V3 ClosestPoint(const V3& p, const Tri& t, float bary[3])
{
    const V3 a = t.v[0], b = t.v[1], c = t.v[2];
    const V3 ab = Sub(b, a), ac = Sub(c, a), ap = Sub(p, a);
    const float d1 = Dot(ab, ap), d2 = Dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) {
        bary[0] = 1.0f; bary[1] = 0.0f; bary[2] = 0.0f;
        return a;
    }
    const V3 bp = Sub(p, b);
    const float d3 = Dot(ab, bp), d4 = Dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) {
        bary[0] = 0.0f; bary[1] = 1.0f; bary[2] = 0.0f;
        return b;
    }
    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        const float v = d1 / (d1 - d3);
        bary[0] = 1.0f - v; bary[1] = v; bary[2] = 0.0f;
        return Add(a, Mul(ab, v));
    }
    const V3 cp = Sub(p, c);
    const float d5 = Dot(ab, cp), d6 = Dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) {
        bary[0] = 0.0f; bary[1] = 0.0f; bary[2] = 1.0f;
        return c;
    }
    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        const float w = d2 / (d2 - d6);
        bary[0] = 1.0f - w; bary[1] = 0.0f; bary[2] = w;
        return Add(a, Mul(ac, w));
    }
    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        bary[0] = 0.0f; bary[1] = 1.0f - w; bary[2] = w;
        return Add(b, Mul(Sub(c, b), w));
    }
    const float denom = 1.0f / (va + vb + vc);
    const float v = vb * denom, w = vc * denom;
    bary[0] = 1.0f - v - w; bary[1] = v; bary[2] = w;
    return Add(a, Add(Mul(ab, v), Mul(ac, w)));
}

struct QueryScratch {
    xr_vector<Candidate> cands;
    xr_vector<u32> stamps;
    u32 stamp = 0;
};

float QuerySample(const Grid& g, const TriSet& target, const V3& p, const float* attr, u32 attrCount, QueryScratch& scratch)
{
    int c[3];
    for (int a = 0; a < 3; ++a)
        c[a] = CellCoord(g, Axis(p, a), a);

    scratch.cands.clear();
    if (scratch.stamps.size() != target.tris.size()) {
        scratch.stamps.assign(target.tris.size(), 0);
        scratch.stamp = 0;
    }
    scratch.stamp++;

    const int maxRing = std::max(g.n[0], std::max(g.n[1], g.n[2]));
    float best = FLT_MAX;

    for (int r = 0; r <= maxRing; ++r) {
        if (r > 0) {
            const float ringDist = float(r - 1) * g.cell;
            if (ringDist > best + kCandidateTol)
                break;
        }
        for (int dz = -r; dz <= r; ++dz) {
            const int z = c[2] + dz;
            if (z < 0 || z >= g.n[2])
                continue;
            for (int dy = -r; dy <= r; ++dy) {
                const int y = c[1] + dy;
                if (y < 0 || y >= g.n[1])
                    continue;
                const bool shellYZ = std::abs(dz) == r || std::abs(dy) == r;
                const int step = shellYZ ? 1 : 2 * r;
                for (int dx = -r; dx <= r; dx += std::max(1, step)) {
                    const int x = c[0] + dx;
                    if (x < 0 || x >= g.n[0])
                        continue;
                    const size_t cell = size_t(z) * g.n[1] * g.n[0] + size_t(y) * g.n[0] + x;
                    for (u32 it = g.starts[cell]; it < g.starts[cell + 1]; ++it) {
                        const u32 ti = g.items[it];
                        if (scratch.stamps[ti] == scratch.stamp)
                            continue;
                        scratch.stamps[ti] = scratch.stamp;
                        Candidate cand;
                        cand.tri = ti;
                        const V3 q = ClosestPoint(p, target.tris[ti], cand.bary);
                        const V3 d = Sub(p, q);
                        cand.dist = std::sqrt(Dot(d, d));
                        best = std::min(best, cand.dist);
                        scratch.cands.push_back(cand);
                    }
                    if (r == 0)
                        break;
                }
            }
        }
    }

    if (best == FLT_MAX)
        return 0.0f;

    const float limit = best + kCandidateTol;
    float bestAttr = FLT_MAX;
    for (const Candidate& cand : scratch.cands) {
        if (cand.dist > limit)
            continue;
        const Tri& t = target.tris[cand.tri];
        float sum = 0.0f;
        for (u32 j = 0; j < attrCount; ++j) {
            const float a = t.attr[0][j] * cand.bary[0] + t.attr[1][j] * cand.bary[1] + t.attr[2][j] * cand.bary[2];
            const float d = a - attr[j];
            sum += d * d;
        }
        bestAttr = std::min(bestAttr, sum);
    }
    return std::max(best, bestAttr == FLT_MAX ? 0.0f : std::sqrt(bestAttr));
}

float Deviate(const TriSet& query, const TriSet& target, const Grid& grid, float step, u32 attrCount, QueryScratch& scratch)
{
    float worst = 0.0f;
    for (const Tri& t : query.tris) {
        const V3 e0 = Sub(t.v[1], t.v[0]), e1 = Sub(t.v[2], t.v[1]), e2 = Sub(t.v[0], t.v[2]);
        const float maxEdge = std::sqrt(std::max(Dot(e0, e0), std::max(Dot(e1, e1), Dot(e2, e2))));
        const u32 n = std::max(1u, std::min(kMaxSubdiv, u32(std::ceil(maxEdge / step))));
        const float inv = 1.0f / float(n);
        for (u32 i = 0; i <= n; ++i) {
            for (u32 j = 0; i + j <= n; ++j) {
                const float u = float(i) * inv, v = float(j) * inv, w = 1.0f - u - v;
                const V3 p = Add(Mul(t.v[0], w), Add(Mul(t.v[1], u), Mul(t.v[2], v)));
                float attr[kMaxAttrs];
                for (u32 k = 0; k < attrCount; ++k)
                    attr[k] = t.attr[0][k] * w + t.attr[1][k] * u + t.attr[2][k] * v;
                worst = std::max(worst, QuerySample(grid, target, p, attr, attrCount, scratch));
            }
        }
    }
    return worst;
}

}

float ClusterMeshDeviation(
    const float* positions, size_t positionStride,
    const float* attributes, size_t attributeStride,
    const float* attributeWeights, u32 attributeCount,
    const u32* source, size_t sourceCount,
    const u32* result, size_t resultCount)
{
    static thread_local TriSet s_a;
    static thread_local TriSet s_b;
    static thread_local Grid s_ga;
    static thread_local Grid s_gb;
    static thread_local QueryScratch s_scratch;

    const u32 attrCount = std::min(attributeCount, kMaxAttrs);
    BuildSet(s_a, positions, positionStride, attributes, attributeStride, attributeWeights, attrCount, source, sourceCount);
    BuildSet(s_b, positions, positionStride, attributes, attributeStride, attributeWeights, attrCount, result, resultCount);
    if (s_a.tris.empty() || s_b.tris.empty())
        return 0.0f;

    BuildGrid(s_ga, s_a);
    BuildGrid(s_gb, s_b);

    const V3 size = Sub(Max3(s_a.bmax, s_b.bmax), Min3(s_a.bmin, s_b.bmin));
    const float extent = std::max(size.x, std::max(size.y, size.z));
    const float step = std::max(kMinSampleStep, extent / kSampleDivisor);

    const float ab = Deviate(s_a, s_b, s_gb, step, attrCount, s_scratch);
    const float ba = Deviate(s_b, s_a, s_ga, step, attrCount, s_scratch);
    return std::max(ab, ba);
}

}
