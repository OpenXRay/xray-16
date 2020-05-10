#include "stdafx.h"
#include "ISpatial.h"
#include "xrCore/_fbox.h"
#include "xrCore/Threading/Lock.hpp"
#include "xrCore/Threading/ScopeLock.hpp"

extern Fvector c_spatial_offset[8];

template <bool b_first>
class walker
{
public:
    u32 mask;
    Fvector center;
    Fvector size;
    Fbox box;
    ISpatial_DB* space;

public:
    walker(ISpatial_DB* _space, u32 _mask, const Fvector& _center, const Fvector& _size)
    {
        mask = _mask;
        center = _center;
        size = _size;
        box.setb(center, size);
        space = _space;
    }

    void walk(ISpatial_NODE* N, Fvector& n_C, float n_R)
    {
        // box
        float n_vR = 2 * n_R;
        Fbox BB;
        BB.set(n_C.x - n_vR, n_C.y - n_vR, n_C.z - n_vR, n_C.x + n_vR, n_C.y + n_vR, n_C.z + n_vR);
        if (!BB.intersect(box))
            return;

        // test items
        for (auto& it : N->items)
        {
            ISpatial* S = it;
            if (0 == (S->GetSpatialData().type & mask))
                continue;

            Fvector& sC = S->GetSpatialData().sphere.P;
            float sR = S->GetSpatialData().sphere.R;
            Fbox sB;
            sB.set(sC.x - sR, sC.y - sR, sC.z - sR, sC.x + sR, sC.y + sR, sC.z + sR);
            if (!sB.intersect(box))
                continue;

            space->q_result->push_back(S);
            if (b_first)
                return;
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
            if (b_first && !space->q_result->empty())
                return;
        }
    }
};

void ISpatial_DB::q_box(xr_vector<ISpatial*>& R, u32 _o, u32 _mask, const Fvector& _center, const Fvector& _size)
{
    ScopeLock scope(&cs);
    Stats.Query.Begin();
    q_result = &R;
    q_result->clear();
    if (_o & O_ONLYFIRST)
    {
        walker<true> W(this, _mask, _center, _size);
        W.walk(m_root, m_center, m_bounds);
    }
    else
    {
        walker<false> W(this, _mask, _center, _size);
        W.walk(m_root, m_center, m_bounds);
    }
    Stats.Query.End();
}

void ISpatial_DB::q_sphere(xr_vector<ISpatial*>& R, u32 _o, u32 _mask, const Fvector& _center, const float _radius)
{
    Fvector _size = {_radius, _radius, _radius};
    q_box(R, _o, _mask, _center, _size);
}
