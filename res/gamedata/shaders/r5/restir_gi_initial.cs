#define BINDLESS_NO_IMPLICIT_GRAD
#include "bindless_common.h"
#include "rt_shading.h"
#include "restir_gi_common.h"

cbuffer ReSTIRGIParams : register(b5) {
    float4x4 g_InvViewProj;
    float4x4 g_PrevViewProj;
    float4 g_CameraPos;
    float4 g_SunDir_Intensity;
    float4 g_SunColor_SkyWeight;
    float2 g_ScreenSize;
    float g_GIIntensity;
    uint g_FrameIndex;
    uint g_IdentityStaticCount;
    uint g_TerrainBatchCount;
    uint g_SkinnedBatchStart;
    uint g_GrassBatchStart;
    uint g_DetailAtlasIndex;
    uint g_DiffuseMode;
    uint g_RTLightCount;
    uint g_Pad;
};

Texture2D<float> t_Depth : register(t14);
Texture2D<float4> t_Normal : register(t15);
Texture2D<float4> t_BaseColor : register(t16);
Texture2D<float2> t_Material : register(t17);

RWTexture2D<float4> u_DirectLighting : register(u0);
RWTexture2D<float4> u_ReservoirA : register(u1);
RWTexture2D<float4> u_ReservoirB : register(u2);
RWTexture2D<float4> u_IndirectLighting : register(u3);

float3 ReconstructWorldPos(float2 pixel, float depth)
{
    float2 uv = (pixel + 0.5) / g_ScreenSize;
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    if (depth >= 0.9)
        clip.z = (depth - 0.9) * 10.0;
    float4 world = mul(g_InvViewProj, clip);
    return world.xyz / world.w;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    float4 normalData = t_Normal.Load(int3(pixel, 0));
    if (depth <= 0.0 || dot(normalData.xyz, normalData.xyz) < 0.25) {
        u_DirectLighting[pixel] = 0;
        u_IndirectLighting[pixel] = 0;
        u_ReservoirA[pixel] = 0;
        u_ReservoirB[pixel] = 0;
        return;
    }

    RTSceneParams scene = RTBuildSceneParams(g_IdentityStaticCount, g_TerrainBatchCount,
        g_SkinnedBatchStart, g_GrassBatchStart, g_DetailAtlasIndex, g_RTLightCount,
        g_DiffuseMode, g_SunDir_Intensity, g_SunColor_SkyWeight);

    float3 worldPos = ReconstructWorldPos(float2(pixel), depth);
    MaterialSurface primary = GBufferMaterialSurface(normalData, t_BaseColor.Load(int3(pixel, 0)), t_Material.Load(int3(pixel, 0)));
    float3 V = RTSafeNormalize(g_CameraPos.xyz - worldPos, primary.N);

    u_DirectLighting[pixel] = float4(RTDirectLighting(scene, primary, worldPos, primary.N, V), 1.0);

    uint rng = pcg_hash(pixel.x + pixel.y * 1973u + g_FrameIndex * 26699u);

    GIReservoir reservoir = EmptyReservoir();
    float3 unresampledIndirect = 0.0;

    RTBSDFSample bounce = RTSampleBSDF(primary, V, scene.diffuseMode, rng);
    if (bounce.valid)
    {
        float bounceSide = dot(bounce.direction, primary.N) >= 0.0 ? 1.0 : -1.0;
        float3 bounceOrigin = worldPos + primary.N * (RT_RAY_ORIGIN_OFFSET * bounceSide);
        RTSceneTrace trace = RTTraceRay(scene, bounceOrigin, bounce.direction, RT_RAY_DISTANCE, false, rng);
        unresampledIndirect = bounce.weight * trace.emissive;
        if (!trace.hit)
        {
            unresampledIndirect += bounce.weight * SampleRTSky(scene, bounce.direction) * trace.transmittance;
        }
        else
        {
            float3 hitPos = bounceOrigin + bounce.direction * trace.t;
            RTHitGeometry geometry = RTFetchHitGeometry(scene, trace, bounce.direction);
            RTHitSurface hit = RTResolveHitSurface(scene, trace, geometry);

            float3 secondaryV = RTSafeNormalize(worldPos - hitPos, -bounce.direction);
            float3 Lo = hit.surface.emissive + RTDirectLighting(scene, hit.surface, hitPos, geometry.geoNormal, secondaryV);
            Lo = min(Lo, RESTIR_MAX_RADIANCE) * trace.transmittance;

            float3 target = GITargetRadiance(primary, V, worldPos, hitPos, Lo, scene.diffuseMode);
            float targetLuminance = Luminance(target);
            if (targetLuminance > 0.0)
            {
                ReservoirUpdate(reservoir, targetLuminance / bounce.pdf, hitPos, hit.surface.N, Lo, rng);
                ReservoirFinalize(reservoir, targetLuminance);
            }
        }
    }

    u_IndirectLighting[pixel] = float4(unresampledIndirect, 1.0);
    float4 reservoirA, reservoirB;
    PackReservoir(reservoir, reservoirA, reservoirB);
    u_ReservoirA[pixel] = reservoirA;
    u_ReservoirB[pixel] = reservoirB;
}
