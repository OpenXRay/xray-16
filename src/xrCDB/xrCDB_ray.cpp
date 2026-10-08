#include "stdafx.h"
#include "RayQuerySimd.h"
#pragma hdrstop // ???

#include "xrCore/_fbox.h"
#include "xrCDB.h"

#if defined(XR_ARCHITECTURE_X86) || defined(XR_ARCHITECTURE_X64) || defined(XR_ARCHITECTURE_E2K) || defined(XR_ARCHITECTURE_PPC64)
#include <xmmintrin.h>
#elif defined(XR_ARCHITECTURE_ARM) || defined(XR_ARCHITECTURE_ARM64)
// The slab test uses SSE min/max operand ordering to filter 0 * infinity.
#define SSE2NEON_PRECISE_MINMAX 1
#include "sse2neon/sse2neon.h"
#elif defined(XR_ARCHITECTURE_RISCV)
#include "sse2rvv/sse2rvv.h"
#else
#error Add your platform here
#endif

#include "RayAabbSimd.h"

namespace CDB
{
using namespace Opcode;

struct alignas(16) vec_t : public Fvector3
{
    float pad = 0;
};
// static vec_t	vec_c	( float _x, float _y, float _z)	{ vec_t v; v.x=_x;v.y=_y;v.z=_z;v.pad=0; return v; }

struct alignas(16) aabb_t
{
    vec_t min;
    vec_t max;
};

struct alignas(16) ray_t
{
    vec_t pos;
    vec_t inv_dir;
    vec_t fwd_dir;
};

ICF u32& uf(float& x) { return (u32&)x; }
ICF bool isect_fpu(const Fvector& min, const Fvector& max, const ray_t& ray, Fvector& coord)
{
    Fvector MaxT;
    MaxT.x = MaxT.y = MaxT.z = -1.0f;
    bool Inside = true;

    // Find candidate planes.
    if (ray.pos[0] < min[0])
    {
        coord[0] = min[0];
        Inside = FALSE;
        if (uf(ray.inv_dir[0]))
            MaxT[0] = (min[0] - ray.pos[0]) * ray.inv_dir[0]; // Calculate T distances to candidate planes
    }
    else if (ray.pos[0] > max[0])
    {
        coord[0] = max[0];
        Inside = FALSE;
        if (uf(ray.inv_dir[0]))
            MaxT[0] = (max[0] - ray.pos[0]) * ray.inv_dir[0]; // Calculate T distances to candidate planes
    }
    if (ray.pos[1] < min[1])
    {
        coord[1] = min[1];
        Inside = FALSE;
        if (uf(ray.inv_dir[1]))
            MaxT[1] = (min[1] - ray.pos[1]) * ray.inv_dir[1]; // Calculate T distances to candidate planes
    }
    else if (ray.pos[1] > max[1])
    {
        coord[1] = max[1];
        Inside = FALSE;
        if (uf(ray.inv_dir[1]))
            MaxT[1] = (max[1] - ray.pos[1]) * ray.inv_dir[1]; // Calculate T distances to candidate planes
    }
    if (ray.pos[2] < min[2])
    {
        coord[2] = min[2];
        Inside = FALSE;
        if (uf(ray.inv_dir[2]))
            MaxT[2] = (min[2] - ray.pos[2]) * ray.inv_dir[2]; // Calculate T distances to candidate planes
    }
    else if (ray.pos[2] > max[2])
    {
        coord[2] = max[2];
        Inside = FALSE;
        if (uf(ray.inv_dir[2]))
            MaxT[2] = (max[2] - ray.pos[2]) * ray.inv_dir[2]; // Calculate T distances to candidate planes
    }

    // Ray ray.pos inside bounding box
    if (Inside)
    {
        coord = ray.pos;
        return true;
    }

    // Get largest of the maxT's for final choice of intersection
    u32 WhichPlane = 0;
    if (MaxT[1] > MaxT[0])
        WhichPlane = 1;
    if (MaxT[2] > MaxT[WhichPlane])
        WhichPlane = 2;

    // Check final candidate actually inside box (if max < 0)
    if (uf(MaxT[WhichPlane]) & 0x80000000)
        return false;

    if (0 == WhichPlane)
    { // 1 & 2
        coord[1] = ray.pos[1] + MaxT[0] * ray.fwd_dir[1];
        if ((coord[1] < min[1]) || (coord[1] > max[1]))
            return false;
        coord[2] = ray.pos[2] + MaxT[0] * ray.fwd_dir[2];
        if ((coord[2] < min[2]) || (coord[2] > max[2]))
            return false;
        return true;
    }
    if (1 == WhichPlane)
    { // 0 & 2
        coord[0] = ray.pos[0] + MaxT[1] * ray.fwd_dir[0];
        if ((coord[0] < min[0]) || (coord[0] > max[0]))
            return false;
        coord[2] = ray.pos[2] + MaxT[1] * ray.fwd_dir[2];
        if ((coord[2] < min[2]) || (coord[2] > max[2]))
            return false;
        return true;
    }
    if (2 == WhichPlane)
    { // 0 & 1
        coord[0] = ray.pos[0] + MaxT[2] * ray.fwd_dir[0];
        if ((coord[0] < min[0]) || (coord[0] > max[0]))
            return false;
        coord[1] = ray.pos[1] + MaxT[2] * ray.fwd_dir[1];
        if ((coord[1] < min[1]) || (coord[1] > max[1]))
            return false;
        return true;
    }
    return false;
}

ICF bool isect_sse(const aabb_t& box, const ray_t& ray, float& dist)
{
    return CDB::ray_aabb_simd(&box.min.x, &box.max.x, &ray.pos.x, &ray.inv_dir.x, dist);
}

template <bool bUseSSE, bool bCull, bool bFirst, bool bNearest>
class alignas(16) ray_collider
{
public:
    COLLIDER* dest;
    TRI* tris;
    Fvector* verts;

    ray_t ray;
    float rRange;
    float rRange2;

    IC void _init(COLLIDER* CL, Fvector* V, TRI* T, const Fvector& C, const Fvector& D, float R)
    {
        dest = CL;
        tris = T;
        verts = V;
        ray.pos.set(C);
        ray.inv_dir.set(1.f, 1.f, 1.f).div(D);
        ray.fwd_dir.set(D);
        rRange = R;
        rRange2 = R * R;
        if constexpr (!bUseSSE)
        {
            // for FPU - zero out inf
            if (_abs(D.x) > flt_eps)
            {
            }
            else
                ray.inv_dir.x = 0;
            if (_abs(D.y) > flt_eps)
            {
            }
            else
                ray.inv_dir.y = 0;
            if (_abs(D.z) > flt_eps)
            {
            }
            else
                ray.inv_dir.z = 0;
        }
    }

    // fpu
    ICF bool _box_fpu(const Fvector& bCenter, const Fvector& bExtents, Fvector& coord)
    {
        Fbox BB;
        BB.vMin.sub(bCenter, bExtents);
        BB.vMax.add(bCenter, bExtents);
        return isect_fpu(BB.vMin, BB.vMax, ray, coord);
    }
    // sse
    ICF bool _box_sse(const Fvector& bCenter, const Fvector& bExtents, float& dist)
    {
        aabb_t box;
        /*
            box.min.sub (bCenter,bExtents);	box.min.pad = 0;
            box.max.add	(bCenter,bExtents); box.max.pad = 0;
        */
        __m128 CN = _mm_unpacklo_ps(_mm_load_ss((float*)&bCenter.x), _mm_load_ss((float*)&bCenter.y));
        CN = _mm_movelh_ps(CN, _mm_load_ss((float*)&bCenter.z));
        __m128 EX = _mm_unpacklo_ps(_mm_load_ss((float*)&bExtents.x), _mm_load_ss((float*)&bExtents.y));
        EX = _mm_movelh_ps(EX, _mm_load_ss((float*)&bExtents.z));

        _mm_store_ps((float*)&box.min, _mm_sub_ps(CN, EX));
        _mm_store_ps((float*)&box.max, _mm_add_ps(CN, EX));

        return isect_sse(box, ray, dist);
    }

    IC bool _tri(u32* p, float& u, float& v, float& range)
    {
        Fvector edge1, edge2, tvec, pvec, qvec;
        float det, inv_det;

        // find vectors for two edges sharing vert0
        Fvector& p0 = verts[p[0]];
        Fvector& p1 = verts[p[1]];
        Fvector& p2 = verts[p[2]];
        edge1.sub(p1, p0);
        edge2.sub(p2, p0);
        // begin calculating determinant - also used to calculate U parameter
        // if determinant is near zero, ray lies in plane of triangle
        pvec.crossproduct(ray.fwd_dir, edge2);
        det = edge1.dotproduct(pvec);
        if constexpr (bCull)
        {
            if (det < EPS)
                return false;
            tvec.sub(ray.pos, p0); // calculate distance from vert0 to ray origin
            u = tvec.dotproduct(pvec); // calculate U parameter and test bounds
            if (u < 0.f || u > det)
                return false;
            qvec.crossproduct(tvec, edge1); // prepare to test V parameter
            v = ray.fwd_dir.dotproduct(qvec); // calculate V parameter and test bounds
            if (v < 0.f || u + v > det)
                return false;
            range = edge2.dotproduct(qvec); // calculate t, scale parameters, ray intersects triangle
            inv_det = 1.0f / det;
            range *= inv_det;
            u *= inv_det;
            v *= inv_det;
        }
        else
        {
            if (det > -EPS && det < EPS)
                return false;
            inv_det = 1.0f / det;
            tvec.sub(ray.pos, p0); // calculate distance from vert0 to ray origin
            u = tvec.dotproduct(pvec) * inv_det; // calculate U parameter and test bounds
            if (u < 0.0f || u > 1.0f)
                return false;
            qvec.crossproduct(tvec, edge1); // prepare to test V parameter
            v = ray.fwd_dir.dotproduct(qvec) * inv_det; // calculate V parameter and test bounds
            if (v < 0.0f || u + v > 1.0f)
                return false;
            range = edge2.dotproduct(qvec) * inv_det; // calculate t, ray intersects triangle
        }
        return true;
    }

    void _prim(u32 prim)
    {
        float u, v, r;
        if (!_tri(tris[prim].verts, u, v, r))
            return;
        if (r <= 0 || r > rRange)
            return;

        if constexpr (bNearest)
        {
            if (dest->r_count())
            {
                RESULT& R = *dest->r_begin();
                if (r < R.range)
                {
                    R.id = prim;
                    R.range = r;
                    R.u = u;
                    R.v = v;
                    R.verts[0] = verts[tris[prim].verts[0]];
                    R.verts[1] = verts[tris[prim].verts[1]];
                    R.verts[2] = verts[tris[prim].verts[2]];
                    R.dummy = tris[prim].dummy;
                    rRange = r;
                    rRange2 = r * r;
                }
            }
            else
            {
                RESULT& R = dest->r_add();
                R.id = prim;
                R.range = r;
                R.u = u;
                R.v = v;
                R.verts[0] = verts[tris[prim].verts[0]];
                R.verts[1] = verts[tris[prim].verts[1]];
                R.verts[2] = verts[tris[prim].verts[2]];
                R.dummy = tris[prim].dummy;
                rRange = r;
                rRange2 = r * r;
            }
        }
        else
        {
            RESULT& R = dest->r_add();
            R.id = prim;
            R.range = r;
            R.u = u;
            R.v = v;
            R.verts[0] = verts[tris[prim].verts[0]];
            R.verts[1] = verts[tris[prim].verts[1]];
            R.verts[2] = verts[tris[prim].verts[2]];
            R.dummy = tris[prim].dummy;
        }
    }
    void _stab(const AABBNoLeafNode* node)
    {
        // Should help
        _mm_prefetch((char*)node->GetNeg(), _MM_HINT_NTA);

        // Actual ray/aabb test
        if constexpr (bUseSSE)
        {
            // use SSE
            float d;
            if (!_box_sse((Fvector&)node->mAABB.mCenter, (Fvector&)node->mAABB.mExtents, d))
                return;
            if (d > rRange)
                return;
        }
        else
        {
            // use FPU
            Fvector P;
            if (!_box_fpu((Fvector&)node->mAABB.mCenter, (Fvector&)node->mAABB.mExtents, P))
                return;
            if (P.distance_to_sqr(ray.pos) > rRange2)
                return;
        }

        // 1st chield
        if (node->HasLeaf())
            _prim(node->GetPrimitive());
        else
            _stab(node->GetPos());

        // Early exit for "only first"
        if constexpr (bFirst)
        {
            if (dest->r_count())
                return;
        }

        // 2nd chield
        if (node->HasLeaf2())
            _prim(node->GetPrimitive2());
        else
            _stab(node->GetNeg());
    }
};

#if defined(XR_PLATFORM_ANDROID)
bool MODEL::audit_ray_path_step(RayPathAudit& audit, const Fvector& origin, const Fvector& direction,
    u32 triangle, float referenceRange, u32 frame, u32 query) const
{
    syncronize();
    const auto* nodes = static_cast<const AABBNoLeafTree*>(tree->GetTree())->GetNodes();
    if (!audit.started)
    {
        audit.started = true;
        audit.path.push_back({nodes, 0});
    }
    const auto start = CPU::QPC();
    const auto budget = std::max(u64(1), CPU::qpc_freq / 1000);
    for (u32 work = 0; !audit.path.empty() && work < 32768; ++work)
    {
        if (work && work % 64 == 0 && CPU::QPC() - start >= budget)
            return false;
        auto& entry = audit.path.back();
        if (entry.child == 2)
        {
            audit.path.pop_back();
            continue;
        }
        const bool negative = entry.child++ != 0;
        const auto* node = entry.node;
        ++audit.visited;
        const bool leaf = negative ? node->HasLeaf2() : node->HasLeaf();
        if (!leaf)
        {
            audit.path.push_back({negative ? node->GetNeg() : node->GetPos(), 0});
            continue;
        }
        if ((negative ? node->GetPrimitive2() : node->GetPrimitive()) != triangle)
            continue;

        ray_collider<false, false, false, true> scalar;
        scalar._init(nullptr, verts, tris, origin, direction, 500.f);
        float u = 0, v = 0, range = 0;
        const bool triangleHit = scalar._tri(tris[triangle].verts, u, v, range);
        COLLIDER simdResult;
        ray_collider<true, false, false, true> simd;
        simd._init(&simdResult, verts, tris, origin, direction, 500.f);
        simd.ray.pos.pad = simd.ray.inv_dir.pad = simd.ray.fwd_dir.pad = 0;
        simd._stab(nodes);
        Msg("[ray-path] frame=%u query=%u target=%u found=1 depth=%zu visited=%u "
            "float-hit=%d float-range=%.9g reference-range=%.9g simd-id=%d simd-range=%.9g",
            frame, query, triangle, audit.path.size(), audit.visited, triangleHit, range, referenceRange,
            simdResult.r_count() ? simdResult.r_begin()->id : -1,
            simdResult.r_count() ? simdResult.r_begin()->range : 0.f);
        u32 rejected = 0;
        for (const auto& ancestor : audit.path)
        {
            const auto& box = ancestor.node->mAABB;
            const Fvector center{box.mCenter.x, box.mCenter.y, box.mCenter.z};
            const Fvector extent{box.mExtents.x, box.mExtents.y, box.mExtents.z};
            Fvector point{};
            float distance = 0;
            const bool fpuHit = scalar._box_fpu(center, extent, point);
            const bool simdHit = simd._box_sse(center, extent, distance);
            const float fpuDistance = fpuHit ? point.distance_to(origin) : -1.f;
            if (!fpuHit || !simdHit || fpuDistance > referenceRange + EPS_L || distance > referenceRange + EPS_L)
            {
                ++rejected;
                Msg("[ray-path] frame=%u query=%u node=%zu fpu=%d entry=%.9g simd=%d entry=%.9g "
                    "center=(%.9g,%.9g,%.9g) extent=(%.9g,%.9g,%.9g)",
                    frame, query, size_t(ancestor.node - nodes), fpuHit, fpuDistance, simdHit, distance,
                    center.x, center.y, center.z, extent.x, extent.y, extent.z);
            }
        }
        Msg("[ray-path] end frame=%u query=%u rejected-ancestors=%u", frame, query, rejected);
        audit.path.clear();
        return true;
    }
    if (audit.path.empty())
    {
        Msg("[ray-path] frame=%u query=%u target=%u found=0 visited=%u", frame, query, triangle, audit.visited);
        return true;
    }
    return false;
}
#endif

void COLLIDER::ray_query(u32 ray_mode, const MODEL* m_def, const Fvector& r_start, const Fvector& r_dir, float r_range)
{
    ZoneScoped;
    m_def->syncronize();

    // Get nodes
    const AABBNoLeafTree* T = (const AABBNoLeafTree*)m_def->tree->GetTree();
    const AABBNoLeafNode* N = T->GetNodes();
    r_clear();

    if (CDB::use_simd_ray_query(CPU::HasSSE))
    {
        // SSE
        // Binary dispatcher
        if (ray_mode & OPT_CULL)
        {
            if (ray_mode & OPT_ONLYFIRST)
            {
                if (ray_mode & OPT_ONLYNEAREST)
                {
                    ray_collider<true, true, true, true> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
                else
                {
                    ray_collider<true, true, true, false> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
            }
            else
            {
                if (ray_mode & OPT_ONLYNEAREST)
                {
                    ray_collider<true, true, false, true> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
                else
                {
                    ray_collider<true, true, false, false> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
            }
        }
        else
        {
            if (ray_mode & OPT_ONLYFIRST)
            {
                if (ray_mode & OPT_ONLYNEAREST)
                {
                    ray_collider<true, false, true, true> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
                else
                {
                    ray_collider<true, false, true, false> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
            }
            else
            {
                if (ray_mode & OPT_ONLYNEAREST)
                {
                    ray_collider<true, false, false, true> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
                else
                {
                    ray_collider<true, false, false, false> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
            }
        }
    }
    else
    {
        // FPU
        // Binary dispatcher
        if (ray_mode & OPT_CULL)
        {
            if (ray_mode & OPT_ONLYFIRST)
            {
                if (ray_mode & OPT_ONLYNEAREST)
                {
                    ray_collider<false, true, true, true> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
                else
                {
                    ray_collider<false, true, true, false> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
            }
            else
            {
                if (ray_mode & OPT_ONLYNEAREST)
                {
                    ray_collider<false, true, false, true> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
                else
                {
                    ray_collider<false, true, false, false> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
            }
        }
        else
        {
            if (ray_mode & OPT_ONLYFIRST)
            {
                if (ray_mode & OPT_ONLYNEAREST)
                {
                    ray_collider<false, false, true, true> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
                else
                {
                    ray_collider<false, false, true, false> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
            }
            else
            {
                if (ray_mode & OPT_ONLYNEAREST)
                {
                    ray_collider<false, false, false, true> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
                else
                {
                    ray_collider<false, false, false, false> RC;
                    RC._init(this, m_def->verts, m_def->tris, r_start, r_dir, r_range);
                    RC._stab(N);
                }
            }
        }
    }
}
} // namespace CDB
