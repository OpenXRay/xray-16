#include "stdafx.h"
#include "_bitwise.h"
#ifndef _EDITOR
#define RENDER 1
#include "xrEngine/Render.h"
#include "Layers/xrRender/light.h"
#endif

namespace XRay
{
namespace Math
{
static constexpr float S_distance = 48;
static constexpr float S_distance2 = S_distance * S_distance;
static constexpr float S_fade = 4.5;
static constexpr float S_fade2 = S_fade * S_fade;

IC float PLC_energy_CPP(const Fvector& p, const Fvector& n, const light* L, float e)
{
    Fvector Ldir;
    if (L->flags.type == IRender_Light::DIRECT)
    {
        // Cos
        Ldir.invert(L->direction);
        float D = Ldir.dotproduct(n);
        if (D <= 0) return 0;
        return e;
    }
    // Distance
    float sqD = p.distance_to_sqr(L->position);
    if (sqD > (L->range * L->range)) return 0;

    // Dir
    Ldir.sub(L->position, p);
    Ldir.normalize_safe();
    float D = Ldir.dotproduct(n);
    if (D <= 0) return 0;

    // Trace Light
    float R = _sqrt(sqD);
    float att = 1 - (1 / (1 + R));
    return (e * att);
}

void PLCCalc_CPP(int& c0, int& c1, int& c2, const Fvector& camPos, const Fvector* ps, const Fvector& n, const light* l,
    float energy, const Fvector& obj)
{
    float E = PLC_energy_CPP(ps[0], n, l, energy);
    float C1 = clampr(camPos.distance_to_sqr(ps[0]) / S_distance2, 0.f, 1.f);
    float C2 = clampr(obj.distance_to_sqr(ps[0]) / S_fade2, 0.f, 1.f);
    float A = 1.f - 1.5f * E * (1.f - C1) * (1.f - C2);
    c0 = iCeil(255.f * A);    
    E = PLC_energy_CPP(ps[1], n, l, energy);
    C1 = clampr(camPos.distance_to_sqr(ps[1]) / S_distance2, 0.f, 1.f);
    C2 = clampr(obj.distance_to_sqr(ps[1]) / S_fade2, 0.f, 1.f);
    A = 1.f - 1.5f * E * (1.f - C1) * (1.f - C2);
    c1 = iCeil(255.f * A);
    E = PLC_energy_CPP(ps[2], n, l, energy);
    C1 = clampr(camPos.distance_to_sqr(ps[2]) / S_distance2, 0.f, 1.f);
    C2 = clampr(obj.distance_to_sqr(ps[2]) / S_fade2, 0.f, 1.f);
    A = 1.f - 1.5f * E * (1.f - C1) * (1.f - C2);
    c2 = iCeil(255.f * A);
}
} // namespace Math
} // namespace XRay
