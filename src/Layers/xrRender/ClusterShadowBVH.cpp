#include "stdafx.h"
#include "ClusterShadowBVH.h"
#include "GPUCullingManager.h"

namespace xray::render::fg
{

namespace {

constexpr u32 kBinCount = 16;
constexpr float kErrorInf = 1e30f;

struct Prim {
    Fvector lo;
    Fvector hi;
    Fvector centroid;
    float selfError;
    float parentError;
    u32 entry;
};

struct Box {
    Fvector lo;
    Fvector hi;
    float minSelf;
    float maxParent;

    void reset()
    {
        lo.set(flt_max, flt_max, flt_max);
        hi.set(-flt_max, -flt_max, -flt_max);
        minSelf = kErrorInf;
        maxParent = 0.0f;
    }

    void add(const Prim& p)
    {
        lo.min(p.lo);
        hi.max(p.hi);
        minSelf = std::min(minSelf, p.selfError);
        maxParent = std::max(maxParent, p.parentError);
    }

    void merge(const Box& o)
    {
        lo.min(o.lo);
        hi.max(o.hi);
        minSelf = std::min(minSelf, o.minSelf);
        maxParent = std::max(maxParent, o.maxParent);
    }

    float area() const
    {
        const float dx = hi.x - lo.x;
        const float dy = hi.y - lo.y;
        const float dz = hi.z - lo.z;
        if (dx < 0.0f)
            return 0.0f;
        return 2.0f * (dx * dy + dy * dz + dz * dx);
    }
};

struct Builder {
    xr_vector<Prim>* prims;
    ClusterBvh* out;

    u32 Build(u32 first, u32 count, u32 depth)
    {
        const u32 self = u32(out->nodes.size());
        out->nodes.push_back(ClusterBvhNode{});
        out->maxDepth = std::max(out->maxDepth, depth);

        Box bounds;
        bounds.reset();
        Box centroids;
        centroids.reset();
        for (u32 i = 0; i < count; ++i) {
            const Prim& p = (*prims)[first + i];
            bounds.add(p);
            centroids.lo.min(p.centroid);
            centroids.hi.max(p.centroid);
        }

        u32 mid = 0;
        if (count > kClusterBvhLeafSize)
            mid = Split(first, count, centroids);

        ClusterBvhNode& node = out->nodes[self];
        node.bmin[0] = bounds.lo.x;
        node.bmin[1] = bounds.lo.y;
        node.bmin[2] = bounds.lo.z;
        node.bmax[0] = bounds.hi.x;
        node.bmax[1] = bounds.hi.y;
        node.bmax[2] = bounds.hi.z;
        node.minSelfError = bounds.minSelf;
        node.maxParentError = bounds.maxParent;

        if (mid == 0) {
            out->nodes[self].first = first;
            out->nodes[self].count = count;
            out->nodes[self].escape = self + 1;
            out->leafCount++;
            return self;
        }

        Build(first, mid, depth + 1);
        Build(first + mid, count - mid, depth + 1);
        out->nodes[self].first = 0;
        out->nodes[self].count = 0;
        out->nodes[self].escape = u32(out->nodes.size());
        return self;
    }

    u32 Split(u32 first, u32 count, const Box& centroids)
    {
        const Fvector span = Fvector().sub(centroids.hi, centroids.lo);
        u32 axis = 0;
        if (span.y > span.x)
            axis = 1;
        if (span[axis] < span.z)
            axis = 2;
        const float extent = span[axis];
        if (!(extent > 1e-6f))
            return MedianSplit(first, count, axis);

        Box binBounds[kBinCount];
        u32 binCount[kBinCount] = {};
        for (u32 b = 0; b < kBinCount; ++b)
            binBounds[b].reset();

        const float scale = float(kBinCount) / extent;
        const float origin = centroids.lo[axis];
        for (u32 i = 0; i < count; ++i) {
            const Prim& p = (*prims)[first + i];
            u32 b = u32((p.centroid[axis] - origin) * scale);
            b = std::min(b, kBinCount - 1);
            binBounds[b].add(p);
            binCount[b]++;
        }

        float leftArea[kBinCount - 1];
        u32 leftCount[kBinCount - 1];
        {
            Box acc;
            acc.reset();
            u32 n = 0;
            for (u32 b = 0; b + 1 < kBinCount; ++b) {
                if (binCount[b]) {
                    acc.merge(binBounds[b]);
                    n += binCount[b];
                }
                leftArea[b] = acc.area();
                leftCount[b] = n;
            }
        }

        float bestCost = flt_max;
        u32 bestBin = kBinCount;
        {
            Box acc;
            acc.reset();
            u32 n = 0;
            for (u32 b = kBinCount - 1; b > 0; --b) {
                if (binCount[b]) {
                    acc.merge(binBounds[b]);
                    n += binCount[b];
                }
                const u32 nl = leftCount[b - 1];
                if (nl == 0 || n == 0)
                    continue;
                const float cost = leftArea[b - 1] * float(nl) + acc.area() * float(n);
                if (cost < bestCost) {
                    bestCost = cost;
                    bestBin = b;
                }
            }
        }

        if (bestBin == kBinCount)
            return MedianSplit(first, count, axis);

        Prim* begin = prims->data() + first;
        Prim* end = begin + count;
        Prim* pivot = std::partition(begin, end, [&](const Prim& p) {
            u32 b = u32((p.centroid[axis] - origin) * scale);
            b = std::min(b, kBinCount - 1);
            return b < bestBin;
        });
        const u32 mid = u32(pivot - begin);
        if (mid == 0 || mid == count)
            return MedianSplit(first, count, axis);
        return mid;
    }

    u32 MedianSplit(u32 first, u32 count, u32 axis)
    {
        const u32 mid = count / 2;
        Prim* begin = prims->data() + first;
        std::nth_element(begin, begin + mid, begin + count, [axis](const Prim& a, const Prim& b) {
            return a.centroid[axis] < b.centroid[axis];
        });
        return mid;
    }
};

}

void BuildClusterShadowBVH(const GPUClusterEntry* entries, u32 count, ClusterBvh& out)
{
    out.nodes.clear();
    out.indices.clear();
    out.maxDepth = 0;
    out.leafCount = 0;
    if (count == 0)
        return;

    xr_vector<Prim> prims;
    prims.resize(count);
    for (u32 i = 0; i < count; ++i) {
        const GPUClusterEntry& e = entries[i];
        Prim& p = prims[i];
        const Fvector c = Fvector().set(e.sphere.x, e.sphere.y, e.sphere.z);
        const Fvector h = Fvector().set(e.extent.x, e.extent.y, e.extent.z);
        p.lo.sub(c, h);
        p.hi.add(c, h);
        p.centroid = c;
        const bool plain = (e.flags & GPU_CLUSTER_ENTRY_PLAIN) != 0;
        p.selfError = plain ? 0.0f : e.selfError;
        p.parentError = plain ? kErrorInf : e.parentError;
        p.entry = i;
    }

    out.nodes.reserve(2u * ((count + kClusterBvhLeafSize - 1) / kClusterBvhLeafSize) + 1u);

    Builder builder;
    builder.prims = &prims;
    builder.out = &out;
    builder.Build(0, count, 0);

    out.indices.resize(count);
    for (u32 i = 0; i < count; ++i)
        out.indices[i] = prims[i].entry;
}

}
