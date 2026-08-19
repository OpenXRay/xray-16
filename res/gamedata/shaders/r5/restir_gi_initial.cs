#include "bindless_common.h"
#include "shared/terrain_blend.h"
#include "rt_common.h"
#include "shared/pbr_brdf.h"
#include "shared/clustered_lighting.h"
#include "shared/surface_marks.h"
#include "shared/nrd_helpers.h"
#include "shared/basecolor_pack.h"
#include "restir_gi_common.h"
#include "restir_di_common.h"
#include "rt_irradiance_cache.h"
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
Texture2D<float4> t_WorldPos : register(t23);
Texture2D<float4> t_SceneColorIn : register(t24);
StructuredBuffer<GPULightData> g_Lights : register(t17);
Texture2D<float> t_WetAccum : register(t18);
Texture2D<float> t_SkyOpen : register(t25);
Texture2D<float> t_GrassShadow : register(t26);
ByteAddressBuffer g_ParticleVB : register(t27);
ByteAddressBuffer g_ParticleIB : register(t28);
Texture3D<float> t_BlueNoise : register(t29);
Texture2D<float> t_PrevSunVis : register(t30);
Texture2D<float2> t_MotionVectors : register(t31);
Texture2D<float> t_PrevDepth : register(t32);
Texture2D<float4> t_PrevNormal : register(t33);
StructuredBuffer<uint2> g_ClusterGrid : register(t19);
StructuredBuffer<uint> g_LightIndexList : register(t20);
StructuredBuffer<uint> g_DILightIndices : register(t21);
StructuredBuffer<float> g_DILightCDF : register(t22);

RWTexture2D<float4> u_DirectLighting : register(u0);
RWStructuredBuffer<uint4> u_Reservoir : register(u1);
RWTexture2D<float4> u_NoisyDiffuse : register(u2);
RWTexture2D<float4> u_NoisySpecular : register(u3);
RWTexture2D<float> u_HitDistance : register(u4);
RWTexture2D<float4> u_DIReservoir : register(u5);
RWStructuredBuffer<IrradianceCacheEntry> u_IrradianceCache : register(u6);
RWTexture2D<float4> u_SpecReservoirA : register(u7);
RWTexture2D<float4> u_SpecReservoirB : register(u8);
RWTexture2D<float> u_SunVis : register(u9);

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

float3 SampleSkySpec(float3 dir, float roughness)
{
    float mip = saturate(roughness) * 5.0;
    float w = g_SunColor_SkyWeight.w;
    float3 s0 = g_Sky0.SampleLevel(smp_linear, dir, mip).rgb;
    float3 s1 = g_Sky1.SampleLevel(smp_linear, dir, mip).rgb;
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

float TraceSoftShadowSun(float3 origin, float3 sunDir, float viewDist, uint2 pixel, float skyOpen)
{
    uint mask = RT_MASK_SHADOW;
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
        t_BlueNoise, seedPx, 0, mask);
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
        if (abs(skyOpenC - skyOpenP) > 0.25) {
            histW = 0.0;
            still = false;
        }
        else if (length(worldPos - prevWorld) >= 0.08 * max(viewDist, 1.0) || dot(N, prevN) < 0.94)
            histW = still ? 8.0 : 1.0;
    }
    vis += t_PrevSunVis.Load(int3(prevPixel, 0)) * histW;
    wSum += histW;
    if (still) {
        float harden = saturate(viewDist / max(g_LodDist, 1.0));
        float rad = lerp(1.0, 2.0, harden);
        const int2 baseOff[4] = { int2(-1, -1), int2(1, -1), int2(-1, 1), int2(1, 1) };
        [unroll] for (uint i = 0; i < 4u; i++)
        {
            int2 np = int2(pixel) + int2(round(float2(baseOff[i]) * rad));
            if (np.x < 0 || np.y < 0 || np.x >= (int)giSize.x || np.y >= (int)giSize.y)
                continue;
            int2 nf = clamp(int2((float2(np) + 0.5) / giSize * fullSize), int2(0, 0), int2(fullSize) - 1);
            float nd = t_PrevDepth.Load(int3(nf, 0));
            if (nd <= 0.0 || nd >= 1.0)
                continue;
            if (abs(nd - prevDepth) / max(prevDepth, 1e-4) > 0.06)
                continue;
            float3 nN = normalize(t_PrevNormal.Load(int3(nf, 0)).xyz);
            if (dot(N, nN) < 0.94)
                continue;
            vis += t_PrevSunVis.Load(int3(np, 0));
            wSum += 1.0;
        }
    }
    return vis / wSum;
}

float3 ImportanceSampleGGXDir(float2 Xi, float3 N, float roughness)
{
    float a = max(roughness, 0.04) * max(roughness, 0.04);
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(max(1.0 - cosTheta * cosTheta, 0.0));
    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
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

float TraceVegSkyVis(float3 origin, uint2 pixel)
{
    float2 giSize = max(g_ScreenSize, 1.0);
    float2 fullSize = max(float2(g_FullWidth, g_FullHeight), 1.0);
    float2 jGi = float2(g_CurrJitterX, -g_CurrJitterY) * (giSize / fullSize);
    uint2 seedPx = uint2(clamp(int2(pixel) - int2(round(jGi)), int2(0, 0), int2(giSize) - 1));
    return EvaluateSunVisibilityWithGrass(
        g_SceneTLAS, g_BatchInfo, g_MegaVB, g_MegaIB, g_GrassVB, g_GrassIB,
        origin, float3(0.0, 1.0, 0.0), 10000.0,
        g_IdentityStaticCount, g_TerrainBatchCount, g_SkinnedBatchStart, g_GrassBatchStart,
        g_ParticleBatchStart, g_DetailAtlasIndex, g_HudSkinnedStart,
        t_BlueNoise, seedPx, 0, RT_MASK_SHADOW_MAPPED);
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
    bool isWater;
};

BounceHit TraceBounce(float3 origin, float3 direction)
{
    BounceHit result;
    result.valid = false;
    result.isWater = false;
    result.baked = 0;
    result.emissive = 0;
    result.sunOcc = 1.0;

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
        bool water = false;
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
            water = (mat.flags & MAT_FLAG_WATER) != 0;
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
            else
            {
                baked *= sunOcc;
            }
            if (emHit && mat.emissiveIntensity > 0.0)
                result.emissive = GlowEmissiveRgb(diffuse, mat.emissiveIntensity);
        }

        result.position = rayOrigin + direction * q.CommittedRayT();
        result.normal = hitN;
        result.geoNormal = geoN;
        result.albedo = albedo;
        result.baked = baked;
        if (IsGrassBatch(batchIdx) || IsTerrainBatch(batchIdx))
            result.emissive = 0;
        result.metallic = metallic;
        result.roughness = roughness;
        result.sunOcc = sunOcc;
        result.t = q.CommittedRayT();
        result.valid = true;
        result.isWater = water;
        return result;
    }
    return result;
}

float3 EvaluateLocalLight(GPULightData light, float3 worldPos, float3 N, float3 albedo, float metallic, float roughness, float3 V)
{
    float3 lightPos = light.positionAndInvRangeSq.xyz;
    float invRangeSq = abs(light.positionAndInvRangeSq.w);
    float3 toLight = lightPos - worldPos;
    float distSq = dot(toLight, toLight);
    float dist = sqrt(max(distSq, 1e-8));
    float3 L = toLight / dist;
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0)
        return 0;

    float atten = PointLightAttenuation(distSq, invRangeSq, 0.1225);
    if (light.spotParamsAndType.y > 0.5) {
        atten *= SpotLightAttenuation(toLight, light.directionAndSpotScale.xyz,
            light.directionAndSpotScale.w, light.spotParamsAndType.x);
    }
    if (atten <= 1e-5)
        return 0;

    float3 radiance = light.colorAndRange.xyz * atten;
    float3 F0 = CalculateF0(albedo, metallic);
    float3 H = normalize(V + L);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    float r = max(roughness, 0.04);
    float D = D_GGX(NdotH, r);
    float G = G_Smith(NdotV, NdotL, r);
    float3 F = F_Schlick(VdotH, F0);
    float3 spec = D * G * F / max(4.0 * NdotV * NdotL, 1e-4);
    float3 kD = (1.0 - F) * (1.0 - metallic);
    return (kD * albedo / PI + spec) * radiance * NdotL;
}

float3 ShadeLocalLightRT(
    GPULightData light, float3 worldPos, float3 biasedPos, float3 N, float3 V,
    float3 albedo, float metallic, float roughness)
{
    float3 lit = EvaluateLocalLight(light, worldPos, N, albedo, metallic, roughness, V);
    if (Luminance(lit) <= 1e-6)
        return 0;
    float3 lightPos = light.positionAndInvRangeSq.xyz;
    float3 toLight = lightPos - biasedPos;
    float dist = length(toLight);
    if (dist < 1e-4)
        return 0;
    float3 L = toLight / dist;
    float shadowL = TraceShadowRay(biasedPos, L, dist * 0.998);
    return min(lit * shadowL, RESTIR_MAX_RADIANCE);
}

float3 ShadeLocalLightUnshadowed(
    GPULightData light, float3 worldPos, float3 N, float3 V,
    float3 albedo, float metallic, float roughness)
{
    return min(EvaluateLocalLight(light, worldPos, N, albedo, metallic, roughness, V), RESTIR_MAX_RADIANCE);
}

float3 EvaluateSecondaryLo(
    BounceHit h,
    float3 primaryPos,
    float3 sunDir,
    float3 sunColor,
    float primaryViewDist,
    float primaryOutdoor,
    uint2 pixel,
    inout uint rngState)
{
    if (Luminance(h.albedo) < 1e-4 && any(h.emissive > 0))
        return min(h.emissive, RESTIR_MAX_RADIANCE);

    float3 hitBiased = h.position + h.geoNormal * 0.005;
    float3 hitV = normalize(primaryPos - h.position);
    float hitShadow = TraceBounceSunVis(hitBiased, sunDir, 10000.0, pixel) * h.sunOcc;
    float3 Lo = ShadeHitDirect(h.albedo, h.normal, hitV, h.metallic, h.roughness, sunDir, sunColor, hitShadow, h.baked);
    Lo += min(h.emissive, RESTIR_MAX_RADIANCE);
    float farLod = saturate(primaryViewDist / max(g_LodDist, 1.0));
    if (g_Bounces >= 2u && farLod < 0.45 && rand_float(rngState) < 0.75) {
        float2 u2 = float2(rand_float(rngState), rand_float(rngState));
        float3 dir2 = cosine_weighted_hemisphere(u2, h.normal);
        BounceHit h2 = TraceBounce(hitBiased, dir2);
        if (h2.valid) {
            float3 hit2Biased = h2.position + h2.geoNormal * 0.005;
            float3 hit2V = normalize(h.position - h2.position);
            float sh2 = TraceBounceSunVis(hit2Biased, sunDir, 10000.0, pixel) * h2.sunOcc;
            float3 Lo2 = ShadeHitDirect(h2.albedo, h2.normal, hit2V, h2.metallic, h2.roughness, sunDir, sunColor, sh2, h2.baked);
            Lo2 += min(h2.emissive, RESTIR_MAX_RADIANCE);
            Lo += Lo2 * h2.albedo * (1.0 / 0.75) * 0.55;
        } else {
            Lo += SampleSkyDiffuse(dir2) * (1.0 / 0.75) * 0.7 * saturate(primaryOutdoor);
        }
    }
    return min(Lo, RESTIR_MAX_RADIANCE);
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

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    uint width = (uint)g_ScreenSize.x;
    uint height = (uint)g_ScreenSize.y;
    if (pixel.x >= width || pixel.y >= height)
        return;

    float2 giSize = g_ScreenSize;
    float2 fullSize = float2(g_FullWidth, g_FullHeight);
    if (fullSize.x < 1.0 || fullSize.y < 1.0) {
        uint fw = 0, fh = 0;
        t_Depth.GetDimensions(fw, fh);
        fullSize = float2(max(fw, 1u), max(fh, 1u));
    }
    int2 fullPx = RestirFullPixel(pixel, giSize, fullSize);

    uint pixelIdx = pixel.y * width + pixel.x;

    float depth = RestirLoadDepth(t_Depth, pixel, giSize, fullSize);
    if (depth <= 0.0 || depth >= 1.0) {
        u_DirectLighting[pixel] = 0;
        u_Reservoir[pixelIdx] = 0;
        u_NoisyDiffuse[pixel] = 0;
        u_NoisySpecular[pixel] = 0;
        u_HitDistance[pixel] = 0;
        u_DIReservoir[pixel] = PackDIReservoir(EmptyDIReservoir());
        u_SpecReservoirA[pixel] = 0;
        u_SpecReservoirB[pixel] = 0;
        u_SunVis[pixel] = 0;
        return;
    }

    float4 worldPosMark = RestirLoadTex4(t_WorldPos, pixel, giSize, fullSize);
    float4 baseColorData = RestirLoadTex4(t_BaseColor, pixel, giSize, fullSize);
    float surfMark = SurfMarkFromGBuffer(worldPosMark.w, baseColorData.a);
    const bool isHud = IsHudSurfMark(surfMark);
    const bool isVeg = IsVegSurfMark(surfMark);
    if (IsWaterSurfMark(surfMark) && !isHud) {
        u_DirectLighting[pixel] = 0;
        u_Reservoir[pixelIdx] = 0;
        u_NoisyDiffuse[pixel] = 0;
        u_NoisySpecular[pixel] = 0;
        u_HitDistance[pixel] = 0;
        u_DIReservoir[pixel] = PackDIReservoir(EmptyDIReservoir());
        u_SpecReservoirA[pixel] = 0;
        u_SpecReservoirB[pixel] = 0;
        u_SunVis[pixel] = 0;
        return;
    }

    float2 uv = (float2(pixel) + 0.5) / giSize;
    float3 worldPos = ResolveGBufferWorldPos(uv, depth, worldPosMark, g_InvViewProj);
    float4 normalData = RestirLoadTex4(t_Normal, pixel, giSize, fullSize);

    float3 N = normalize(normalData.xyz);
    float roughness = max(abs(normalData.w), MIN_ROUGHNESS);
    float3 albedo = baseColorData.rgb;
    float sssMask = 0.0;
    float metallic = UnpackGBufferMetallic(
        baseColorData.a, isVeg || isHud || IsCharSurfMark(surfMark), sssMask);

    float wet = 0;
    if (g_WetEnabled != 0 && !isHud && !IsCharSurfMark(surfMark))
        wet = saturate(RestirLoadTex1(t_WetAccum, pixel, giSize, fullSize) * g_WetStrength);
    if (wet > 0) {
        albedo = lerp(albedo, albedo * 0.35, wet);
        roughness = lerp(roughness, max(roughness * 0.25, 0.02), wet);
        metallic = lerp(metallic, min(metallic + 0.15 * wet, 1.0), wet * 0.5);
    }

    float3 V = normalize(g_CameraPos.xyz - worldPos);
    float3 sunDir = normalize(-g_SunDir_Intensity.xyz);
    float sunIntensity = g_SunDir_Intensity.w;
    float3 sunColor = g_SunColor_SkyWeight.xyz * sunIntensity;
    float3 biasedPos = worldPos + N * (isHud ? 0.03 : 0.01);
    float viewDist = length(worldPos - g_CameraPos.xyz);

    uint rng = pcg_hash(pixel.x + pixel.y * 1973u + g_FrameIndex * 26699u);
    uint rngDI = pcg_hash(pixel.x + pixel.y * 1973u + g_FrameIndex * 9176u + 3343u);
    float bakedSunOcc = saturate(RestirLoadTex4(t_SceneColorIn, pixel, giSize, fullSize).a);
    float skyOpen = saturate(RestirLoadTex1(t_SkyOpen, pixel, giSize, fullSize));
    float outdoor = (bakedSunOcc < 0.97) ? bakedSunOcc : skyOpen;
    float rawSunVis = TraceSoftShadowSun(biasedPos, sunDir, viewDist, pixel, skyOpen);
    rawSunVis *= SampleGrassShadow(biasedPos);
    float filteredSunVis = FilterSunVisibility(
        rawSunVis, pixel, giSize, fullSize, depth, N, worldPos, IsCharSurfMark(surfMark));
    u_SunVis[pixel] = filteredSunVis;
    float shadow = filteredSunVis;
    float3 direct = 0;
    float3 noisyDiff = 0;
    float3 noisySpec = 0;
    float hitDist = 0;

    if (shadow > 0.001) {
        float3 Ns = N;
        if (isVeg && dot(N, sunDir) < 0.0)
            Ns = -N;
        float3 sunRadiance = sunColor * (isVeg ? 1.55 : 1.35) * shadow;
        direct += PBRDirectLighting(albedo, Ns, V, sunDir, sunRadiance, metallic, roughness, 1u);
        if (isVeg && sssMask > 0.01) {
            float sssThickness = saturate(0.35 + sssMask * 0.3);
            direct += EvaluateFoliageSSS(
                albedo, Ns, V, sunDir, sunColor * 1.35, shadow,
                LeafSSSTint(), sssThickness, sssMask);
        }
    }

    GIReservoir reservoir = EmptyReservoir();
    GIReservoir specReservoir = EmptyReservoir();
    DIReservoir diRes = EmptyDIReservoir();

    float linearDepth = max(abs(mul(g_WorldToView, float4(worldPos, 1.0)).z), 0.01);
    uint clusterIdx = GetClusterIndex(float2(fullPx) + 0.5, linearDepth, g_ClusterParams.xyz, g_ClusterDepth);
    uint2 clusterData = g_ClusterGrid[clusterIdx];
    uint lightOffset = clusterData.x;
    uint lightCount = min(clusterData.y, RESTIR_MAX_LIGHTS_PER_TILE);
    uint clusterSamples = min(lightCount, RESTIR_MAX_CLUSTER_LIGHTS);
    for (uint ci = 0; ci < clusterSamples; ci++) {
        uint lightId = g_LightIndexList[lightOffset + ci];
        if (lightId >= g_NumLights)
            continue;
        float3 lit = ShadeLocalLightUnshadowed(g_Lights[lightId], worldPos, N, V,
            albedo, metallic, roughness);
        float pdf = Luminance(lit);
        if (pdf > 0)
            DIReservoirUpdate(diRes, pdf, lightId, pdf, rngDI);
    }

    uint diCount = (uint)g_DISampleParams.x;
    uint diCandidates = min((uint)g_DISampleParams.z, RESTIR_MAX_LOCAL_LIGHT_SAMPLES);
    for (uint li = 0; li < diCandidates; li++) {
        if (diCount == 0)
            break;
        float lightPdf = 0;
        uint lightId = SampleDILightIS(rand_float(rngDI), lightPdf);
        if (lightPdf <= 1e-8 || lightId >= g_NumLights)
            continue;
        float3 lit = ShadeLocalLightUnshadowed(g_Lights[lightId], worldPos, N, V,
            albedo, metallic, roughness);
        float pdf = Luminance(lit);
        if (pdf > 0 && lightPdf > 1e-8)
            DIReservoirUpdate(diRes, pdf / max(lightPdf, 1e-8), lightId, pdf, rngDI);
    }

    float3 F0a = CalculateF0(albedo, metallic);
    if (isVeg && skyOpen > 0.01) {
        float skyVis = TraceVegSkyVis(biasedPos + float3(0.0, 0.02, 0.0), pixel);
        if (skyVis > 0.001) {
            float wrap = saturate(abs(N.y) * 0.35 + 0.65);
            float3 LoSky = SampleSkyDiffuse(float3(0.0, 1.0, 0.0)) * skyVis;
            float3 kD = (1.0 - F_Schlick(wrap, F0a)) * (1.0 - metallic);
            direct += min(LoSky * kD * albedo * wrap, RESTIR_MAX_RADIANCE);
        }
    }
    bool diffLobeActive = any((1.0 - metallic) * albedo > 1e-4);

    if (diffLobeActive) {
        float3 Nb = N;
        if (isVeg && N.y < 0.0)
            Nb = -N;
        float2 u = float2(rand_float(rng), rand_float(rng));
        float3 bounceDir = cosine_weighted_hemisphere(u, Nb);
        float cosPDF = max(dot(bounceDir, Nb), 0) / PI;
        BounceHit hit = TraceBounce(biasedPos, bounceDir);

        if (!hit.valid && !isVeg) {
            float3 LoSky = SampleSkyDiffuse(bounceDir);
            float cosTheta = max(dot(N, bounceDir), 0);
            float3 kD = (1.0 - F_Schlick(cosTheta, F0a)) * (1.0 - metallic);
            float3 brdfCos = kD * albedo / PI * cosTheta;
            float3 gi = (cosPDF > 1e-6) ? (LoSky * brdfCos / cosPDF) : 0;
            gi = min(gi, RESTIR_MAX_RADIANCE);
            noisyDiff += gi;
            float targetLum = Luminance(LoSky * brdfCos);
            if (targetLum > 0 && cosPDF > 1e-6) {
                float w = targetLum / cosPDF;
                ReservoirUpdate(reservoir, w, worldPos + bounceDir * 1000.0, -bounceDir, LoSky, RESTIR_INVALID_ID, rng);
            }
            hitDist = 1000.0;
        }

        if (hit.valid) {
            float3 Lo = EvaluateSecondaryLo(hit, worldPos, sunDir, sunColor, viewDist, outdoor, pixel, rng);
            if (isHud)
                Lo = min(Lo, sunColor * 0.35 + 0.08);

            float3 wi = normalize(hit.position - worldPos);
            float cosTheta = isVeg ? saturate(abs(dot(N, wi)) * 0.35 + 0.65) : max(dot(N, wi), 0);
            float3 kD = (1.0 - F_Schlick(cosTheta, F0a)) * (1.0 - metallic);
            float3 brdfCos = kD * albedo / PI * cosTheta;
            float3 target = Lo * brdfCos;
            float targetLum = Luminance(target);

            if (targetLum > 0 && cosPDF > 1e-6) {
                float w = targetLum / cosPDF;
                ReservoirUpdate(reservoir, w, hit.position, hit.normal, Lo, RESTIR_INVALID_ID, rng);
                noisyDiff += min(target / cosPDF, RESTIR_MAX_RADIANCE);
            }
            hitDist = hit.t;

            if (!isVeg && skyOpen > 0.02) {
                float2 uSky = float2(rand_float(rng), rand_float(rng));
                float3 skyDir = cosine_weighted_hemisphere(uSky, N);
                float skyPDF = max(dot(skyDir, N), 0) / PI;
                BounceHit skyHit = TraceBounce(biasedPos, skyDir);
                if (!skyHit.valid && skyPDF > 1e-6) {
                    float3 LoSky = SampleSkyDiffuse(skyDir);
                    float skyCos = max(dot(N, skyDir), 0);
                    float3 skyKd = (1.0 - F_Schlick(skyCos, F0a)) * (1.0 - metallic);
                    float3 skyBrdf = skyKd * albedo / PI * skyCos;
                    float skyTarget = Luminance(LoSky * skyBrdf);
                    if (skyTarget > 0)
                        ReservoirUpdate(reservoir, skyTarget / skyPDF, worldPos + skyDir * 1000.0, -skyDir, LoSky, RESTIR_INVALID_ID, rng);
                }
            }
        }

        if (reservoir.M > 0) {
            float3 selW = normalize(reservoir.samplePos - worldPos);
            float selCos = isVeg ? saturate(abs(dot(N, selW)) * 0.35 + 0.65) : max(dot(N, selW), 0);
            float3 selKd = (1.0 - F_Schlick(selCos, F0a)) * (1.0 - metallic);
            float selTarget = Luminance(reservoir.Lo * selKd * albedo / PI * selCos);
            reservoir.W = (selTarget > 1e-8) ? min(reservoir.w_sum / (selTarget * (float)reservoir.M), 4.0) : 0;
        }
        if (g_CacheSize > 0 && any(noisyDiff > 0))
            UpdateIrradianceCache(u_IrradianceCache, worldPos,
                min(noisyDiff, RESTIR_MAX_RADIANCE),
                g_CacheCellSize, g_CacheSize, g_FrameIndex, 1.0);
    }

    float3 Fenv = NRD_EnvironmentTerm_Rtg(F0a, abs(dot(N, V)), roughness);
    float sampleRough = max(roughness, 0.06);
    float3 specular = 0;
    float specHitDist = 0;
    float3 bestSamplePos = worldPos;
    float3 bestSampleN = N;
    float3 bestLo = 0;
    float accW = 0;
    {
        float2 uSpec = float2(rand_float(rng), rand_float(rng));
        float3 H = ImportanceSampleGGXDir(uSpec, N, sampleRough);
        float3 R = normalize(2.0 * max(dot(V, H), 0.0) * H - V);
        if (dot(R, N) <= 0.0)
            R = reflect(-V, N);
        if (dot(R, N) > 0.0) {
            BounceHit specHit = TraceBounce(biasedPos, R);
            float3 Lo = 0;
            float3 samplePos = worldPos + R * 1000.0;
            float3 sampleN = -R;
            float sDist = 1000.0;
            if (specHit.valid) {
                Lo = EvaluateSecondaryLo(specHit, worldPos, sunDir, sunColor, viewDist, outdoor, pixel, rng);
                if (isHud)
                    Lo = min(Lo, sunColor * 0.3 + 0.05);
                samplePos = specHit.position;
                sampleN = specHit.normal;
                sDist = max(length(specHit.position - worldPos), 0.0);
            } else {
                Lo = min(SampleSkySpec(R, sampleRough), RESTIR_MAX_RADIANCE);
            }
            Lo = min(Lo, RESTIR_MAX_RADIANCE);
            specular = Lo * Fenv;
            specHitDist = sDist;
            accW = 1.0;
            bestLo = Lo;
            bestSamplePos = samplePos;
            bestSampleN = sampleN;
        }
    }
    if (accW > 1e-4) {
        float targetLum = Luminance(specular);
        if (targetLum > 0) {
            ReservoirUpdate(specReservoir, targetLum, bestSamplePos, bestSampleN, bestLo, RESTIR_INVALID_ID, rng);
            specReservoir.W = (specReservoir.M > 0) ? min(specReservoir.w_sum / (targetLum * (float)specReservoir.M), 4.0) : 0;
            specReservoir.age = 0;
        }
    }
    float dLum = max(Luminance(direct), 0.05);
    float specClamp = lerp(4.0, 12.0, saturate(roughness * 5.0));
    if (isHud)
        specClamp = min(specClamp, 1.5);
    if (Luminance(specular) > dLum * specClamp)
        specular *= (dLum * specClamp) / max(Luminance(specular), 1e-4);
    noisySpec = specular;
    if (specHitDist > 0)
        hitDist = specHitDist;

    reservoir.Lo = min(reservoir.Lo, RESTIR_MAX_RADIANCE);

    if (diRes.M > 0 && diRes.targetPdf > 0)
        diRes.W = ClampDIReservoirW(diRes.w_sum / max(diRes.targetPdf * (float)diRes.M, 1e-6));
    else
        diRes = EmptyDIReservoir();
    diRes.zone = IsInteriorSurfMark(surfMark) ? 1u : 0u;

    float4 sA, sB;
    PackReservoirAB(specReservoir, sA, sB);

    u_DirectLighting[pixel] = float4(min(direct, RESTIR_MAX_RADIANCE), 1.0);
    u_Reservoir[pixelIdx] = PackReservoirU4(reservoir, worldPos);
    u_NoisyDiffuse[pixel] = float4(min(noisyDiff, RESTIR_MAX_RADIANCE), 1.0);
    u_NoisySpecular[pixel] = float4(min(noisySpec, RESTIR_MAX_RADIANCE), hitDist);
    u_HitDistance[pixel] = hitDist;
    u_DIReservoir[pixel] = PackDIReservoir(diRes);
    u_SpecReservoirA[pixel] = sA;
    u_SpecReservoirB[pixel] = sB;
}
