#include "stdafx.h"
#include "RayQuerySimd.h"
#include "ISpatial.h"
#include "xrCore/_fbox.h"
#include "xrCore/Threading/Lock.hpp"
#include "xrCore/Threading/ScopeLock.hpp"

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

extern Fvector c_spatial_offset[8];

namespace Spatial
{
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

template <bool b_use_sse, bool b_first, bool b_nearest>
class alignas(16) ray_walker
{
public:
    ray_t ray;
    u32 mask;
    float range;
    float range2;
    ISpatial_DB* space;

public:
    ray_walker(ISpatial_DB* _space, u32 _mask, const Fvector& _start, const Fvector& _dir, float _range)
    {
        mask = _mask;
        ray.pos.set(_start);
        ray.inv_dir.set(1.f, 1.f, 1.f).div(_dir);
        ray.fwd_dir.set(_dir);
        if constexpr (!b_use_sse)
        {
            // for FPU - zero out inf
            if (_abs(_dir.x) > flt_eps)
            {
            }
            else
                ray.inv_dir.x = 0;
            if (_abs(_dir.y) > flt_eps)
            {
            }
            else
                ray.inv_dir.y = 0;
            if (_abs(_dir.z) > flt_eps)
            {
            }
            else
                ray.inv_dir.z = 0;
        }
        range = _range;
        range2 = _range * _range;
        space = _space;
    }
    // fpu
    ICF bool _box_fpu(const Fvector& n_C, const float n_R, Fvector& coord)
    {
        // box
        float n_vR = 2 * n_R;
        Fbox BB;
        BB.set(n_C.x - n_vR, n_C.y - n_vR, n_C.z - n_vR, n_C.x + n_vR, n_C.y + n_vR, n_C.z + n_vR);
        return isect_fpu(BB.vMin, BB.vMax, ray, coord);
    }
    // sse
    ICF bool _box_sse(const Fvector& n_C, const float n_R, float& dist)
    {
        aabb_t box;
        /*
            float		n_vR	=		2*n_R;
            box.min.set	(n_C.x-n_vR, n_C.y-n_vR, n_C.z-n_vR);	box.min.pad = 0;
            box.max.set	(n_C.x+n_vR, n_C.y+n_vR, n_C.z+n_vR);	box.max.pad = 0;
        */
        __m128 NR = _mm_load_ss((float*)&n_R);
        __m128 NC = _mm_unpacklo_ps(_mm_load_ss((float*)&n_C.x), _mm_load_ss((float*)&n_C.y));
        NR = _mm_add_ss(NR, NR);
        NC = _mm_movelh_ps(NC, _mm_load_ss((float*)&n_C.z));
        NR = _mm_shuffle_ps(NR, NR, _MM_SHUFFLE(1, 0, 0, 0));

        _mm_store_ps((float*)&box.min, _mm_sub_ps(NC, NR));
        _mm_store_ps((float*)&box.max, _mm_add_ps(NC, NR));

        return isect_sse(box, ray, dist);
    }
    void walk(ISpatial_NODE* N, Fvector& n_C, float n_R)
    {
        // Actual ray/aabb test
        if constexpr (b_use_sse)
        {
            // use SSE
            float d;
            if (!_box_sse(n_C, n_R, d))
                return;
            if (d > range)
                return;
        }
        else
        {
            // use FPU
            Fvector P;
            if (!_box_fpu(n_C, n_R, P))
                return;
            if (P.distance_to_sqr(ray.pos) > range2)
                return;
        }

        // test items
        for (auto& it : N->items)
        {
            ISpatial* S = it;
            if (mask != (S->GetSpatialData().type & mask))
                continue;
            Fsphere& sS = S->GetSpatialData().sphere;
            int quantity;
            float afT[2];
            Fsphere::ERP_Result result = sS.intersect(ray.pos, ray.fwd_dir, range, quantity, afT);

            if (result == Fsphere::rpOriginInside || ((result == Fsphere::rpOriginOutside) && (afT[0] < range)))
            {
                if constexpr (b_nearest)
                {
                    switch (result)
                    {
                    case Fsphere::rpOriginInside: range = afT[0] < range ? afT[0] : range; break;
                    case Fsphere::rpOriginOutside: range = afT[0]; break;
                    }
                    range2 = range * range;
                }
                space->q_result->push_back(S);
                if constexpr (b_first)
                    return;
            }
        }

        // recurse
        float c_R = n_R / 2;
        for (u32 octant = 0; octant < 8; octant++)
        {
            if (0 == N->children[octant])
                continue;
            Fvector c_C;
            c_C.mad(n_C, c_spatial_offset[octant], c_R);
            walk(N->children[octant], c_C, c_R);
            if constexpr (b_first)
            {
                if (!space->q_result->empty())
                    return;
            }
        }
    }
};
} // namespace Spatial

void ISpatial_DB::q_ray(
    xr_vector<ISpatial*>& R, u32 _o, u32 _mask_and, const Fvector& _start, const Fvector& _dir, float _range)
{
    using namespace Spatial;

    ZoneScoped;
    ScopeLock scope(&cs);
    Stats.Query.Begin();
    q_result = &R;
    q_result->clear();
    if (CDB::use_simd_ray_query(CPU::HasSSE))
    {
        if (_o & O_ONLYFIRST)
        {
            if (_o & O_ONLYNEAREST)
            {
                ray_walker<true, true, true> W(this, _mask_and, _start, _dir, _range);
                W.walk(m_root, m_center, m_bounds);
            }
            else
            {
                ray_walker<true, true, false> W(this, _mask_and, _start, _dir, _range);
                W.walk(m_root, m_center, m_bounds);
            }
        }
        else
        {
            if (_o & O_ONLYNEAREST)
            {
                ray_walker<true, false, true> W(this, _mask_and, _start, _dir, _range);
                W.walk(m_root, m_center, m_bounds);
            }
            else
            {
                ray_walker<true, false, false> W(this, _mask_and, _start, _dir, _range);
                W.walk(m_root, m_center, m_bounds);
            }
        }
    }
    else
    {
        if (_o & O_ONLYFIRST)
        {
            if (_o & O_ONLYNEAREST)
            {
                ray_walker<false, true, true> W(this, _mask_and, _start, _dir, _range);
                W.walk(m_root, m_center, m_bounds);
            }
            else
            {
                ray_walker<false, true, false> W(this, _mask_and, _start, _dir, _range);
                W.walk(m_root, m_center, m_bounds);
            }
        }
        else
        {
            if (_o & O_ONLYNEAREST)
            {
                ray_walker<false, false, true> W(this, _mask_and, _start, _dir, _range);
                W.walk(m_root, m_center, m_bounds);
            }
            else
            {
                ray_walker<false, false, false> W(this, _mask_and, _start, _dir, _range);
                W.walk(m_root, m_center, m_bounds);
            }
        }
    }
    Stats.Query.End();
}
