#ifndef RESTIR_GI_COMMON_H
#define RESTIR_GI_COMMON_H

#ifndef RT_COMMON_H
#error "rt_common.h must be included before restir_gi_common.h"
#endif

static const uint RESTIR_INVALID_ID = 0xFFFFFFFF;
static const float RESTIR_MAX_RADIANCE = 100.0;
static const uint RESTIR_M_MAX = 20;
static const uint RESTIR_MAX_LOCAL_LIGHT_SAMPLES = 16;
static const uint RESTIR_MAX_CLUSTER_LIGHTS = 32;
static const uint RESTIR_MAX_LIGHTS_PER_TILE = 128;

struct GIReservoir
{
    float3 samplePos;
    float3 sampleNormal;
    float3 Lo;
    float W;
    float w_sum;
    uint M;
    uint age;
    uint lightId;
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
    r.lightId = RESTIR_INVALID_ID;
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

float3 ReconstructWorldPosReverseZ(float2 uv, float depth, float4x4 invViewProj)
{
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    float4 world = mul(invViewProj, clip);
    return world.xyz / max(world.w, 1e-8);
}

float3 ResolveGBufferWorldPos(float2 uv, float depth, float4 wpSample, float4x4 invViewProj)
{
    if (wpSample.w > 1.5 && length(wpSample.xyz) > 0.01)
        return wpSample.xyz;
    return ReconstructWorldPosReverseZ(uv, depth, invViewProj);
}

float2 OctEncode(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z) + 1e-8);
    if (n.z < 0) {
        float2 wrap = (1.0 - abs(n.yx)) * (float2(n.xy >= 0) * 2.0 - 1.0);
        n.xy = wrap;
    }
    return n.xy * 0.5 + 0.5;
}

float3 OctDecode(float2 e)
{
    e = e * 2.0 - 1.0;
    float3 n = float3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    if (n.z < 0) {
        float2 wrap = (1.0 - abs(n.yx)) * (float2(n.xy >= 0) * 2.0 - 1.0);
        n.xy = wrap;
    }
    return normalize(n);
}

uint PackUnorm2To16(float2 v)
{
    uint x = (uint)(saturate(v.x) * 65535.0 + 0.5);
    uint y = (uint)(saturate(v.y) * 65535.0 + 0.5);
    return (x & 0xFFFF) | ((y & 0xFFFF) << 16);
}

float2 UnpackUnorm2From16(uint p)
{
    return float2(float(p & 0xFFFF), float((p >> 16) & 0xFFFF)) / 65535.0;
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

void PackReservoirAB(GIReservoir r, out float4 A, out float4 B)
{
    A = float4(r.samplePos, r.W);
    B = float4(r.Lo, asfloat(PackNormalMAge(r.sampleNormal, r.M, r.age)));
}

GIReservoir UnpackReservoirAB(float4 A, float4 B)
{
    GIReservoir r = EmptyReservoir();
    r.samplePos = A.xyz;
    r.W = A.w;
    r.Lo = B.xyz;
    UnpackNormalMAge(asuint(B.w), r.sampleNormal, r.M, r.age);
    r.lightId = RESTIR_INVALID_ID;
    return r;
}

uint PackRGB9E5(float3 c)
{
    c = clamp(c, 0.0, RESTIR_MAX_RADIANCE);
    float maxc = max(max(c.r, c.g), max(c.b, 1e-6));
    int e = (int)ceil(log2(maxc)) + 15;
    e = clamp(e, 0, 31);
    float scale = exp2(float(15 - e));
    uint r = (uint)clamp(c.r * scale * 511.0 + 0.5, 0.0, 511.0);
    uint g = (uint)clamp(c.g * scale * 511.0 + 0.5, 0.0, 511.0);
    uint b = (uint)clamp(c.b * scale * 511.0 + 0.5, 0.0, 511.0);
    return (r) | (g << 9) | (b << 18) | ((uint)e << 27);
}

float3 UnpackRGB9E5(uint p)
{
    float scale = exp2(float(int((p >> 27) & 31) - 15));
    float r = float(p & 511) / 511.0;
    float g = float((p >> 9) & 511) / 511.0;
    float b = float((p >> 18) & 511) / 511.0;
    return float3(r, g, b) * scale;
}

uint4 PackReservoirU4(GIReservoir r, float3 primaryPos)
{
    float3 toSample = r.samplePos - primaryPos;
    float dist = length(toSample);
    float3 dir = dist > 1e-5 ? toSample / dist : float3(0, 1, 0);
    uint4 o;
    bool isGI = (r.lightId == RESTIR_INVALID_ID);
    o.x = isGI ? PackUnorm2To16(OctEncode(dir)) : r.lightId;
    o.y = PackRGB9E5(r.Lo);
    uint age = r.age & 0x7F;
    if (isGI)
        age |= 0x80;
    o.z = PackNormalMAge(r.sampleNormal, r.M, age);
    o.w = (f32tof16(min(r.W, 65000.0)) & 0xFFFF)
        | ((f32tof16(min(dist, 65000.0)) & 0xFFFF) << 16);
    return o;
}

GIReservoir UnpackReservoirU4(uint4 p, float3 primaryPos)
{
    GIReservoir r = EmptyReservoir();
    r.Lo = UnpackRGB9E5(p.y);
    UnpackNormalMAge(p.z, r.sampleNormal, r.M, r.age);
    bool isGI = (r.age & 0x80) != 0;
    r.age &= 0x7F;
    r.W = f16tof32(p.w & 0xFFFF);
    float dist = f16tof32((p.w >> 16) & 0xFFFF);
    if (isGI) {
        r.lightId = RESTIR_INVALID_ID;
        float3 dir = OctDecode(UnpackUnorm2From16(p.x));
        r.samplePos = primaryPos + dir * max(dist, 0.05);
    } else {
        r.lightId = p.x;
        r.samplePos = primaryPos + normalize(r.sampleNormal) * max(dist, 0.05);
    }
    r.w_sum = 0;
    return r;
}

bool ReservoirUpdate(inout GIReservoir r, float weight, float3 pos, float3 normal, float3 lo, uint lightId, inout uint rng)
{
    if (isnan(weight) || isinf(weight) || weight <= 0)
        return false;

    r.w_sum += weight;
    r.M += 1;

    float xi = rand_float(rng);
    if (xi < weight / max(r.w_sum, 1e-6)) {
        r.samplePos = pos;
        r.sampleNormal = normal;
        r.Lo = lo;
        r.lightId = lightId;
        return true;
    }
    return false;
}

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

bool ValidateTemporalNeighbor(float currLinearDepth, float3 currNormal, float prevLinearDepth, float3 prevNormal)
{
    float depthDiff = abs(currLinearDepth - prevLinearDepth) / max(currLinearDepth, 1e-4);
    if (depthDiff > 0.1)
        return false;
    if (dot(currNormal, prevNormal) < 0.906)
        return false;
    return true;
}

uint TemporalMClamp(uint M, uint age, uint mMax)
{
    uint ageCut = age > 16u ? (mMax / 2u) : mMax;
    if (age > 40u)
        ageCut = max(mMax / 4u, 1u);
    return min(M, max(ageCut, 1u));
}

float AgeConfidence(uint age, uint M)
{
    float a = 1.0 - saturate((float)age / 48.0);
    float m = saturate((float)M / 8.0);
    return saturate(0.35 + 0.65 * a * m);
}

int2 RestirFullPixel(uint2 giPixel, float2 giSize, float2 fullSize)
{
    float2 uv = (float2(giPixel) + 0.5) / max(giSize, float2(1.0, 1.0));
    return clamp(int2(uv * fullSize), int2(0, 0), int2(fullSize) - 1);
}

float RestirLoadDepth(Texture2D<float> depthTex, uint2 giPixel, float2 giSize, float2 fullSize)
{
    return depthTex.Load(int3(RestirFullPixel(giPixel, giSize, fullSize), 0));
}

float4 RestirLoadTex4(Texture2D<float4> tex, uint2 giPixel, float2 giSize, float2 fullSize)
{
    return tex.Load(int3(RestirFullPixel(giPixel, giSize, fullSize), 0));
}

float RestirLoadTex1(Texture2D<float> tex, uint2 giPixel, float2 giSize, float2 fullSize)
{
    return tex.Load(int3(RestirFullPixel(giPixel, giSize, fullSize), 0));
}

#endif
