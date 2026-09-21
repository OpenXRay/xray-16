#define BINDLESS_NO_IMPLICIT_GRAD
#include "bindless_common.h"
#include "rt_shading.h"
#include "restir_gi_common.h"

cbuffer ReSTIRGIParams : register(b5)
{
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
    uint g_ReuseReservoirs;
    uint g_EmissiveCount;
    uint g_MaxNullEvents;
    float g_EnvironmentRotation;
    float g_SunAngularRadius;
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
    if (pixel.x >= uint(g_ScreenSize.x) || pixel.y >= uint(g_ScreenSize.y))
        return;
    float depth = t_Depth.Load(int3(pixel, 0));
    float4 normalData = t_Normal.Load(int3(pixel, 0));
    if (depth <= 0.0 || dot(normalData.xyz, normalData.xyz) < 0.25)
    {
        u_DirectLighting[pixel] = 0.0;
        u_IndirectLighting[pixel] = 0.0;
        u_ReservoirA[pixel] = 0.0;
        u_ReservoirB[pixel] = 0.0;
        return;
    }
    RTSceneParams scene = RTBuildSceneParams(g_IdentityStaticCount, g_TerrainBatchCount,
        g_SkinnedBatchStart, g_GrassBatchStart, g_DetailAtlasIndex, g_RTLightCount,
        g_DiffuseMode, g_SunDir_Intensity, g_SunColor_SkyWeight, g_EmissiveCount,
        g_MaxNullEvents, g_EnvironmentRotation, g_SunAngularRadius);
    uint rng = pcg_hash(pixel.x + pixel.y * 1973u + g_FrameIndex * 26699u);
    float3 worldPos = ReconstructWorldPos(float2(pixel), depth);
    MaterialSurface primary = GBufferMaterialSurface(normalData, t_BaseColor.Load(int3(pixel, 0)), t_Material.Load(int3(pixel, 0)));
    float3 V = RTSafeNormalize(g_CameraPos.xyz - worldPos, primary.N);
    float coneWidth = max(length(ReconstructWorldPos(float2(pixel) + float2(1.0, 0.0), depth) - worldPos),
        length(ReconstructWorldPos(float2(pixel) + float2(0.0, 1.0), depth) - worldPos));
    float coneSpread = coneWidth / max(length(g_CameraPos.xyz - worldPos), 0.001);
    coneSpread = max(coneSpread, max(primary.roughness * primary.roughness, 1.0 - primary.metallic));
    RTDirectTerms direct = RTDirectLightingTerms(scene, primary, worldPos, primary.N, V,
        coneWidth, coneSpread, true, rng);
    float3 directRadiance = direct.diffuse + direct.specular;
    float3 unresampledIndirect = 0.0;
    GIReservoir reservoir = EmptyReservoir();
    RTBSDFSample bounce = RTSampleBSDF(primary, V, scene.diffuseMode, rng);
    if (bounce.valid)
    {
        float side = dot(bounce.direction, primary.N) >= 0.0 ? 1.0 : -1.0;
        float3 origin = worldPos + primary.N * (RT_RAY_ORIGIN_OFFSET * side);
        float3 direction = bounce.direction;
        float3 pathWeight = bounce.weight;
        float3 previousPosition = worldPos;
        float previousPdf = bounce.pdf;
        bool previousDelta = bounce.delta;
        bool reusable = true;
        bool indirectSource = false;
        uint nullEvents = 0u;
        for (uint event = 0u; event < scene.maxNullEvents; ++event)
        {
            RTSceneTrace trace = RTTraceRay(scene, origin, direction, RT_RAY_DISTANCE, false, rng,
                coneWidth, coneSpread, previousPosition, previousPdf, previousDelta);
            float3 sourceRadiance = pathWeight * trace.emissive;
            if (indirectSource)
                unresampledIndirect += sourceRadiance;
            else
                directRadiance += sourceRadiance;
            pathWeight *= trace.transmittance;
            nullEvents += trace.nullEvents;
            if (trace.exhausted || nullEvents >= scene.maxNullEvents)
                break;
            if (!trace.hit)
            {
                sourceRadiance = pathWeight * RTMissRadiance(scene, direction, previousPdf, previousDelta);
                if (indirectSource)
                    unresampledIndirect += sourceRadiance;
                else
                    directRadiance += sourceRadiance;
                break;
            }
            float3 hitPosition = origin + direction * trace.t;
            RTHitGeometry geometry = RTFetchHitGeometry(scene, trace, direction);
            RTHitSurface hit = RTResolveHitSurface(scene, trace, geometry);
            coneWidth += coneSpread * trace.t;
            float emissionWeight = RTEmissionWeight(scene, trace.batchIdx, trace.info, trace.primitiveIndex,
                previousPosition, hitPosition, previousPdf, previousDelta);
            sourceRadiance = pathWeight * hit.surface.emissive * emissionWeight;
            if (indirectSource)
                unresampledIndirect += sourceRadiance;
            else
                directRadiance += sourceRadiance;
            if ((hit.flags & MAT_FLAG_WATER) != 0u)
            {
                reusable = false;
                bool passthrough;
                RTBSDFSample water = RTSampleSurface(hit, geometry, -direction, scene.diffuseMode, rng, passthrough);
                if (!water.valid)
                    break;
                pathWeight *= water.weight;
                if (!passthrough)
                {
                    previousPosition = hitPosition;
                    previousPdf = water.pdf;
                    previousDelta = water.delta;
                    indirectSource = true;
                }
                else if (++nullEvents >= scene.maxNullEvents)
                    break;
                float waterSide = dot(water.direction, geometry.geoNormal) >= 0.0 ? 1.0 : -1.0;
                origin = hitPosition + geometry.geoNormal * (RT_RAY_ORIGIN_OFFSET * waterSide);
                direction = water.direction;
                continue;
            }
            coneSpread = max(coneSpread, max(hit.surface.roughness * hit.surface.roughness, 1.0 - hit.surface.metallic));
            RTDirectTerms secondary = RTDirectLightingTerms(scene, hit.surface, hitPosition,
                geometry.geoNormal, -direction, coneWidth, coneSpread, false, rng);
            float3 Lo = secondary.diffuse + secondary.specular;
            if (g_ReuseReservoirs == 0u || !reusable)
            {
                unresampledIndirect += pathWeight * Lo;
            }
            else
            {
                Lo = min(Lo, RESTIR_MAX_RADIANCE);
                float3 target = GITargetRadiance(primary, V, worldPos, hitPosition, Lo, scene.diffuseMode);
                float targetLuminance = Luminance(target);
                if (targetLuminance > 0.0)
                {
                    ReservoirUpdate(reservoir, targetLuminance / bounce.pdf, hitPosition, hit.surface.N, Lo, rng);
                    ReservoirFinalize(reservoir, targetLuminance);
                }
            }
            break;
        }
    }
    if (!all(isfinite(directRadiance)))
        directRadiance = 0.0;
    if (!all(isfinite(unresampledIndirect)))
        unresampledIndirect = 0.0;
    u_DirectLighting[pixel] = float4(directRadiance, 1.0);
    u_IndirectLighting[pixel] = float4(unresampledIndirect, 1.0);
    float4 reservoirA, reservoirB;
    PackReservoir(reservoir, reservoirA, reservoirB);
    u_ReservoirA[pixel] = reservoirA;
    u_ReservoirB[pixel] = reservoirB;
}
