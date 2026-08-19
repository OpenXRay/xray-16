#include "bindless_common.h"
#include "shared/terrain_blend.h"
#include "rt_common.h"
#include "shared/pbr_brdf.h"
#include "shared/clustered_lighting.h"
#include "shared/surface_marks.h"
#include "shared/nrd_helpers.h"
#include "shared/basecolor_pack.h"
#include "restir_gi_common.h"
#include "restir_pt_common.h"
#include "restir_di_eval.h"
#include "rt_shade_hit.h"
#include "rt_grass_alpha.h"
#include "rt_material_alpha.h"
#include "rt_visibility.h"
#include "shared/foliage_sss.h"
#include "shared/skin_sss.h"

cbuffer ReSTIRGIParams : register(b5) {
    float4x4 g_InvViewProj;
    float4x4 g_PrevViewProj;
    float4 g_CameraPos;
    float4 g_SunDir_Intensity;
    float4 g_SunColor_SkyWeight;
    float4 g_SkyColor;
    float2 g_ScreenSize;
    float g_GIIntensity;
    uint g_FrameIndex;
    uint g_IdentityStaticCount;
    uint g_TerrainBatchCount;
    uint g_SkinnedBatchStart;
    uint g_GrassBatchStart;
    uint g_DetailAtlasIndex;
    uint g_NumLights;
    uint g_WetEnabled;
    float g_WetStrength;
    float4 g_ClusterParams;
    float4 g_ClusterDepth;
    float4 g_DISampleParams;
    uint g_Bounces;
    uint g_CacheSize;
    float g_CacheCellSize;
    uint g_CacheMaxAge;
    uint g_GrassShadowEnabled;
    uint g_PadA0;
    uint g_PadA1;
    uint g_PadA2;
    float4x4 g_GrassShadowVP;
    float4x4 g_WorldToView;
    float4 g_HemiColor;
    float g_LodDist;
    float g_AmbientScale;
    float g_SunAngular;
    uint g_HudSkinnedStart;
    uint g_ParticleBatchStart;
    float g_FullWidth;
    float g_FullHeight;
    uint g_PadEnd2;
    float4x4 g_PrevInvViewProj;
    uint g_HasPrevSunVis;
    float g_CurrJitterX;
    float g_CurrJitterY;
    float g_PrevJitterX;
    float g_PrevJitterY;
    float g_WindSpeed;
    uint g_PadSun1;
    uint g_PadSun2;
};

RaytracingAccelerationStructure g_SceneTLAS : register(t1);
StructuredBuffer<RTBatchInfo> g_BatchInfo : register(t2);
ByteAddressBuffer g_MegaVB : register(t3);
ByteAddressBuffer g_MegaIB : register(t4);
TextureCube<float4> g_Sky0 : register(t5);
TextureCube<float4> g_Sky1 : register(t6);
ByteAddressBuffer g_SkinnedVB : register(t7);
ByteAddressBuffer g_SkinnedIB : register(t11);
ByteAddressBuffer g_GrassVB : register(t12);
ByteAddressBuffer g_GrassIB : register(t13);
Texture2D<float> t_Depth : register(t14);
Texture2D<float4> t_Normal : register(t15);
Texture2D<float4> t_BaseColor : register(t16);
StructuredBuffer<GPULightData> g_Lights : register(t17);
Texture2D<float> t_WetAccum : register(t18);
StructuredBuffer<uint2> g_ClusterGrid : register(t19);
StructuredBuffer<uint> g_LightIndexList : register(t20);
StructuredBuffer<uint> g_DILightIndices : register(t21);
StructuredBuffer<float> g_DILightCDF : register(t22);
Texture2D<float4> t_WorldPos : register(t23);
Texture2D<float4> t_SceneColorIn : register(t24);
Texture2D<float> t_SkyOpen : register(t25);
Texture2D<float> t_GrassShadow : register(t26);
ByteAddressBuffer g_ParticleVB : register(t27);
ByteAddressBuffer g_ParticleIB : register(t28);
Texture3D<float> t_BlueNoise : register(t29);
Texture2D<float> t_PrevSunVis : register(t30);
Texture2D<float2> t_MotionVectors : register(t31);
Texture2D<float> t_PrevDepth : register(t32);
Texture2D<float4> t_PrevNormal : register(t33);

RWTexture2D<uint4> u_PTA : register(u0);
RWTexture2D<uint4> u_PTB : register(u1);
RWTexture2D<float4> u_NoisyDiffuse : register(u2);
RWTexture2D<float4> u_NoisySpecular : register(u3);
RWTexture2D<float> u_HitDistance : register(u4);
RWTexture2D<float4> u_DirectLighting : register(u5);
RWTexture2D<float> u_SunVis : register(u6);

bool IsParticleBatch(uint batchIdx)
{
    return g_ParticleBatchStart != 0xFFFFFFFFu && batchIdx >= g_ParticleBatchStart;
}

bool IsSkinnedBatch(uint batchIdx)
{
    return g_SkinnedBatchStart != 0xFFFFFFFFu && batchIdx >= g_SkinnedBatchStart &&
           (g_GrassBatchStart == 0xFFFFFFFFu || batchIdx < g_GrassBatchStart) &&
           !IsParticleBatch(batchIdx);
}

bool IsHudSkinnedBatch(uint batchIdx)
{
    return g_HudSkinnedStart != 0xFFFFFFFFu && batchIdx >= g_HudSkinnedStart &&
           (g_GrassBatchStart == 0xFFFFFFFFu || batchIdx < g_GrassBatchStart) &&
           !IsParticleBatch(batchIdx);
}

bool IsGrassBatch(uint batchIdx)
{
    return g_GrassBatchStart != 0xFFFFFFFFu && batchIdx >= g_GrassBatchStart &&
           !IsParticleBatch(batchIdx);
}

bool IsTerrainBatch(uint batchIdx)
{
    return batchIdx >= g_IdentityStaticCount &&
           batchIdx < g_IdentityStaticCount + g_TerrainBatchCount;
}

float3 SampleSky(float3 dir)
{
    float w = g_SunColor_SkyWeight.w;
    float3 s0 = g_Sky0.SampleLevel(smp_linear, dir, 0).rgb;
    float3 s1 = g_Sky1.SampleLevel(smp_linear, dir, 0).rgb;
    return lerp(s0, s1, w) * g_SkyColor.rgb * 0.80;
}

float3 SampleSkyDiffuse(float3 dir)
{
    float w = g_SunColor_SkyWeight.w;
    float3 s0 = g_Sky0.SampleLevel(smp_linear, dir, 4.0).rgb;
    float3 s1 = g_Sky1.SampleLevel(smp_linear, dir, 4.0).rgb;
    return lerp(s0, s1, w) * g_SkyColor.rgb * 0.80;
}

float SampleGrassShadow(float3 worldPos)
{
    if (g_GrassShadowEnabled == 0)
        return 1.0;
    float4 shadowPos = mul(g_GrassShadowVP, float4(worldPos, 1.0));
    float3 shadowCoord = shadowPos.xyz / max(abs(shadowPos.w), 1e-6);
    if (any(shadowCoord.xy < 0.0) || any(shadowCoord.xy > 1.0) ||
        shadowCoord.z < 0.0 || shadowCoord.z > 1.0)
        return 1.0;
    uint width, height;
    t_GrassShadow.GetDimensions(width, height);
    int2 texel = clamp(int2(shadowCoord.xy * float2(width, height)), int2(0, 0), int2(width, height) - 1);
    float blockerDepth = t_GrassShadow.Load(int3(texel, 0));
    return shadowCoord.z <= blockerDepth + 0.0015 ? 1.0 : 0.0;
}

float TraceSoftShadowSun(float3 origin, float3 sunDir, float viewDist, uint2 pixel)
{
    float2 giSize = max(g_ScreenSize, 1.0);
    float2 fullSize = max(float2(g_FullWidth, g_FullHeight), 1.0);
    float2 jGi = float2(g_CurrJitterX, -g_CurrJitterY) * (giSize / fullSize);
    uint2 seedPx = uint2(clamp(int2(pixel) - int2(round(jGi)), int2(0, 0), int2(giSize) - 1));
    float u0 = SampleSTBN(t_BlueNoise, seedPx, 0, 0);
    float u1 = SampleSTBN(t_BlueNoise, seedPx, 0, 1);
    float contact = saturate(viewDist / 16.0);
    float radius = clamp(g_SunAngular, 0.001, 0.05) * lerp(0.2, 1.0, contact);
    float ang = u1 * 6.2831853;
    float r = sqrt(u0) * radius;
    float3 up = abs(sunDir.y) < 0.99 ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, sunDir));
    float3 bitangent = cross(sunDir, tangent);
    float3 dir = normalize(sunDir + tangent * (cos(ang) * r) + bitangent * (sin(ang) * r));
    return EvaluateSunVisibilityWithGrass(
        g_SceneTLAS, g_BatchInfo, g_MegaVB, g_MegaIB, g_GrassVB, g_GrassIB,
        origin, dir, 10000.0,
        g_IdentityStaticCount, g_TerrainBatchCount, g_SkinnedBatchStart, g_GrassBatchStart,
        g_ParticleBatchStart, g_DetailAtlasIndex, g_HudSkinnedStart,
        t_BlueNoise, seedPx, 0, RT_MASK_SHADOW);
}

float FilterSunVisibility(float rawVis, uint2 pixel, float2 giSize, float2 fullSize, float depth, float3 N, float3 worldPos, bool charSurf)
{
    float vis = rawVis;
    float wSum = 1.0;
    if (g_HasPrevSunVis == 0 || charSurf)
        return rawVis;
    int2 fullPx = RestirFullPixel(pixel, giSize, fullSize);
    float2 uv = (float2(pixel) + 0.5) / giSize;
    float2 motion = t_MotionVectors.Load(int3(fullPx, 0));
    float motionPx = length(motion * giSize);
    bool still = motionPx < 0.4;
    float2 prevUV = uv + motion;
    float viewDist = length(worldPos - g_CameraPos.xyz);
    float histW = still ? 16.0 : lerp(8.0, 2.0, saturate(motionPx / 4.0));
    histW *= saturate(1.0 - g_WindSpeed * 0.08);
    bool histOk = !any(prevUV < 0.0) && !any(prevUV >= 1.0);
    int2 prevPixel = clamp(int2(round(prevUV * giSize)), int2(0, 0), int2(giSize) - 1);
    int2 prevFull = clamp(int2(round(prevUV * fullSize)), int2(0, 0), int2(fullSize) - 1);
    float prevDepth = histOk ? t_PrevDepth.Load(int3(prevFull, 0)) : 0.0;
    if (!histOk || prevDepth <= 0.0 || prevDepth >= 1.0)
        histW = still ? 8.0 : 1.0;
    else {
        float3 prevN = normalize(t_PrevNormal.Load(int3(prevFull, 0)).xyz);
        float2 prevNdcUV = (float2(prevFull) + 0.5) / fullSize;
        float3 prevWorld = ReconstructWorldPosReverseZ(prevNdcUV, prevDepth, g_PrevInvViewProj);
        float skyOpenC = saturate(RestirLoadTex1(t_SkyOpen, pixel, giSize, fullSize));
        float skyOpenP = saturate(t_SkyOpen.Load(int3(prevFull, 0)));
        if (abs(skyOpenC - skyOpenP) > 0.25)
            histW = 0.0;
        else if (length(worldPos - prevWorld) >= 0.08 * max(viewDist, 1.0) || dot(N, prevN) < 0.94)
            histW = still ? 8.0 : 1.0;
    }
    vis += t_PrevSunVis.Load(int3(prevPixel, 0)) * histW;
    wSum += histW;
    return vis / wSum;
}

float TraceShadowRay(float3 origin, float3 dir, float tMax)
{
    return TraceVisibilityAtten(
        g_SceneTLAS, g_BatchInfo, g_MegaVB, g_MegaIB, g_GrassVB, g_GrassIB,
        g_ParticleVB, g_ParticleIB,
        origin, dir, max(tMax, 0.001), RT_MASK_SHADOW,
        g_IdentityStaticCount, g_TerrainBatchCount, g_SkinnedBatchStart, g_GrassBatchStart,
        g_ParticleBatchStart, g_DetailAtlasIndex, false, 0.0, g_HudSkinnedStart,
        t_BlueNoise, uint2(0, 0), g_FrameIndex);
}

float TraceBounceSunVis(float3 origin, float3 dir, float tMax, uint2 pixel)
{
    return EvaluateSunVisibilityWithGrass(
        g_SceneTLAS, g_BatchInfo, g_MegaVB, g_MegaIB, g_GrassVB, g_GrassIB,
        origin, dir, max(tMax, 0.001),
        g_IdentityStaticCount, g_TerrainBatchCount, g_SkinnedBatchStart, g_GrassBatchStart,
        g_ParticleBatchStart, g_DetailAtlasIndex, g_HudSkinnedStart,
        t_BlueNoise, pixel, g_FrameIndex, RT_MASK_SHADOW);
}

float4 SampleTerrainTexture(uint index, float2 uv)
{
    if (index == INVALID_TEXTURE_INDEX)
        return float4(0.5, 0.5, 0.5, 1.0);
    return GetBindlessTexture(index).SampleLevel(smp_linear, uv, 0);
}

float3 SampleTerrainAlbedo(TerrainMaterialData mat, float2 uv)
{
    float2 baseUV = uv;
    float2 detailUV = uv * mat.detailScale;
    float4 baseSample = SampleTerrainTexture(mat.baseAlbedoIndex, baseUV);
    float4 mask = TerrainNormalizeMask(SampleTerrainTexture(mat.blendMaskIndex, baseUV));
    float4 detailR = SampleTerrainTexture(mat.detailR_Index, detailUV);
    float4 detailG = SampleTerrainTexture(mat.detailG_Index, detailUV);
    float4 detailB = SampleTerrainTexture(mat.detailB_Index, detailUV);
    float4 detailA = SampleTerrainTexture(mat.detailA_Index, detailUV);
    float3 blendedDetail = TerrainBlendRGB(detailR.rgb, detailG.rgb, detailB.rgb, detailA.rgb, mask);
    return baseSample.rgb * blendedDetail * 2.0;
}

struct BounceHit {
    float3 position;
    float3 normal;
    float3 geoNormal;
    float3 albedo;
    float3 baked;
    float3 emissive;
    float metallic;
    float roughness;
    float sunOcc;
    float t;
    bool valid;
};

BounceHit TraceBounce(float3 origin, float3 direction)
{
    BounceHit result;
    result.valid = false;
    result.baked = 0;
    result.emissive = 0;
    result.sunOcc = 1.0;
    result.albedo = 0;
    result.metallic = 0;
    result.roughness = 1;
    result.t = 0;
    result.position = origin;
    result.normal = -direction;
    result.geoNormal = -direction;

    float3 rayOrigin = origin;
    for (uint skip = 0; skip < 4; skip++) {
        RayDesc ray;
        ray.Origin = rayOrigin;
        ray.Direction = direction;
        ray.TMin = 0.001;
        ray.TMax = 10000.0;
        RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
        q.TraceRayInline(g_SceneTLAS, RAY_FLAG_NONE, RT_MASK_GI, ray);
        while (q.Proceed()) {
            if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE) {
                uint candBatch = q.CandidateInstanceID() + q.CandidateGeometryIndex();
                if (IsParticleBatch(candBatch))
                    continue;
                if (IsGrassBatch(candBatch)) {
                    if (GrassTexelOpaque(g_GrassVB, g_GrassIB, g_BatchInfo, candBatch,
                            q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics(),
                            g_DetailAtlasIndex))
                        q.CommitNonOpaqueTriangleHit();
                } else if (IsSkinnedBatch(candBatch)) {
                    if (SkinnedMaterialOpaque(g_SkinnedVB, g_SkinnedIB, g_BatchInfo, candBatch,
                            q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics()))
                        q.CommitNonOpaqueTriangleHit();
                } else if (MegaMaterialOpaque(g_MegaVB, g_MegaIB, g_BatchInfo, candBatch,
                        q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics()) ||
                    MegaEmissiveHit(g_MegaVB, g_MegaIB, g_BatchInfo, candBatch,
                        q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics())) {
                    q.CommitNonOpaqueTriangleHit();
                }
            }
        }
        if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
            return result;

        uint batchIdx = q.CommittedInstanceID() + q.CommittedGeometryIndex();
        if (IsParticleBatch(batchIdx)) {
            rayOrigin = rayOrigin + direction * (q.CommittedRayT() + 0.002);
            continue;
        }
        RTBatchInfo info = g_BatchInfo[batchIdx];
        uint primIdx = q.CommittedPrimitiveIndex();
        float2 bary = q.CommittedTriangleBarycentrics();
        float3x4 objectToWorld = q.CommittedObjectToWorld3x4();

        float3 hitN, geoN;
        float2 hitUV;
        float hemi = 0.55;
        float2 lmUV = 0;
        if (IsGrassBatch(batchIdx)) {
            hitUV = GetSkinnedHitUV(g_GrassVB, g_GrassIB, info, primIdx, bary);
            hitN = GetSkinnedHitNormal(g_GrassVB, g_GrassIB, info, primIdx, bary);
            geoN = GetSkinnedHitGeoNormal(g_GrassVB, g_GrassIB, info, primIdx);
        } else if (IsSkinnedBatch(batchIdx)) {
            hitUV = GetSkinnedHitUV(g_SkinnedVB, g_SkinnedIB, info, primIdx, bary);
            hitN = GetSkinnedHitNormal(g_SkinnedVB, g_SkinnedIB, info, primIdx, bary);
            geoN = GetSkinnedHitGeoNormal(g_SkinnedVB, g_SkinnedIB, info, primIdx);
        } else {
            hitUV = GetHitUV(g_MegaVB, g_MegaIB, info, primIdx, bary);
            hitN = TransformNormalToWorld(GetHitNormal(g_MegaVB, g_MegaIB, info, primIdx, bary), objectToWorld);
            geoN = TransformNormalToWorld(GetHitGeometricNormal(g_MegaVB, g_MegaIB, info, primIdx), objectToWorld);
            hemi = GetHitHemi(g_MegaVB, g_MegaIB, info, primIdx, bary);
            lmUV = GetHitLightmapUV(g_MegaVB, g_MegaIB, info, primIdx, bary);
        }
        if (dot(geoN, direction) > 0) geoN = -geoN;
        if (dot(hitN, geoN) < 0) hitN = -hitN;

        float3 albedo = float3(0.5, 0.5, 0.5);
        float metallic = 0;
        float roughness = 1.0;
        float3 baked = 0;
        float sunOcc = 1.0;

        if (IsGrassBatch(batchIdx)) {
            if (g_DetailAtlasIndex > 0)
                albedo = GetBindlessTexture(g_DetailAtlasIndex).SampleLevel(smp_linear, hitUV, 0).rgb;
            else
                albedo = lerp(float3(0.08, 0.18, 0.03), float3(0.15, 0.35, 0.06), 1.0 - hitUV.y);
            baked = ShadeBakedFromHemi(0.55, albedo, g_HemiColor.rgb);
        } else if (IsTerrainBatch(batchIdx)) {
            TerrainMaterialData tmat = g_TerrainMaterials[info.materialID];
            float hitDist = q.CommittedRayT();
            if (hitDist > g_LodDist * 0.5)
                albedo = SampleTerrainTexture(tmat.baseAlbedoIndex, hitUV).rgb;
            else
                albedo = SampleTerrainAlbedo(tmat, hitUV);
            baked = ShadeBakedFromTerrainLmap(tmat, lmUV, albedo, g_HemiColor.rgb, hemi);
        } else {
            MaterialData mat = g_Materials[info.materialID];
            float4 diffuse = SampleDiffuseLevel(mat, hitUV);
            albedo = diffuse.rgb;
            bool water = (mat.flags & MAT_FLAG_WATER) != 0;
            bool emHit = EmissiveTexelLit(mat, diffuse);
            if (!water && !emHit && !MaterialDiffuseOpaque(mat, diffuse)) {
                rayOrigin = rayOrigin + direction * (q.CommittedRayT() + 0.002);
                continue;
            }
            if ((mat.flags & MAT_FLAG_HAS_PBR) != 0) {
                float3 pbr = SamplePBR(mat, hitUV);
                metallic = pbr.r;
                roughness = pbr.g;
            }
            if (water) {
                rayOrigin = rayOrigin + direction * (q.CommittedRayT() + 0.002);
                continue;
            }
            baked = ShadeBakedFromHemi(hemi, albedo, g_HemiColor.rgb);
            if ((mat.flags & MAT_FLAG_HAS_LMAP) != 0 && mat.lmapIndex != INVALID_TEXTURE_INDEX
                && dot(lmUV, lmUV) > 1e-8)
            {
                float4 lmh = GetBindlessTexture(mat.lmapIndex).SampleLevel(smp_rtlinear, lmUV, 0);
                sunOcc = smoothstep(0.04, 0.96, saturate(lmh.g));
                baked = ShadeBakedFromHemi(max(hemi, lmh.a), albedo, g_HemiColor.rgb) * sunOcc;
            }
            if (emHit && mat.emissiveIntensity > 0.0)
                result.emissive = GlowEmissiveRgb(diffuse, mat.emissiveIntensity);
        }

        result.position = rayOrigin + direction * q.CommittedRayT();
        result.normal = hitN;
        result.geoNormal = geoN;
        result.albedo = albedo;
        result.baked = baked;
        result.metallic = metallic;
        result.roughness = roughness;
        result.sunOcc = sunOcc;
        result.t = q.CommittedRayT();
        result.valid = true;
        return result;
    }
    return result;
}

GPULightDataDI AsDI(GPULightData light)
{
    GPULightDataDI d;
    d.positionAndInvRangeSq = light.positionAndInvRangeSq;
    d.colorAndRange = light.colorAndRange;
    d.directionAndSpotScale = light.directionAndSpotScale;
    d.spotParamsAndType = light.spotParamsAndType;
    d.spotVP = light.spotVP;
    return d;
}

uint SampleDILightIS(float u, out float lightPdf)
{
    uint count = (uint)g_DISampleParams.x;
    float powerSum = g_DISampleParams.y;
    lightPdf = 0;
    if (count == 0 || powerSum <= 1e-8)
        return 0;
    float target = u * powerSum;
    uint lo = 0;
    uint hi = count;
    while (lo < hi) {
        uint mid = (lo + hi) >> 1;
        if (g_DILightCDF[mid] < target)
            lo = mid + 1;
        else
            hi = mid;
    }
    uint i = min(lo, count - 1);
    float prev = (i == 0) ? 0.0 : g_DILightCDF[i - 1];
    float w = max(g_DILightCDF[i] - prev, 1e-8);
    lightPdf = w / powerSum;
    return g_DILightIndices[i];
}

float3 ShadeLocalLightNEE(GPULightData light, float3 worldPos, float3 biasedPos, float3 N, float3 V,
    float3 albedo, float metallic, float roughness)
{
    GPULightDataDI di = AsDI(light);
    float3 L, lightColor;
    float dist;
    float atten = EvalLocalLightAttenuationDI(di, worldPos, L, dist, lightColor);
    if (atten <= 0.001)
        return 0;
    float3 lit = PBRDirectLighting(albedo, N, V, L, lightColor * atten, metallic, roughness, 1);
    if (Luminance(lit) <= 1e-6)
        return 0;
    float shadowL = TraceShadowRay(biasedPos, L, max(dist * 0.998, 0.05));
    return min(lit * shadowL, RESTIR_MAX_RADIANCE);
}

float3 ShadeClusterLights(float3 worldPos, float3 biasedPos, float3 N, float3 V,
    float3 albedo, float metallic, float roughness, int2 fullPx, bool shadeAll)
{
    float3 accum = 0;
    if (g_NumLights == 0)
        return accum;
    float linearDepth = max(abs(mul(g_WorldToView, float4(worldPos, 1.0)).z), 0.01);
    uint clusterIdx = GetClusterIndex(float2(fullPx) + 0.5, linearDepth, g_ClusterParams.xyz, g_ClusterDepth);
    uint2 clusterData = g_ClusterGrid[clusterIdx];
    uint lightOffset = clusterData.x;
    uint lightCount = min(clusterData.y, RESTIR_MAX_LIGHTS_PER_TILE);
    uint cap = shadeAll ? min(lightCount, RESTIR_MAX_CLUSTER_LIGHTS) : min(lightCount, RESTIR_MAX_CLUSTER_LIGHTS);
    for (uint ci = 0; ci < cap; ci++) {
        uint lightId = g_LightIndexList[lightOffset + ci];
        if (lightId >= g_NumLights)
            continue;
        accum += ShadeLocalLightNEE(g_Lights[lightId], worldPos, biasedPos, N, V, albedo, metallic, roughness);
    }
    return accum;
}

float3 ShadeHitNEE(BounceHit h, float3 primaryPos, float3 sunDir, float3 sunColor, uint2 pixel, inout uint rng)
{
    if (Luminance(h.albedo) < 1e-4 && any(h.emissive > 0))
        return min(h.emissive, RESTIR_MAX_RADIANCE);
    float3 hitBiased = h.position + h.geoNormal * 0.005;
    float3 hitV = normalize(primaryPos - h.position);
    float hitShadow = TraceBounceSunVis(hitBiased, sunDir, 10000.0, pixel) * SampleGrassShadow(hitBiased) * h.sunOcc;
    float3 Lo = ShadeHitDirect(h.albedo, h.normal, hitV, h.metallic, h.roughness, sunDir, sunColor, hitShadow, h.baked);
    Lo += min(h.emissive, RESTIR_MAX_RADIANCE);
    float lightPdf = 0;
    uint lid = SampleDILightIS(rand_float(rng), lightPdf);
    if (lightPdf > 1e-8 && lid < g_NumLights)
        Lo += ShadeLocalLightNEE(g_Lights[lid], h.position, hitBiased, h.normal, hitV, h.albedo, h.metallic, h.roughness);
    return min(Lo, RESTIR_MAX_RADIANCE);
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint2 pixel = id.xy;
    uint w = (uint)g_ScreenSize.x;
    uint h = (uint)g_ScreenSize.y;
    if (pixel.x >= w || pixel.y >= h)
        return;

    float2 giSize = max(g_ScreenSize, 1.0);
    float2 fullSize = max(float2(g_FullWidth, g_FullHeight), 1.0);
    float depth = RestirLoadDepth(t_Depth, pixel, giSize, fullSize);
    if (depth <= 0.0 || depth >= 1.0) {
        u_PTA[pixel] = 0;
        u_PTB[pixel] = 0;
        u_NoisyDiffuse[pixel] = 0;
        u_NoisySpecular[pixel] = 0;
        u_HitDistance[pixel] = 0;
        u_DirectLighting[pixel] = 0;
        u_SunVis[pixel] = 0;
        return;
    }

    float2 uv = (float2(pixel) + 0.5) / giSize;
    float4 wp = RestirLoadTex4(t_WorldPos, pixel, giSize, fullSize);
    float4 bc = RestirLoadTex4(t_BaseColor, pixel, giSize, fullSize);
    float4 nd = RestirLoadTex4(t_Normal, pixel, giSize, fullSize);
    float3 worldPos = ResolveGBufferWorldPos(uv, depth, wp, g_InvViewProj);
    float3 N = normalize(nd.xyz);
    float roughness = max(abs(nd.w), MIN_ROUGHNESS);
    float surf = SurfMarkFromGBuffer(wp.w, bc.a);
    float sssMask = 0;
    float metallic = UnpackGBufferMetallic(bc.a, IsHudSurfMark(surf) || IsCharSurfMark(surf), sssMask);
    bool isVeg = IsFoliageSurfMark(surf);
    bool isHud = IsHudSurfMark(surf);
    float3 albedo = bc.rgb;
    if (g_WetEnabled != 0 && !isHud && !IsCharSurfMark(surf)) {
        float wet = saturate(RestirLoadTex1(t_WetAccum, pixel, giSize, fullSize) * g_WetStrength);
        albedo = lerp(albedo, albedo * 0.35, wet);
        roughness = lerp(roughness, max(roughness * 0.35, 0.08), wet);
    }
    float3 V = normalize(g_CameraPos.xyz - worldPos);
    float3 biased = worldPos + N * (isHud ? 0.03 : 0.01);
    float viewDist = length(worldPos - g_CameraPos.xyz);
    uint rng = pcg_hash(pixel.x + pixel.y * 19891u + g_FrameIndex * 33461u);
    uint pathSeed = rng;
    int2 fullPx = RestirFullPixel(pixel, giSize, fullSize);

    float3 sunDir = normalize(-g_SunDir_Intensity.xyz);
    float3 sunCol = g_SunColor_SkyWeight.xyz * g_SunDir_Intensity.w;
    float skyOpen = saturate(RestirLoadTex1(t_SkyOpen, pixel, giSize, fullSize));
    float rawSunVis = TraceSoftShadowSun(biased, sunDir, viewDist, pixel);
    rawSunVis *= SampleGrassShadow(biased);
    float sunVis = FilterSunVisibility(rawSunVis, pixel, giSize, fullSize, depth, N, worldPos, IsCharSurfMark(surf));
    u_SunVis[pixel] = sunVis;

    float3 direct = 0;
    if (sunVis > 0.001) {
        float3 Ns = N;
        if (isVeg && dot(N, sunDir) < 0.0)
            Ns = -N;
        float3 sunRadiance = sunCol * (isVeg ? 1.55 : 1.35) * sunVis;
        direct += PBRDirectLighting(albedo, Ns, V, sunDir, sunRadiance, metallic, roughness, 1u);
        if (isVeg && sssMask > 0.01)
            direct += EvaluateFoliageSSS(albedo, Ns, V, sunDir, sunCol * 1.35, sunVis, LeafSSSTint(),
                saturate(0.35 + sssMask * 0.3), sssMask);
    }

    bool stableDirect = IsCharSurfMark(surf) || IsInteriorSurfMark(surf);
    direct += ShadeClusterLights(worldPos, biased, N, V, albedo, metallic, roughness, fullPx, stableDirect);
    if (!stableDirect) {
        uint diCount = (uint)g_DISampleParams.x;
        uint diCandidates = min((uint)g_DISampleParams.z, RESTIR_MAX_LOCAL_LIGHT_SAMPLES);
        for (uint li = 0; li < diCandidates; li++) {
            if (diCount == 0)
                break;
            float lightPdf = 0;
            uint lightId = SampleDILightIS(rand_float(rng), lightPdf);
            if (lightPdf <= 1e-8 || lightId >= g_NumLights)
                continue;
            direct += ShadeLocalLightNEE(g_Lights[lightId], worldPos, biased, N, V, albedo, metallic, roughness);
        }
    }

    if (isVeg && skyOpen > 0.01) {
        float skyVis = TraceBounceSunVis(biased + float3(0, 0.02, 0), float3(0, 1, 0), 10000.0, pixel);
        if (skyVis > 0.001) {
            float wrap = saturate(abs(N.y) * 0.35 + 0.65);
            float3 LoSky = SampleSkyDiffuse(float3(0, 1, 0)) * skyVis;
            float3 F0v = CalculateF0(albedo, metallic);
            float3 kD = (1.0 - F_Schlick(wrap, F0v)) * (1.0 - metallic);
            direct += min(LoSky * kD * albedo * wrap, RESTIR_MAX_RADIANCE);
        }
    }

    PTReservoir res = EmptyPTReservoir();
    float3 noisyDiff = 0;
    float3 noisySpec = 0;
    float hitDist = 0;
    float3 throughput = 1;
    float3 kd = albedo * (1.0 - metallic);
    float3 origin = biased;
    float3 dirN = N;
    if (isVeg && N.y < 0.0)
        dirN = -N;
    float3 dir = cosine_weighted_hemisphere(float2(rand_float(rng), rand_float(rng)), dirN);
    uint bounces = clamp(g_Bounces, 1u, 8u);
    bool pickedRc = false;
    float3 bounceN = N;
    float bounceRough = roughness;

    [loop]
    for (uint b = 0; b < bounces; b++) {
        BounceHit hit = TraceBounce(origin, dir);
        float3 Lo;
        float3 hpos;
        float3 hN;
        if (!hit.valid) {
            Lo = SampleSkyDiffuse(dir);
            hpos = origin + dir * 80.0;
            hN = -dir;
        } else {
            Lo = ShadeHitNEE(hit, origin, sunDir, sunCol, pixel, rng);
            hpos = hit.position;
            hN = hit.normal;
        }
        Lo = min(Lo, RESTIR_MAX_RADIANCE);
        noisyDiff += throughput * kd * Lo;
        float alpha = max(bounceRough * bounceRough, 0.04);
        bool rcOk = (b == 0) ? (alpha >= RESTIR_PT_RC_ALPHA) : PTFootprintOk(hpos, origin, bounceN, alpha);
        if (!pickedRc && rcOk) {
            res.rcPos = hpos;
            res.rcN = hN;
            res.Lo = Lo;
            res.hitDist = length(hpos - worldPos);
            res.flags = PackPTFlags(b + 2, 1, 0);
            res.seed = pathSeed;
            res.M = 1;
            res.W = 1;
            res.targetPdf = max(Luminance(Lo), 1e-4);
            pickedRc = true;
        }
        if (b == 0) {
            float3 F0 = CalculateF0(albedo, metallic);
            float3 Fenv = NRD_EnvironmentTerm_Rtg(F0, abs(dot(N, V)), roughness);
            noisySpec += Lo * Fenv;
            hitDist = length(hpos - worldPos);
        }
        if (!hit.valid)
            break;
        throughput *= kd;
        kd = hit.albedo * (1.0 - hit.metallic);
        float p = max(max(throughput.r, throughput.g), throughput.b);
        if (b >= 2) {
            if (rand_float(rng) > p)
                break;
            throughput /= max(p, 1e-3);
        }
        origin = hpos + hit.geoNormal * 0.005;
        bounceN = hN;
        bounceRough = hit.roughness;
        dir = cosine_weighted_hemisphere(float2(rand_float(rng), rand_float(rng)), hN);
    }

    if (!pickedRc && any(noisyDiff > 0)) {
        res.rcPos = worldPos + V * 4.0;
        res.rcN = N;
        res.Lo = noisyDiff;
        res.hitDist = 4.0;
        res.flags = PackPTFlags(2, 0, 0);
        res.seed = pathSeed;
        res.M = 1;
        res.W = 1;
        res.targetPdf = max(Luminance(res.Lo), 1e-4);
    }

    uint4 A, B;
    PackPTReservoir(res, A, B);
    u_PTA[pixel] = A;
    u_PTB[pixel] = B;
    u_NoisyDiffuse[pixel] = float4(min(noisyDiff, RESTIR_MAX_RADIANCE), 1);
    u_NoisySpecular[pixel] = float4(min(noisySpec, RESTIR_MAX_RADIANCE), hitDist);
    u_HitDistance[pixel] = hitDist;
    u_DirectLighting[pixel] = float4(min(direct, RESTIR_MAX_RADIANCE), 1);
}
