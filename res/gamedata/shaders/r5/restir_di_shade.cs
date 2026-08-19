#include "bindless_common.h"
#include "rt_common.h"
#include "rt_visibility.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "shared/foliage_sss.h"
#include "shared/skin_sss.h"
#include "shared/clustered_lighting.h"
#include "restir_gi_common.h"
#include "restir_di_common.h"
#include "restir_di_eval.h"
#include "shared/surface_marks.h"

cbuffer ReSTIRDIShadeParams : register(b5) {
    float4x4 g_InvViewProj;
    float4x4 g_WorldToView;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    uint g_GrassBatchStart;
    uint g_DetailAtlasIndex;
    float4 g_ClusterParams;
    float4 g_ClusterDepth;
    uint g_IdentityStaticCount;
    uint g_TerrainBatchCount;
    uint g_SkinnedBatchStart;
    uint g_ParticleBatchStart;
    uint g_HudSkinnedStart;
    float g_FullWidth;
    float g_FullHeight;
    uint g_FrameIndex;
};

Texture3D<float> t_BlueNoise : register(t21);
Texture2D<float> t_SkyOpen : register(t22);

RaytracingAccelerationStructure g_SceneTLAS : register(t1);
StructuredBuffer<GPULightDataDI> g_LightData : register(t20);
StructuredBuffer<uint2> g_ClusterGrid : register(t15);
StructuredBuffer<uint> g_LightIndexList : register(t16);
StructuredBuffer<RTBatchInfo> g_BatchInfo : register(t2);
ByteAddressBuffer g_MegaVB : register(t3);
ByteAddressBuffer g_MegaIB : register(t18);
ByteAddressBuffer g_GrassVB : register(t12);
ByteAddressBuffer g_GrassIB : register(t13);
ByteAddressBuffer g_ParticleVB : register(t19);
ByteAddressBuffer g_ParticleIB : register(t17);
Texture2D<float4> t_DIReservoir : register(t0);
Texture2D<float> t_Depth : register(t14);
Texture2D<float4> t_BaseColor : register(t6);
Texture2D<float4> t_WorldPos : register(t7);
Texture2D<float4> t_Normal : register(t11);

RWTexture2D<float4> u_DirectLighting : register(u0);

float TraceShadowRayDI(float3 origin, float3 dir, float tMax, float skinnedSelfMax, uint2 pixel, uint mask)
{
    return TraceVisibilityAtten(
        g_SceneTLAS, g_BatchInfo, g_MegaVB, g_MegaIB, g_GrassVB, g_GrassIB, g_ParticleVB, g_ParticleIB,
        origin, dir, tMax, mask,
        g_IdentityStaticCount, g_TerrainBatchCount, g_SkinnedBatchStart, g_GrassBatchStart,
        g_ParticleBatchStart, g_DetailAtlasIndex, true, skinnedSelfMax, g_HudSkinnedStart,
        t_BlueNoise, pixel, g_FrameIndex);
}

float TraceHardShadowDI(float3 origin, float3 lightPos, float range, bool hudLight, bool isSpot, float skinnedSelfMax, uint2 pixel, uint mask)
{
    float3 toLight = lightPos - origin;
    float dist = length(toLight);
    if (dist < 1e-4)
        return 1.0;
    float endSkip = 0.02;
    if (!hudLight && !isSpot && range < 6.0)
        endSkip = 0.28;
    else if (!hudLight && isSpot)
        endSkip = 0.03;
    float tMax = max(dist - endSkip, 0.02);
    return TraceShadowRayDI(origin, toLight / dist, tMax, skinnedSelfMax, pixel, mask);
}

float3 ShadeOneDI(GPULightDataDI light, float3 worldPos, float3 N, float3 V, float3 albedo,
    float metallic, float roughness, float sssMask, bool vegSurf, float surfMark,
    float3 biasN, float skinnedSelfMax, uint2 pixel, float2 giSize, float2 fullSize)
{
    if (!IsLiveLightDI(light))
        return 0;

    const bool hudLight = IsHudLightDI(light);
    float3 L, lightColor;
    float dist;
    float atten = EvalLocalLightAttenuationDI(light, worldPos, L, dist, lightColor);
    if (atten <= 0.001)
        return 0;

    const bool isSpot = light.spotParamsAndType.y > 0.5;
    const bool transientPoint = IsTransientPointLightDI(light);
    float shadow = 1.0;
    if (!hudLight && !transientPoint) {
        float range = max(abs(light.colorAndRange.w), 0.5);
        shadow = TraceHardShadowDI(
            biasN, light.positionAndInvRangeSq.xyz, range,
            false, isSpot, skinnedSelfMax, pixel, RT_MASK_SHADOW);
    }
    if (shadow <= 0.001)
        return 0;

    float3 Ns = N;
    if (vegSurf && dot(N, L) < 0.0)
        Ns = -N;
    float3 shaded = PBRDirectLighting(albedo, Ns, V, L, lightColor * atten * shadow, metallic, roughness, 1);
    if (vegSurf && sssMask > 0.01) {
        float sssThickness = saturate(0.35 + sssMask * 0.3);
        shaded += EvaluateFoliageSSS(
            albedo, Ns, V, L, lightColor * atten, shadow,
            LeafSSSTint(), sssThickness, sssMask);
    }
    return min(shaded, RESTIR_MAX_RADIANCE);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float2 giSize = g_ScreenSize;
    float2 fullSize = float2(g_FullWidth, g_FullHeight);
    if (fullSize.x < 1.0 || fullSize.y < 1.0) {
        uint fw = 0, fh = 0;
        t_Depth.GetDimensions(fw, fh);
        fullSize = float2(max(fw, 1u), max(fh, 1u));
    }
    int2 fullPx = RestirFullPixel(pixel, giSize, fullSize);

    float depth = RestirLoadDepth(t_Depth, pixel, giSize, fullSize);
    if (depth <= 0.0)
        return;

    const uint numLights = (uint)g_ClusterParams.w;
    if (numLights == 0)
        return;

    float4 worldPosData = RestirLoadTex4(t_WorldPos, pixel, giSize, fullSize);
    float2 uv = (float2(pixel) + 0.5) / giSize;
    float3 worldPos = ResolveGBufferWorldPos(uv, depth, worldPosData, g_InvViewProj);
    float4 baseColorData = RestirLoadTex4(t_BaseColor, pixel, giSize, fullSize);
    const float surfMark = SurfMarkFromGBuffer(worldPosData.w, baseColorData.a);
    const bool hudSurf = IsHudSurfMark(surfMark);
    const bool charSurf = IsCharSurfMark(surfMark);
    const float skinnedSelfMax = hudSurf ? 0.12 : (charSurf ? 0.06 : 0.0);
    float4 normalData = RestirLoadTex4(t_Normal, pixel, giSize, fullSize);
    float3 N = normalize(normalData.xyz);
    float roughness = max(abs(normalData.w), MIN_ROUGHNESS);
    float3 albedo = max(baseColorData.rgb, 0.0);
    float sssMask = 0.0;
    const bool vegSurf = IsVegSurfMark(surfMark);
    float metallic = UnpackGBufferMetallic(baseColorData.a, vegSurf, sssMask);
    float3 V = normalize(g_CameraPos.xyz - worldPos);
    float3 biasN = worldPos + N * (skinnedSelfMax > 0.0 ? 0.03 : 0.012);

    float linearDepth = max(abs(mul(g_WorldToView, float4(worldPos, 1.0)).z), 0.01);
    uint clusterIdx = GetClusterIndex(float2(fullPx) + 0.5, linearDepth, g_ClusterParams.xyz, g_ClusterDepth);
    uint2 clusterData = g_ClusterGrid[clusterIdx];
    uint lightOffset = clusterData.x;
    uint tileLightCount = min(clusterData.y, RESTIR_MAX_LIGHTS_PER_TILE);
    uint shadeCount = min(tileLightCount, RESTIR_MAX_CLUSTER_LIGHTS);

    float3 accum = 0;
    float viewDist = length(g_CameraPos.xyz - worldPos);
    bool stableDirect = charSurf || IsInteriorSurfMark(surfMark) || viewDist < 4.0;
    DIReservoir di = UnpackDIReservoir(t_DIReservoir.Load(int3(pixel, 0)));
    if (stableDirect) {
        for (uint li = 0; li < shadeCount; li++) {
            uint lightIdx = g_LightIndexList[lightOffset + li];
            if (lightIdx >= numLights)
                continue;
            accum += ShadeOneDI(g_LightData[lightIdx], worldPos, N, V, albedo,
                metallic, roughness, sssMask, vegSurf, surfMark, biasN, skinnedSelfMax,
                pixel, giSize, fullSize);
        }
    } else if (IsDIReservoirValid(di) && di.lightIndex < numLights) {
        accum = ShadeOneDI(g_LightData[di.lightIndex], worldPos, N, V, albedo,
            metallic, roughness, sssMask, vegSurf, surfMark, biasN, skinnedSelfMax,
            pixel, giSize, fullSize) * di.W;
    } else {
        for (uint li = 0; li < shadeCount; li++) {
            uint lightIdx = g_LightIndexList[lightOffset + li];
            if (lightIdx >= numLights)
                continue;
            accum += ShadeOneDI(g_LightData[lightIdx], worldPos, N, V, albedo,
                metallic, roughness, sssMask, vegSurf, surfMark, biasN, skinnedSelfMax,
                pixel, giSize, fullSize);
        }
    }

    accum = min(accum, RESTIR_MAX_RADIANCE);
    if (Luminance(accum) <= 1e-6)
        return;

    float3 direct = u_DirectLighting[pixel].rgb + accum;
    u_DirectLighting[pixel] = float4(direct, 1.0);
}
