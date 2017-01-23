#include "stdafx.h"
#ifndef _EDITOR
#define RENDER 1
#include "xrEngine/Render.h"
#include "Layers/xrRender/light.h"
#endif

namespace XRay
{
namespace Math
{
static const float S_distance = 48;
static const float S_distance2 = S_distance*S_distance;
static const float S_fade = 4.5;
static const float S_fade2 = S_fade*S_fade;

static ICF float PLC_energy_SSE(const Fvector &p, const Fvector &n, const light *L, float e)
{
    Fvector lDir;
    if (L->flags.type==IRender_Light::DIRECT)
    {
        // Cos
        lDir.invert(L->direction);
        float D = lDir.dotproduct(n);
        if (D<=0)
            return 0;
        return e;
    }
    else
    {
        // Distance
        float sqD = p.distance_to_sqr(L->position);
        if (sqD > L->range*L->range)
            return 0;
        // Dir
        lDir.sub(L->position, p);
        lDir.normalize_safe();
        float D = lDir.dotproduct(n);
        if (D<=0)
            return 0;
        // Trace Light
        __m128 rcpr = _mm_rsqrt_ss(_mm_load_ss(&sqD));
        rcpr = _mm_rcp_ss(_mm_add_ss(rcpr, _mm_set_ss(1.0f)));
        float att;
        _mm_store_ss(&att, rcpr);
        return e*att;
    }
}

static ICF int iCeil_SSE(float x)
{ return _mm_cvt_ss2si(_mm_set_ss(x)); }

void PLCCalc_SSE(int &c0, int &c1, int &c2, const Fvector &camPos, const Fvector *ps, const Fvector &n,
    const light *l, float energy, const Fvector &obj)
{
    float e = PLC_energy_SSE(ps[0], n, l, energy);
    float nc1 = clampr(camPos.distance_to_sqr(ps[0]) / S_distance2, 0.f, 1.f);
    float nc2 = clampr(obj.distance_to_sqr(ps[0]) / S_fade2, 0.f, 1.f);
    float a = 1.f - 1.5f*e*(1.f - nc1)*(1.f - nc2);
    c0 = iCeil_SSE(255.f*a);
    e = PLC_energy_SSE(ps[1], n, l, energy);
    nc1 = clampr(camPos.distance_to_sqr(ps[1]) / S_distance2, 0.f, 1.f);
    nc2 = clampr(obj.distance_to_sqr(ps[1]) / S_fade2, 0.f, 1.f);
    a = 1.f - 1.5f*e*(1.f - nc1)*(1.f - nc2);
    c1 = iCeil_SSE(255.f*a);
    e = PLC_energy_SSE(ps[2], n, l, energy);
    nc1 = clampr(camPos.distance_to_sqr(ps[2]) / S_distance2, 0.f, 1.f);
    nc2 = clampr(obj.distance_to_sqr(ps[2]) / S_fade2, 0.f, 1.f);
    a = 1.f - 1.5f*e*(1.f - nc1)*(1.f - nc2);
    c2 = iCeil_SSE(255.f*a);
}

} // namespace Math
} // namespace XRay
