#ifndef RESTIR_GI_COMMON_H
#define RESTIR_GI_COMMON_H

#ifndef RT_COMMON_H
#error "rt_common.h must be included before restir_gi_common.h"
#endif

static const uint RESTIR_INVALID_ID = 0xFFFFFFFF;
static const float RESTIR_MAX_RADIANCE = 100.0;
static const uint RESTIR_M_MAX = 20;

struct GIReservoir
{
    float3 samplePos;
    float3 sampleNormal;
    float3 Lo;
    float W;
    float w_sum;
    uint M;
    uint age;
};

GIReservoir EmptyReservoir()
{
    GIReservoir r;
    r.samplePos = 0;
    r.sampleNormal = 0;
    r.Lo = 0;
    r.W = 0;
    r.w_sum = 0;
    r.M = 0;
    r.age = 0;
    return r;
}

bool IsReservoirValid(GIReservoir r)
{
    return r.M > 0 && any(r.Lo > 0);
}

float Luminance(float3 c)
{
    return dot(c, float3(0.2126, 0.7152, 0.0722));
}

bool ReservoirUpdate(inout GIReservoir r, float weight, float3 pos, float3 normal, float3 lo, inout uint rng)
{
    if (isnan(weight) || isinf(weight))
        return false;

    r.w_sum += weight;
    r.M += 1;

    float xi = rand_float(rng);
    if (xi < weight / max(r.w_sum, 1e-6)) {
        r.samplePos = pos;
        r.sampleNormal = normal;
        r.Lo = lo;
        return true;
    }
    return false;
}

float2 OctEncode(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    if (n.z < 0) {
        float2 wrap = (1.0 - abs(n.yx)) * (n.xy >= 0 ? 1.0 : -1.0);
        n.xy = wrap;
    }
    return n.xy * 0.5 + 0.5;
}

float3 OctDecode(float2 e)
{
    e = e * 2.0 - 1.0;
    float3 n = float3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    if (n.z < 0) {
        float2 wrap = (1.0 - abs(n.yx)) * (n.xy >= 0 ? 1.0 : -1.0);
        n.xy = wrap;
    }
    return normalize(n);
}

uint PackNormalMAge(float3 normal, uint M, uint age)
{
    float2 oct = OctEncode(normal);
    uint ox = (uint)(saturate(oct.x) * 255.0) & 0xFF;
    uint oy = (uint)(saturate(oct.y) * 255.0) & 0xFF;
    uint mp = min(M, 255) & 0xFF;
    uint ap = min(age, 255) & 0xFF;
    return (ox << 24) | (oy << 16) | (mp << 8) | ap;
}

void UnpackNormalMAge(uint packed, out float3 normal, out uint M, out uint age)
{
    float2 oct;
    oct.x = float((packed >> 24) & 0xFF) / 255.0;
    oct.y = float((packed >> 16) & 0xFF) / 255.0;
    normal = OctDecode(oct);
    M = (packed >> 8) & 0xFF;
    age = packed & 0xFF;
}

// ReservoirA (RGBA32_FLOAT): samplePos.xyz, W
// ReservoirB (RGBA32_FLOAT): Lo.rgb, packed(normal_oct16 | M_u8 | age_u8)
void PackReservoir(GIReservoir r, out float4 A, out float4 B)
{
    A = float4(r.samplePos, r.W);
    B = float4(r.Lo, asfloat(PackNormalMAge(r.sampleNormal, r.M, r.age)));
}

GIReservoir UnpackReservoir(float4 A, float4 B)
{
    GIReservoir r;
    r.samplePos = A.xyz;
    r.W = A.w;
    r.Lo = B.xyz;
    UnpackNormalMAge(asuint(B.w), r.sampleNormal, r.M, r.age);
    r.w_sum = 0;
    return r;
}

// Jacobian of reconnection shift: reusing sample from pixel q at pixel r
// x1_new = primary surface at destination pixel (r)
// x1_old = primary surface at source pixel (q)
// x2 = secondary surface (sample point, fixed)
// x2_normal = normal at secondary surface
float JacobianReconnectionShift(float3 x2_normal, float3 x1_new, float3 x1_old, float3 x2)
{
    float3 v_new = x1_new - x2;
    float t_new2 = dot(v_new, v_new);
    v_new = t_new2 > 0 ? v_new * rsqrt(t_new2) : 0;

    float3 v_old = x1_old - x2;
    float t_old2 = dot(v_old, v_old);
    v_old = t_old2 > 0 ? v_old * rsqrt(t_old2) : 0;

    float cos_new = abs(dot(v_new, x2_normal));
    float cos_old = abs(dot(v_old, x2_normal));

    return (cos_new * t_old2) / max(cos_old * t_new2, 1e-6);
}

bool ValidateTemporalNeighbor(float currDepth, float3 currNormal, float prevDepth, float3 prevNormal)
{
    float depthDiff = abs(currDepth - prevDepth) / max(currDepth, 1e-4);
    if (depthDiff > 0.1)
        return false;
    if (dot(currNormal, prevNormal) < 0.906)
        return false;
    return true;
}

#endif
