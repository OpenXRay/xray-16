#ifndef VOL_FOG_COMMON_H
#define VOL_FOG_COMMON_H

static const uint VOL_FOG_W = 160;
static const uint VOL_FOG_H = 90;
static const uint VOL_FOG_D = 64;

float VolFogHenyeyGreenstein(float cosTheta, float g)
{
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosTheta;
    return (1.0 - g2) / max(12.5663706 * pow(max(denom, 1e-4), 1.5), 1e-4);
}

float VolFogHash31(float3 p)
{
    p = frac(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return frac((p.x + p.y) * p.z);
}

float VolFogValueNoise(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);
    float n000 = VolFogHash31(i);
    float n100 = VolFogHash31(i + float3(1, 0, 0));
    float n010 = VolFogHash31(i + float3(0, 1, 0));
    float n110 = VolFogHash31(i + float3(1, 1, 0));
    float n001 = VolFogHash31(i + float3(0, 0, 1));
    float n101 = VolFogHash31(i + float3(1, 0, 1));
    float n011 = VolFogHash31(i + float3(0, 1, 1));
    float n111 = VolFogHash31(i + float3(1, 1, 1));
    float nx00 = lerp(n000, n100, f.x);
    float nx10 = lerp(n010, n110, f.x);
    float nx01 = lerp(n001, n101, f.x);
    float nx11 = lerp(n011, n111, f.x);
    float nxy0 = lerp(nx00, nx10, f.y);
    float nxy1 = lerp(nx01, nx11, f.y);
    return lerp(nxy0, nxy1, f.z);
}

float VolFogFBM(float3 p)
{
    float a = 0.0;
    float w = 0.5;
    [unroll] for (int i = 0; i < 4; ++i)
    {
        a += w * VolFogValueNoise(p);
        p = p * 2.02 + float3(17.1, 9.3, 3.7);
        w *= 0.5;
    }
    return a;
}

float VolFogCloudMask(float3 worldPos, float time, float amount)
{
    float amt = saturate(amount);
    if (amt < 1e-4)
        return 1.0;

    float3 drift = float3(time * 0.12, time * 0.035, time * 0.08);
    float banks = VolFogFBM(worldPos * float3(0.012, 0.028, 0.012) + drift * 0.35);
    float clumps = VolFogFBM(worldPos * float3(0.045, 0.09, 0.045) + drift + float3(31.0, 7.0, 19.0));
    float wisps = VolFogValueNoise(worldPos * 0.18 + drift * 1.7);

    float shape = saturate(banks * 1.15 - 0.28);
    shape = smoothstep(0.08, 0.72, shape);
    float detail = saturate(clumps * 1.25 - 0.2);
    detail = pow(detail, 1.35);
    float mask = saturate(shape * lerp(0.35, 1.0, detail));
    mask = lerp(mask, mask * lerp(0.7, 1.15, wisps), 0.35);
    return lerp(1.0, mask, amt);
}

float VolFogFroxelZ(uint z, uint depthSlices, float zNear, float zFar)
{
    float t = (float(z) + 0.5) / float(depthSlices);
    return zNear * pow(zFar / max(zNear, 1e-3), t);
}

float VolFogViewToT(float viewZ, float zNear, float zFar)
{
    return saturate(log(max(viewZ / max(zNear, 1e-3), 1e-4)) / log(max(zFar / max(zNear, 1e-3), 1.01)));
}

float3 VolFogFroxelWorldPos(uint3 id, float4x4 invViewProj, float3 cameraPos, float zNear, float zFar)
{
    float2 uv = (float2(id.xy) + 0.5) / float2(VOL_FOG_W, VOL_FOG_H);
    float viewZ = VolFogFroxelZ(id.z, VOL_FOG_D, zNear, zFar);
    float t = VolFogViewToT(viewZ, zNear, zFar);
    float depth = saturate(zNear / max(viewZ, zNear));
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    float4 world = mul(invViewProj, clip);
    float3 wpos = world.xyz / max(world.w, 1e-8);
    float3 dir = wpos - cameraPos;
    float d = length(dir);
    if (d > 1e-4)
        wpos = cameraPos + dir * (viewZ / d);
    else
    {
        float4 clipFar = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
        float4 worldFar = mul(invViewProj, clipFar);
        float3 farPos = worldFar.xyz / max(worldFar.w, 1e-8);
        dir = farPos - cameraPos;
        d = length(dir);
        wpos = cameraPos + (d > 1e-4 ? dir / d : float3(0, 0, 1)) * viewZ;
    }
    return wpos;
}

#endif
