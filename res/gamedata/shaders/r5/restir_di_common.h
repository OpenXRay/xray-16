#ifndef RESTIR_DI_COMMON_H
#define RESTIR_DI_COMMON_H

#ifndef RESTIR_GI_COMMON_H
#error "restir_gi_common.h must be included before restir_di_common.h"
#endif

static const float RESTIR_DI_W_MAX = 64.0;

struct DIReservoir
{
    uint lightIndex;
    float targetPdf;
    float W;
    float w_sum;
    uint M;
    uint age;
};

float ClampDIReservoirW(float W)
{
    if (isnan(W) || isinf(W) || W <= 0)
        return 0;
    return min(W, RESTIR_DI_W_MAX);
}

DIReservoir EmptyDIReservoir()
{
    DIReservoir r;
    r.lightIndex = RESTIR_INVALID_ID;
    r.targetPdf = 0;
    r.W = 0;
    r.w_sum = 0;
    r.M = 0;
    r.age = 0;
    return r;
}

bool IsDIReservoirValid(DIReservoir r)
{
    return r.lightIndex != RESTIR_INVALID_ID && r.M > 0 && r.W > 0;
}

bool DIReservoirUpdate(inout DIReservoir r, float weight, uint lightIndex, float targetPdf, inout uint rng)
{
    r.M += 1;
    if (isnan(weight) || isinf(weight) || weight <= 0)
        return false;

    r.w_sum += weight;

    float xi = rand_float(rng);
    if (xi < weight / max(r.w_sum, 1e-6))
    {
        r.lightIndex = lightIndex;
        r.targetPdf = targetPdf;
        return true;
    }
    return false;
}

float4 PackDIReservoir(DIReservoir r)
{
    uint meta = ((min(r.M, 65535u) & 0xFFFFu) << 16) | (min(r.age, 65535u) & 0xFFFFu);
    return float4(asfloat(r.lightIndex), r.W, r.targetPdf, asfloat(meta));
}

DIReservoir UnpackDIReservoir(float4 packed)
{
    DIReservoir r;
    r.lightIndex = asuint(packed.x);
    r.W = packed.y;
    r.targetPdf = packed.z;
    uint meta = asuint(packed.w);
    r.M = (meta >> 16) & 0xFFFFu;
    r.age = meta & 0xFFFFu;
    r.w_sum = 0;
    return r;
}

#endif
