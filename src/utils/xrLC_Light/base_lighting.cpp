#include "stdafx.h"

#include "base_lighting.h"
#include "serialize.h"
void base_lighting::select(xr_vector<R_Light>& dest, xr_vector<R_Light>& src, Fvector& P, float R)
{
    Fsphere Sphere;
    Sphere.set(P, R);
    dest.clear();
    for (const R_Light& L : src)
    {
        if (L.type == LT_POINT)
        {
            float dist = Sphere.P.distance_to(L.position);
            if (dist > (Sphere.R + L.range))
                continue;
        }
        dest.emplace_back(L);
    }
}
void base_lighting::select(base_lighting& from, Fvector& P, float R)
{
    select(rgb, from.rgb, P, R);
    select(hemi, from.hemi, P, R);
    select(sun, from.sun, P, R);
}
/*
    xr_vector<R_Light>		rgb;		// P,N
    xr_vector<R_Light>		hemi;		// P,N
    xr_vector<R_Light>		sun;		// P
*/
void base_lighting::read(INetReader& r)
{
    r_pod_vector(r, rgb);
    r_pod_vector(r, hemi);
    r_pod_vector(r, sun);
}
void base_lighting::write(IWriter& w) const
{
    w_pod_vector(w, rgb);
    w_pod_vector(w, hemi);
    w_pod_vector(w, sun);
}
