#ifndef SKY_VISIBILITY_H
#define SKY_VISIBILITY_H

#define SKY_VISIBILITY_SH_Y0 0.282094792
#define SKY_VISIBILITY_SH_Y1 0.488602512
#define SKY_PROBE_OFFSET_RANGE 0.45
#define SKY_PROBE_FLAG_RELOCATED 1u
#define SKY_PROBE_FAIL_SHIFT 1u
#define SKY_PROBE_FAIL_UNREACHABLE 1u
#define SKY_PROBE_FAIL_STUCK 2u
#define SKY_PROBE_DEPTH_RES 4
#define SKY_PROBE_DEPTH_TEXELS 16
#define SKY_PROBE_DEPTH_RANGE 3.0
#define SKY_PROBE_DEPTH_SHARPNESS 6.0
#define SKY_PROBE_DEPTH_BACKFACE_SCALE 0.2
#define SKY_PROBE_VISIBILITY_FLOOR 0.05

struct SkyProbeRecord
{
    uint4 visibility;
    uint4 depthMean;
    uint4 depthSigma;
};

float2 SkyProbeSignNotZero(float2 v)
{
    return float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

float2 SkyProbeOctEncode(float3 direction)
{
    float2 p = direction.xz / (abs(direction.x) + abs(direction.y) + abs(direction.z));
    if (direction.y < 0.0)
        p = (1.0 - abs(p.yx)) * SkyProbeSignNotZero(p);
    return p;
}

float3 SkyProbeOctDecode(float2 e)
{
    float3 v = float3(e.x, 1.0 - abs(e.x) - abs(e.y), e.y);
    if (v.y < 0.0)
        v.xz = (1.0 - abs(v.zx)) * SkyProbeSignNotZero(v.xz);
    return normalize(v);
}

float3 SkyProbeDepthTexelDirection(uint texel)
{
    float2 t = float2(texel % SKY_PROBE_DEPTH_RES, texel / SKY_PROBE_DEPTH_RES);
    return SkyProbeOctDecode((t + 0.5) * (2.0 / SKY_PROBE_DEPTH_RES) - 1.0);
}

uint SkyProbePackUnorm4(float4 v)
{
    uint4 q = uint4(round(saturate(v) * 255.0));
    return q.x | (q.y << 8) | (q.z << 16) | (q.w << 24);
}

float SkyProbeUnpackUnorm(uint4 packed, uint texel)
{
    return float((packed[texel >> 2] >> ((texel & 3u) * 8u)) & 0xFFu) * (1.0 / 255.0);
}

int2 SkyProbeOctWrap(int2 t)
{
    int last = SKY_PROBE_DEPTH_RES - 1;
    if (t.x < 0)
        t = int2(0, last - t.y);
    else if (t.x > last)
        t = int2(last, last - t.y);
    if (t.y < 0)
        t = int2(last - t.x, 0);
    else if (t.y > last)
        t = int2(last - t.x, last);
    return t;
}

float2 SkyProbeDepthTexel(SkyProbeRecord record, int2 t)
{
    int2 w = SkyProbeOctWrap(t);
    uint texel = uint(w.x + w.y * SKY_PROBE_DEPTH_RES);
    float mean = SkyProbeUnpackUnorm(record.depthMean, texel);
    float sigma = SkyProbeUnpackUnorm(record.depthSigma, texel) * 0.5;
    return float2(mean, sigma * sigma + mean * mean);
}

float2 SkyProbeDepthMoments(SkyProbeRecord record, float3 direction)
{
    float2 coord = (SkyProbeOctEncode(direction) * 0.5 + 0.5) * SKY_PROBE_DEPTH_RES - 0.5;
    int2 base = int2(floor(coord));
    float2 f = coord - float2(base);
    float2 m00 = SkyProbeDepthTexel(record, base);
    float2 m10 = SkyProbeDepthTexel(record, base + int2(1, 0));
    float2 m01 = SkyProbeDepthTexel(record, base + int2(0, 1));
    float2 m11 = SkyProbeDepthTexel(record, base + int2(1, 1));
    return lerp(lerp(m00, m10, f.x), lerp(m01, m11, f.x), f.y);
}

float SkyProbeChebyshevDetailed(SkyProbeRecord record, float3 probePosition, float3 samplePosition, float spacing,
    out float4 detail)
{
    float3 probeToPoint = samplePosition - probePosition;
    float pointDistance = length(probeToPoint);
    float range = SKY_PROBE_DEPTH_RANGE * spacing;
    float2 moments = SkyProbeDepthMoments(record, probeToPoint / max(pointDistance, 1e-4));
    float mean = moments.x * range;
    float tolerance = 0.03 * spacing;
    float variance = abs(moments.y - moments.x * moments.x) * range * range + tolerance * tolerance;
    detail = float4(pointDistance, mean, variance, 1.0);
    if (pointDistance <= mean)
        return 1.0;
    float delta = pointDistance - mean;
    float chebyshev = variance / (variance + delta * delta);
    float visibility = chebyshev * chebyshev * chebyshev;
    detail.w = visibility;
    return visibility;
}

float SkyProbeChebyshev(SkyProbeRecord record, float3 probePosition, float3 samplePosition, float spacing)
{
    float4 detail;
    return SkyProbeChebyshevDetailed(record, probePosition, samplePosition, spacing, detail);
}

uint SkyProbePackOffset(float3 offset, float spacing)
{
    int3 q = int3(round(clamp(offset / (SKY_PROBE_OFFSET_RANGE * spacing), -1.0, 1.0) * 127.0));
    return (uint(q.x) & 0xFFu) | ((uint(q.y) & 0xFFu) << 8) | ((uint(q.z) & 0xFFu) << 16);
}

float3 SkyProbeUnpackOffset(uint packed, float spacing)
{
    int3 q = int3(int(packed << 24) >> 24, int(packed << 16) >> 24, int(packed << 8) >> 24);
    return float3(q) * (SKY_PROBE_OFFSET_RANGE * spacing / 127.0);
}

uint2 SkyVisibilityPack(float c0, float3 c1)
{
    return uint2(f32tof16(c0) | (f32tof16(c1.x) << 16), f32tof16(c1.y) | (f32tof16(c1.z) << 16));
}

void SkyVisibilityUnpack(uint2 packed, out float c0, out float3 c1)
{
    c0 = f16tof32(packed.x & 0xFFFFu);
    c1 = float3(f16tof32(packed.x >> 16), f16tof32(packed.y & 0xFFFFu), f16tof32(packed.y >> 16));
}

bool SkyVisibilityBaked(float c0)
{
    return !isnan(c0);
}

bool SkyVisibilityValid(float c0)
{
    return SkyVisibilityBaked(c0) && c0 >= 0.0;
}

float SkyVisibilityCosine(float c0, float3 c1, float3 n)
{
    return saturate(SKY_VISIBILITY_SH_Y0 * c0 + (2.0 / 3.0) * SKY_VISIBILITY_SH_Y1 * dot(c1, n));
}

float SkyVisibilityAverage(float c0)
{
    return saturate(SKY_VISIBILITY_SH_Y0 * c0);
}

#endif
