#pragma once

struct SPHImpact
{
    Fvector force;
    Fvector point;
    u16 geom;
    SPHImpact(const Fvector& aforce, const Fvector& apoint, u16 root_geom)
    {
        force.set(aforce);
        point.set(apoint);
        geom = root_geom;
    }
};
using PH_IMPACT_STORAGE = xr_vector<SPHImpact>;
