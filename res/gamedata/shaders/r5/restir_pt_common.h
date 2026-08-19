#ifndef RESTIR_PT_COMMON_H
#define RESTIR_PT_COMMON_H

#ifndef RESTIR_GI_COMMON_H
#error "restir_gi_common.h must be included before restir_pt_common.h"
#endif

static const float RESTIR_PT_FOOTPRINT_C = 0.02;
static const float RESTIR_PT_RC_ALPHA = 0.2;

struct PTReservoir
{
    float3 rcPos;
    float W;
    float3 Lo;
    float targetPdf;
    float3 rcN;
    uint M;
    uint seed;
    uint flags;
    float hitDist;
    uint age;
};

PTReservoir EmptyPTReservoir()
{
    PTReservoir r;
    r.rcPos = 0;
    r.W = 0;
    r.Lo = 0;
    r.targetPdf = 0;
    r.rcN = float3(0, 1, 0);
    r.M = 0;
    r.seed = 0;
    r.flags = 0;
    r.hitDist = 0;
    r.age = 0;
    return r;
}

bool IsPTReservoirValid(PTReservoir r)
{
    return r.M > 0 && any(r.Lo > 0) && r.W > 0;
}

uint PackPTFlags(uint length, uint tech, uint specLobe)
{
    return (length & 7) | ((tech & 3) << 3) | ((specLobe & 1) << 5);
}

uint PTPathLength(uint flags) { return flags & 7; }
uint PTTech(uint flags) { return (flags >> 3) & 3; }
uint PTSpecLobe(uint flags) { return (flags >> 5) & 1; }

void PackPTReservoir(PTReservoir r, out uint4 A, out uint4 B)
{
    A.x = asuint(r.rcPos.x);
    A.y = asuint(r.rcPos.y);
    A.z = asuint(r.rcPos.z);
    A.w = (f32tof16(min(r.W, 65000.0)) & 0xFFFF) | ((f32tof16(min(r.hitDist, 65000.0)) & 0xFFFF) << 16);
    B.x = PackRGB9E5(r.Lo);
    B.y = PackUnorm2To16(OctEncode(r.rcN));
    B.z = r.seed;
    B.w = (r.M & 0xFF) | ((r.age & 0xFF) << 8) | ((r.flags & 0xFF) << 16);
}

PTReservoir UnpackPTReservoir(uint4 A, uint4 B)
{
    PTReservoir r = EmptyPTReservoir();
    r.rcPos = float3(asfloat(A.x), asfloat(A.y), asfloat(A.z));
    r.W = f16tof32(A.w & 0xFFFF);
    r.hitDist = f16tof32((A.w >> 16) & 0xFFFF);
    r.Lo = UnpackRGB9E5(B.x);
    r.rcN = OctDecode(UnpackUnorm2From16(B.y));
    r.seed = B.z;
    r.M = B.w & 0xFF;
    r.age = (B.w >> 8) & 0xFF;
    r.flags = (B.w >> 16) & 0xFF;
    r.targetPdf = max(Luminance(r.Lo) * max(r.W, 1e-4) * max((float)r.M, 1.0), 1e-4);
    return r;
}

float3 ShadePTReservoir(PTReservoir r, float3 worldPos, float3 N, float3 albedo, float metallic)
{
    if (!IsPTReservoirValid(r))
        return 0;
    float3 wi = r.rcPos - worldPos;
    float dist = length(wi);
    wi = dist > 1e-4 ? wi / dist : N;
    if (dot(N, wi) <= 0.02)
        return 0;
    return r.Lo * r.W * albedo * (1.0 - metallic);
}

bool PTFootprintOk(float3 xk, float3 xkm1, float3 Nkm1, float alphaKm1)
{
    float3 d = xk - xkm1;
    float dist2 = dot(d, d);
    float nDot = abs(dot(Nkm1, d));
    float ratio = nDot / max(dist2, 1e-6);
    return ratio < RESTIR_PT_FOOTPRINT_C && alphaKm1 >= RESTIR_PT_RC_ALPHA;
}

float HybridShiftJacobian(float3 rcN, float3 x1New, float3 x1Old, float3 rcPos)
{
    return clamp(JacobianReconnectionShift(rcN, x1New, x1Old, rcPos), 0.25, 4.0);
}

bool PTReservoirUpdate(inout PTReservoir r, float weight, PTReservoir cand, inout uint rng)
{
    if (isnan(weight) || isinf(weight) || weight <= 0)
        return false;
    float wsum = r.targetPdf + weight;
    r.M += 1;
    float xi = rand_float(rng);
    if (xi < weight / max(wsum, 1e-6)) {
        float keepM = r.M;
        uint keepAge = r.age;
        r = cand;
        r.M = keepM;
        r.age = keepAge;
        r.targetPdf = wsum;
        return true;
    }
    r.targetPdf = wsum;
    return false;
}

float PTCapFromDup(float D, float cCap)
{
    D = saturate(D);
    return lerp(cCap, 1.0, pow(D, 0.1));
}

#endif
