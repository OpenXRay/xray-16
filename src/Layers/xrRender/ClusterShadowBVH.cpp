#include "stdafx.h"
#include "ClusterShadowBVH.h"
#include <cmath>
#include <limits>

namespace xray::render::fg
{

namespace cluster_bvh_detail
{

constexpr u32 kBinCount = 16;
constexpr float kErrorInf = 1e30f;

void Box::reset()
{
    lo.set(flt_max, flt_max, flt_max);
    hi.set(-flt_max, -flt_max, -flt_max);
    lodCenter.set(0.0f, 0.0f, 0.0f);
    lodRadius = 0.0f;
    minSelf = kErrorInf;
    maxSelf = 0.0f;
    minParent = kErrorInf;
    maxParent = 0.0f;
    hasLod = false;
}

void Box::addSphere(const Fvector& c, float r)
{
    if (!hasLod)
    {
        lodCenter = c;
        lodRadius = r;
        hasLod = true;
        return;
    }
    ClusterMergeSphere(lodCenter, lodRadius, c, r);
}

void Box::add(const ClusterBvhPrim& p)
{
    lo.min(p.lo);
    hi.max(p.hi);
    addSphere(p.lodCenter, p.lodRadius);
    minSelf = std::min(minSelf, p.selfError);
    maxSelf = std::max(maxSelf, p.selfError);
    minParent = std::min(minParent, p.parentError);
    maxParent = std::max(maxParent, p.parentError);
}

void Box::merge(const Box& o)
{
    lo.min(o.lo);
    hi.max(o.hi);
    if (o.hasLod)
        addSphere(o.lodCenter, o.lodRadius);
    minSelf = std::min(minSelf, o.minSelf);
    maxSelf = std::max(maxSelf, o.maxSelf);
    minParent = std::min(minParent, o.minParent);
    maxParent = std::max(maxParent, o.maxParent);
}

float Box::area() const
{
    const float dx = hi.x - lo.x;
    const float dy = hi.y - lo.y;
    const float dz = hi.z - lo.z;
    if (dx < 0.0f)
        return 0.0f;
    return 2.0f * (dx * dy + dy * dz + dz * dx);
}

u32 MedianSplit(xr_vector<ClusterBvhPrim>& prims, u32 first, u32 count, u32 axis)
{
    const u32 mid = count / 2;
    ClusterBvhPrim* begin = prims.data() + first;
    std::nth_element(begin, begin + mid, begin + count, [axis](const ClusterBvhPrim& a, const ClusterBvhPrim& b) {
        return a.centroid[axis] < b.centroid[axis];
    });
    return mid;
}

u32 SpatialSplit(xr_vector<ClusterBvhPrim>& prims, u32 first, u32 count, const Fvector& clo, const Fvector& chi)
{
    const Fvector span = Fvector().sub(chi, clo);
    u32 axis = 0;
    if (span.y > span.x)
        axis = 1;
    if (span[axis] < span.z)
        axis = 2;
    const float extent = span[axis];
    if (!(extent > 1e-6f))
        return MedianSplit(prims, first, count, axis);

    Box binBounds[kBinCount];
    u32 binCount[kBinCount] = {};
    for (u32 b = 0; b < kBinCount; ++b)
        binBounds[b].reset();

    const float scale = float(kBinCount) / extent;
    const float origin = clo[axis];
    for (u32 i = 0; i < count; ++i) {
        const ClusterBvhPrim& p = prims[first + i];
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
        return MedianSplit(prims, first, count, axis);

    ClusterBvhPrim* begin = prims.data() + first;
    ClusterBvhPrim* end = begin + count;
    ClusterBvhPrim* pivot = std::partition(begin, end, [&](const ClusterBvhPrim& p) {
        u32 b = u32((p.centroid[axis] - origin) * scale);
        b = std::min(b, kBinCount - 1);
        return b < bestBin;
    });
    const u32 mid = u32(pivot - begin);
    if (mid == 0 || mid == count)
        return MedianSplit(prims, first, count, axis);
    return mid;
}

void CentroidRange(const xr_vector<ClusterBvhPrim>& prims, u32 first, u32 count, Fvector& lo, Fvector& hi)
{
    lo.set(flt_max, flt_max, flt_max);
    hi.set(-flt_max, -flt_max, -flt_max);
    for (u32 i = 0; i < count; ++i) {
        lo.min(prims[first + i].centroid);
        hi.max(prims[first + i].centroid);
    }
}

u32 KeySplit(xr_vector<ClusterBvhPrim>& prims, u32 first, u32 count)
{
    u32 minKey = UINT32_MAX;
    u32 maxKey = 0;
    for (u32 i = 0; i < count; ++i) {
        minKey = std::min(minKey, prims[first + i].key);
        maxKey = std::max(maxKey, prims[first + i].key);
    }
    if (minKey == maxKey)
        return 0;
    const u32 pivotKey = minKey + (maxKey - minKey) / 2u;
    ClusterBvhPrim* begin = prims.data() + first;
    ClusterBvhPrim* pivot = std::partition(begin, begin + count,
        [pivotKey](const ClusterBvhPrim& p) { return p.key <= pivotKey; });
    const u32 mid = u32(pivot - begin);
    if (mid == 0 || mid == count)
        return 0;
    return mid;
}

u32 EscapeBuilder::Build(u32 first, u32 count, u32 depth)
{
    const u32 self = u32(out->nodes.size());
    out->nodes.push_back(ClusterBvhNode{});
    out->maxDepth = std::max(out->maxDepth, depth);

    Box bounds;
    bounds.reset();
    for (u32 i = 0; i < count; ++i)
        bounds.add((*prims)[first + i]);

    u32 mid = 0;
    if (count > kClusterBvhLeafSize) {
        Fvector clo, chi;
        CentroidRange(*prims, first, count, clo, chi);
        mid = SpatialSplit(*prims, first, count, clo, chi);
    }

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


u32 WideBuilder::BuildTemp(u32 first, u32 count)
{
    const u32 self = u32(temps.size());
    temps.push_back(TempNode{});
    temps[self].bounds.reset();
    for (u32 i = 0; i < count; ++i)
        temps[self].bounds.add((*prims)[first + i]);
    temps[self].first = first;
    temps[self].count = count;
    temps[self].left = UINT32_MAX;
    temps[self].right = UINT32_MAX;
    temps[self].leaf = true;

    if (count <= kClusterBvhLeafSize)
        return self;

    u32 mid = KeySplit(*prims, first, count);
    if (mid == 0) {
        Fvector clo, chi;
        CentroidRange(*prims, first, count, clo, chi);
        mid = SpatialSplit(*prims, first, count, clo, chi);
    }
    if (mid == 0 || mid >= count)
        return self;

    const u32 left = BuildTemp(first, mid);
    const u32 right = BuildTemp(first + mid, count - mid);
    temps[self].leaf = false;
    temps[self].left = left;
    temps[self].right = right;
    return self;
}

void WideBuilder::Store(u32 slot, const Box& bounds, u32 child, u32 flags)
{
    GPUClusterAssetNode& n = out->nodes[slot];
    n.bmin[0] = bounds.lo.x;
    n.bmin[1] = bounds.lo.y;
    n.bmin[2] = bounds.lo.z;
    n.bmax[0] = bounds.hi.x;
    n.bmax[1] = bounds.hi.y;
    n.bmax[2] = bounds.hi.z;
    n.lodCenter[0] = bounds.lodCenter.x;
    n.lodCenter[1] = bounds.lodCenter.y;
    n.lodCenter[2] = bounds.lodCenter.z;
    n.lodRadius = bounds.lodRadius;
    n.minSelfError = bounds.minSelf;
    n.maxSelfError = bounds.maxSelf;
    n.minParentError = bounds.minParent;
    n.maxParentError = bounds.maxParent;
    n.child = child;
    n.flags = flags | (strict ? CLUSTER_ASSET_NODE_STRICT : 0u);
}

void WideBuilder::BuildWide(u32 slot, u32 temp, u32 depth)
{
    out->maxDepth = std::max(out->maxDepth, depth);
    const TempNode& node = temps[temp];
    if (node.leaf) {
        if (node.count > CLUSTER_ASSET_NODE_COUNT_MASK) {
            overflow = true;
            return;
        }
        const u32 base = u32(out->order.size());
        for (u32 i = 0; i < node.count; ++i)
            out->order.push_back((*prims)[node.first + i].payload);
        Store(slot, node.bounds, base, CLUSTER_ASSET_NODE_LEAF | (node.count << CLUSTER_ASSET_NODE_COUNT_SHIFT));
        out->leafCount++;
        return;
    }

    u32 kids[kClusterAssetBvhWidth];
    u32 kidCount = 2;
    kids[0] = node.left;
    kids[1] = node.right;
    while (kidCount < kClusterAssetBvhWidth) {
        u32 best = kClusterAssetBvhWidth;
        float bestArea = -1.0f;
        for (u32 i = 0; i < kidCount; ++i) {
            if (temps[kids[i]].leaf)
                continue;
            const float a = temps[kids[i]].bounds.area();
            if (a > bestArea) {
                bestArea = a;
                best = i;
            }
        }
        if (best == kClusterAssetBvhWidth)
            break;
        const u32 chosen = kids[best];
        kids[best] = temps[chosen].left;
        kids[kidCount++] = temps[chosen].right;
    }

    const u32 firstChild = u32(out->nodes.size());
    out->nodes.resize(out->nodes.size() + kidCount);
    Store(slot, node.bounds, firstChild, kidCount << CLUSTER_ASSET_NODE_COUNT_SHIFT);
    for (u32 i = 0; i < kidCount; ++i)
        BuildWide(firstChild + i, kids[i], depth + 1);
}

}

void ClusterMergeSphere(Fvector& center, float& radius, const Fvector& other, float otherRadius)
{
    const double dx = double(other.x) - center.x;
    const double dy = double(other.y) - center.y;
    const double dz = double(other.z) - center.z;
    const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (distance + otherRadius <= radius)
        return;
    if (distance + radius <= otherRadius)
    {
        center = other;
        radius = otherRadius;
        return;
    }

    const Fvector oldCenter = center;
    const float oldRadius = radius;
    const double merged = (distance + oldRadius + otherRadius) * 0.5;
    const double shift = (merged - oldRadius) / distance;
    center.set(float(double(oldCenter.x) + dx * shift),
        float(double(oldCenter.y) + dy * shift),
        float(double(oldCenter.z) + dz * shift));

    auto radiusFor = [&](const Fvector& sphereCenter, float sphereRadius)
    {
        const double x = double(sphereCenter.x) - center.x;
        const double y = double(sphereCenter.y) - center.y;
        const double z = double(sphereCenter.z) - center.z;
        return std::sqrt(x * x + y * y + z * z) + sphereRadius;
    };
    const double required = std::max(radiusFor(oldCenter, oldRadius), radiusFor(other, otherRadius));
    radius = std::nextafter(float(required), std::numeric_limits<float>::infinity());
    if (!std::isfinite(radius))
        FATAL("Cluster LOD bounds exceed the finite representation");
}

void BuildClusterShadowBVH(ClusterBvhPrim* prims, u32 count, ClusterBvh& out)
{
    out.nodes.clear();
    out.indices.clear();
    out.maxDepth = 0;
    out.leafCount = 0;
    if (count == 0 || !prims)
        return;

    xr_vector<ClusterBvhPrim> work;
    work.assign(prims, prims + count);

    out.nodes.reserve(2u * ((count + kClusterBvhLeafSize - 1) / kClusterBvhLeafSize) + 1u);

    cluster_bvh_detail::EscapeBuilder builder;
    builder.prims = &work;
    builder.out = &out;
    builder.Build(0, count, 0);

    out.indices.resize(count);
    for (u32 i = 0; i < count; ++i)
        out.indices[i] = work[i].payload;
}

bool BuildClusterAssetBVH8(ClusterBvhPrim* prims, u32 count, bool strict, ClusterAssetBvh& out)
{
    out.nodes.clear();
    out.order.clear();
    out.maxDepth = 0;
    out.leafCount = 0;
    if (count == 0 || !prims)
        return false;

    xr_vector<ClusterBvhPrim> work;
    work.assign(prims, prims + count);

    cluster_bvh_detail::WideBuilder builder;
    builder.prims = &work;
    builder.out = &out;
    builder.strict = strict;
    builder.overflow = false;
    builder.temps.reserve(2u * ((count + kClusterBvhLeafSize - 1) / kClusterBvhLeafSize) + 1u);
    const u32 root = builder.BuildTemp(0, count);

    out.nodes.resize(1);
    out.order.reserve(count);
    builder.BuildWide(0, root, 1);

    if (builder.overflow || out.order.size() != count)
        return false;
    return true;
}

bool ClusterShadowPairCapacity(const xr_vector<ClusterShadowCaster>& casters,
    float pageWidth, float errorThreshold, u32 pagesAxis, u32* capacity)
{
    if (!(pageWidth > 0.0f) || pagesAxis == 0)
        return false;
    u64 required[3] = {};
    for (const ClusterShadowCaster& caster : casters) {
        u64& pairs = required[caster.stream];
        ++pairs;
        if (caster.selfError > errorThreshold || caster.parentError <= errorThreshold)
            continue;
        const double span = 2.0 * double(caster.radius) / double(pageWidth);
        const u64 cells = u64(std::min(std::ceil(span) + 2.0, double(pagesAxis)));
        pairs += cells * cells;
    }
    for (u32 i = 0; i < 3; ++i) {
        if (required[i] > UINT32_MAX)
            return false;
    }
    for (u32 i = 0; i < 3; ++i)
        capacity[i] = u32(required[i]);
    return true;
}

}
