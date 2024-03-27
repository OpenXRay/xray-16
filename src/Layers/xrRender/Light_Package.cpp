#include "stdafx.h"
#include "Light_Package.h"

void light_Package::clear()
{
    v_point.clear();
    v_spot.clear();
    v_shadowed.clear();
}

#if (RENDER == R_R2) || (RENDER == R_R3) || (RENDER == R_R4) || (RENDER == R_R5) || (RENDER == R_GL)
void light_Package::sort()
{
    const auto pred_light_cmp = [](const light* l1, const light* l2)
    {
        if (l1->vis.pending)
        {
            if (l2->vis.pending)
                return l1->vis.query_order > l2->vis.query_order; // q-order
            else
                return false; // _2 should be first
        }
        else
        {
            if (l2->vis.pending)
                return true; // _1 should be first
            else
                return l1->range > l2->range; // sort by range
        }
    };

    // resort lights (pending -> at the end), maintain stable order
    std::stable_sort(v_point.begin(), v_point.end(), pred_light_cmp);
    std::stable_sort(v_spot.begin(), v_spot.end(), pred_light_cmp);
    std::stable_sort(v_shadowed.begin(), v_shadowed.end(), pred_light_cmp);
}
#endif // (RENDER==R_R2) || (RENDER==R_R3) || (RENDER==R_R4) || (RENDER==R_GL)
