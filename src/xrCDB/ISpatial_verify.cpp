#include "stdafx.h"
#include "ISpatial.h"

extern Fvector c_spatial_offset[8];

class walker
{
public:
    u32 o_count;
    u32 n_count;

public:
    walker() : o_count(0), n_count(0)
    {
    }
    void walk(ISpatial_NODE* N, Fvector& n_C, float n_R)
    {
        // test items
        n_count += 1;
        o_count += N->items.size();

        // recurse
        float c_R = n_R / 2;
        for (u32 octant = 0; octant < 8; octant++)
        {
            if (0 == N->children[octant])
                continue;
            Fvector c_C;
            c_C.mad(n_C, c_spatial_offset[octant], c_R);
            walk(N->children[octant], c_C, c_R);
        }
    }
};

bool ISpatial_DB::verify()
{
    walker W;
    W.walk(m_root, m_center, m_bounds);
    bool bResult = (W.o_count == Stats.ObjectCount) && (W.n_count == Stats.NodeCount);
    VERIFY(bResult);
    return bResult;
}
