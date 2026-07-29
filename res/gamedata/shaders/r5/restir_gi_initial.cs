#include "bindless_common.h"
#include "rt_common.h"
#include "rt_visibility.h"
#include "rt_shade_hit.h"
#include "rt_irradiance_cache.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "shared/nrd_helpers.h"
#include "shared/foliage_sss.h"
#include "shared/skin_sss.h"
#include "shared/surface_marks.h"
#include "restir_gi_common.h"
#include "restir_di_common.h"
#include "restir_di_eval.h"

cbuffer ReSTIRGIParams : register(b5) {
    float4x4 g_InvViewProj;
    float4x4 g_PrevViewProj;
    float4x4 g_WorldToView;
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
    uint g_LocalLightSamples;
    uint g_DICandidates;
    uint g_Bounces;
    float4 g_DISampleParams;
    float4 g_ClusterParams;
    float4 g_ClusterScales;
    float4 g_HemiColor;
    uint g_CacheSize;
    float g_CacheCellSize;
    uint g_CacheMaxAge;
    uint g_ParticleBatchStart;
    float g_LodDist;
    float g_SunAngular;
    uint g_SunSoftSamples;
    float g_CameraMotion;
};

RaytracingAccelerationStructure g_SceneTLAS : register(t1);
StructuredBuffer<RTBatchInfo> g_BatchInfo : register(t2);
ByteAddressBuffer g_MegaVB : register(t3);
TextureCube<float4> g_Sky0 : register(t5);
TextureCube<float4> g_Sky1 : register(t6);
ByteAddressBuffer g_SkinnedVB : register(t7);
ByteAddressBuffer g_SkinnedIB : register(t11);
ByteAddressBuffer g_GrassVB : register(t12);
ByteAddressBuffer g_GrassIB : register(t13);
ByteAddressBuffer g_ParticleVB : register(t19);
ByteAddressBuffer g_ParticleIB : register(t4);
Texture2D<float> t_Depth : register(t14);
Texture2D<float4> t_Normal : register(t15);
Texture2D<float4> t_BaseColor : register(t16);
Texture2D<float4> t_WorldPos : register(t17);
ByteAddressBuffer g_MegaIB : register(t18);

StructuredBuffer<GPULightDataDI> g_LightData : register(t20);
StructuredBuffer<uint> g_DILightIndices : register(t21);
StructuredBuffer<float> g_DILightCDF : register(t22);

RWTexture2D<float4> u_DirectLighting : register(u0);
RWTexture2D<float4> u_ReservoirA : register(u1);
RWTexture2D<float4> u_ReservoirB : register(u2);
RWTexture2D<float4> u_NoisySpecular : register(u3);
RWTexture2D<float4> u_DIReservoir : register(u4);
RWTexture2D<float4> u_SpecReservoirA : register(u5);
RWTexture2D<float4> u_SpecReservoirB : register(u6);
RWTexture2D<float2> u_ReservoirC : register(u7);
RWStructuredBuffer<IrradianceCacheEntry> u_IrradianceCache : register(u8);

bool IsSkinnedBatch(uint batchIdx)
{
    return g_SkinnedBatchStart != 0xFFFFFFFFu && batchIdx >= g_SkinnedBatchStart &&
           (g_GrassBatchStart == 0xFFFFFFFFu || batchIdx < g_GrassBatchStart) &&
           (g_ParticleBatchStart == 0xFFFFFFFFu || batchIdx < g_ParticleBatchStart);
}

bool IsGrassBatch(uint batchIdx)
{
    return g_GrassBatchStart != 0xFFFFFFFFu && batchIdx >= g_GrassBatchStart &&
           (g_ParticleBatchStart == 0xFFFFFFFFu || batchIdx < g_ParticleBatchStart);
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
    return lerp(s0, s1, w);
}

float3 SampleSkyDiffuse(float3 dir)
{
    float w = g_SunColor_SkyWeight.w;
    float3 s0 = g_Sky0.SampleLevel(smp_linear, dir, 4.0).rgb;
    float3 s1 = g_Sky1.SampleLevel(smp_linear, dir, 4.0).rgb;
    return lerp(s0, s1, w);
}

float3 SampleSkySpec(float3 dir, float roughness)
{
    float mip = saturate(roughness) * 5.0;
    float w = g_SunColor_SkyWeight.w;
    float3 s0 = g_Sky0.SampleLevel(smp_linear, dir, mip).rgb;
    float3 s1 = g_Sky1.SampleLevel(smp_linear, dir, mip).rgb;
    return lerp(s0, s1, w);
}

float TraceShadow(float3 origin, float3 dir, float tMax, bool nearSkinnedOccludes, float skinnedSelfMax)
{
    return TraceVisibilityAtten(
        g_SceneTLAS, g_BatchInfo, g_MegaVB, g_MegaIB, g_GrassVB, g_GrassIB, g_ParticleVB, g_ParticleIB,
        origin, dir, tMax,
        g_IdentityStaticCount, g_TerrainBatchCount, g_SkinnedBatchStart, g_GrassBatchStart,
        g_ParticleBatchStart, g_DetailAtlasIndex, nearSkinnedOccludes, skinnedSelfMax);
}

float TraceSoftShadowSun(float3 origin, float3 sunDir, float viewDist, bool nearSkinnedOccludes, float skinnedSelfMax, inout uint rng)
{
    float lod = saturate(viewDist / max(g_LodDist, 1.0));
    uint maxS = max(g_SunSoftSamples, 1u);
    uint samples = 1u;
    if (lod < 0.55)
        samples = maxS;
    else if (lod < 0.85)
        samples = max(1u, maxS / 2u);

    if (samples <= 1u)
        return TraceShadow(origin, sunDir, 10000.0, nearSkinnedOccludes, skinnedSelfMax);

    float3 L = normalize(sunDir);
    float3 up = abs(L.y) < 0.99 ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 T = normalize(cross(up, L));
    float3 B = cross(L, T);
    float radius = max(g_SunAngular, 0.001);
    float vis = 0.0;
    for (uint i = 0; i < samples; ++i)
    {
        float2 u = float2(rand_float(rng), rand_float(rng));
        float r = sqrt(u.x) * radius;
        float a = u.y * 6.2831853;
        float3 dir = normalize(L + T * (cos(a) * r) + B * (sin(a) * r));
        vis += TraceShadow(origin, dir, 10000.0, nearSkinnedOccludes, skinnedSelfMax);
    }
    return vis / (float)samples;
}

float PointLightAttenuationRT(float distSq, float invRangeSq)
{
    float factor = saturate(1.0 - distSq * abs(invRangeSq));
    return factor * factor;
}

float SpotLightAttenuationRT(float3 toLight, float3 spotDir, float scale, float offset)
{
    float cosAngle = dot(normalize(-toLight), spotDir);
    return saturate(cosAngle * scale + offset);
}

float LinearizeDepthRT(float ndcDepth, float zNear, float zFar)
{
    return zNear * zFar / (zFar - ndcDepth * (zFar - zNear));
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

float3 EvaluateLocalLightsRT(
    float3 worldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness,
    float2 pixelPos, float linearDepth, uint maxSamples, inout uint rng)
{
    uint diCount = (uint)g_DISampleParams.x;
    if (diCount == 0 || maxSamples == 0 || g_DISampleParams.y <= 1e-8)
        return 0;

    uint sampleCount = min(diCount, maxSamples);
    float3 total = 0;
    float3 biasedPos = worldPos + N * 0.02;

    bool exhaustive = (sampleCount == diCount);
    for (uint i = 0; i < sampleCount; i++) {
        float lightPdf = 1.0;
        uint lightIdx;
        if (exhaustive) {
            lightIdx = g_DILightIndices[i];
        } else {
            lightIdx = SampleDILightIS(rand_float(rng), lightPdf);
            if (lightPdf <= 1e-8)
                continue;
        }
        GPULightDataDI light = g_LightData[lightIdx];
        float3 L, lightColor;
        float dist;
        float atten = EvalLocalLightAttenuationDI(light, worldPos, L, dist, lightColor);
        if (atten <= 0.001)
            continue;

        float shadow = 1.0;
        if (asuint(light.spotParamsAndType.w) != 0xFFFFFFFFu) {
            float endSkip = max(0.55, LightEmitterRadiusDI(light) * 3.0 + 0.25);
            float shadowDist = max(dist - endSkip, dist * 0.88);
            if (shadowDist > 0.02)
                shadow = TraceShadow(biasedPos, L, shadowDist, true, 0.0);
        }
        if (shadow <= 0.001)
            continue;

        float3 lit = PBRDirectLighting(albedo, N, V, L, lightColor * atten * shadow, metallic, roughness, 1);
        total += exhaustive ? lit : (lit / (lightPdf * (float)sampleCount));
    }

    return total;
}

DIReservoir SampleDIReservoir(
    float3 worldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness,
    float2 pixelPos, float linearDepth, uint candidates, bool hudSurface, inout uint rng)
{
    DIReservoir r = EmptyDIReservoir();
    uint diCount = (uint)g_DISampleParams.x;
    if (diCount == 0 || candidates == 0 || g_DISampleParams.y <= 1e-8)
        return r;

    uint sampleCount = min(diCount, candidates);
    for (uint i = 0; i < sampleCount; i++) {
        float lightPdf = 0;
        uint lightIdx;
        if (sampleCount == diCount) {
            lightIdx = g_DILightIndices[i];
            float prev = (i == 0) ? 0.0 : g_DILightCDF[i - 1];
            lightPdf = max(g_DILightCDF[i] - prev, 1e-8) / g_DISampleParams.y;
        } else {
            lightIdx = SampleDILightIS(rand_float(rng), lightPdf);
        }
        if (hudSurface && !IsHudLightDI(g_LightData[lightIdx]))
            continue;
        float pdf = EvalLocalLightTargetPdfDI(
            g_LightData[lightIdx], worldPos, N, V, albedo, metallic, roughness);
        float w = (lightPdf > 1e-8) ? (pdf / lightPdf) : 0.0;
        DIReservoirUpdate(r, w, lightIdx, pdf, rng);
    }

    if (r.M > 0 && r.targetPdf > 0 && r.lightIndex != RESTIR_INVALID_ID) {
        r.W = ClampDIReservoirW(r.w_sum / max(r.targetPdf * (float)r.M, 1e-6));
        if (r.W <= 0)
            r = EmptyDIReservoir();
    } else
        r = EmptyDIReservoir();
    return r;
}

float3 SampleTerrainAlbedo(TerrainMaterialData mat, float2 uv, float hitDist);

struct BounceHit {
    float3 position;
    float3 normal;
    float3 geoNormal;
    float3 albedo;
    float3 baked;
    float3 emissive;
    float metallic;
    float roughness;
    float hemi;
    float t;
    bool valid;
};

BounceHit TraceBounce(float3 origin, float3 direction, inout uint rng, bool skipNearSkinned)
{
    BounceHit result;
    result.valid = false;
    result.baked = 0;
    result.emissive = 0;
    result.hemi = 0;

    float3 rayOrigin = origin;
    for (uint skip = 0; skip < 4; skip++) {
        RayDesc ray;
        ray.Origin = rayOrigin;
        ray.Direction = direction;
        ray.TMin = 0.001;
        ray.TMax = 10000.0;

        RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
        q.TraceRayInline(g_SceneTLAS, RAY_FLAG_NONE, 0xFF, ray);
        while (q.Proceed()) {
            if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE) {
                uint candBatch = q.CandidateInstanceID() + q.CandidateGeometryIndex();
                if (IsGrassBatch(candBatch) && g_DetailAtlasIndex > 0) {
                    RTBatchInfo candInfo = g_BatchInfo[candBatch];
                    float2 candUV = GetSkinnedHitUV(g_GrassVB, g_GrassIB, candInfo,
                        q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics());
                    float4 texel = GetBindlessTexture(g_DetailAtlasIndex).SampleLevel(smp_linear, candUV, 0);
                    if (texel.a >= 0.3)
                        q.CommitNonOpaqueTriangleHit();
                }
            }
        }

        if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
            return result;

        uint batchIdx = q.CommittedInstanceID() + q.CommittedGeometryIndex();
        RTBatchInfo info = g_BatchInfo[batchIdx];
        uint primIdx = q.CommittedPrimitiveIndex();
        float2 bary = q.CommittedTriangleBarycentrics();
        float3x4 objectToWorld = q.CommittedObjectToWorld3x4();

        float3 hitN, geoN;
        float2 hitUV;
        float2 lmUV = 0;
        float hemi = 0.5;
        if (IsGrassBatch(batchIdx)) {
            hitUV = GetSkinnedHitUV(g_GrassVB, g_GrassIB, info, primIdx, bary);
            hitN = GetSkinnedHitNormal(g_GrassVB, g_GrassIB, info, primIdx, bary);
            geoN = GetSkinnedHitGeoNormal(g_GrassVB, g_GrassIB, info, primIdx);
        } else if (IsSkinnedBatch(batchIdx)) {
            if (skipNearSkinned && q.CommittedRayT() < 0.2) {
                rayOrigin = rayOrigin + direction * (q.CommittedRayT() + 0.002);
                continue;
            }
            hitUV = GetSkinnedHitUV(g_SkinnedVB, g_SkinnedIB, info, primIdx, bary);
            hitN = GetSkinnedHitNormal(g_SkinnedVB, g_SkinnedIB, info, primIdx, bary);
            geoN = GetSkinnedHitGeoNormal(g_SkinnedVB, g_SkinnedIB, info, primIdx);
        } else {
            hitUV = GetHitUV(g_MegaVB, g_MegaIB, info, primIdx, bary);
            lmUV = GetHitLightmapUV(g_MegaVB, g_MegaIB, info, primIdx, bary);
            hemi = GetHitHemi(g_MegaVB, g_MegaIB, info, primIdx, bary);
            hitN = TransformNormalToWorld(GetHitNormal(g_MegaVB, g_MegaIB, info, primIdx, bary), objectToWorld);
            geoN = TransformNormalToWorld(GetHitGeometricNormal(g_MegaVB, g_MegaIB, info, primIdx), objectToWorld);
        }

        if (dot(geoN, direction) > 0) geoN = -geoN;
        if (dot(hitN, geoN) < 0) hitN = -hitN;

        float3 albedo = float3(0.5, 0.5, 0.5);
        float metallic = 0;
        float roughness = 1.0;
        float3 baked = 0;
        float3 emissive = 0;

        if (IsGrassBatch(batchIdx)) {
            if (g_DetailAtlasIndex > 0) {
                float4 texel = GetBindlessTexture(g_DetailAtlasIndex).SampleLevel(smp_linear, hitUV, 0);
                albedo = texel.rgb;
                if (texel.a < 0.3) {
                    rayOrigin = rayOrigin + direction * (q.CommittedRayT() + 0.002);
                    continue;
                }
            } else {
                albedo = lerp(float3(0.08, 0.18, 0.03), float3(0.15, 0.35, 0.06), 1.0 - hitUV.y);
            }
            baked = g_HemiColor.rgb * albedo * 0.35;
        } else if (IsTerrainBatch(batchIdx)) {
            TerrainMaterialData tmat = g_TerrainMaterials[info.materialID];
            albedo = SampleTerrainAlbedo(tmat, hitUV, q.CommittedRayT());
            baked = ShadeBakedFromTerrainLmap(tmat, lmUV, albedo, g_HemiColor.rgb, hemi);
        } else {
            MaterialData mat = g_Materials[info.materialID];
            if (skipNearSkinned && (mat.flags & MAT_FLAG_WATER)) {
                rayOrigin = rayOrigin + direction * (q.CommittedRayT() + 0.002);
                continue;
            }
            float4 diffuse = SampleDiffuseLevel(mat, hitUV);
            albedo = diffuse.rgb;

            if ((mat.flags & MAT_FLAG_ALPHA_TEST) && diffuse.a < mat.alphaRef) {
                rayOrigin = rayOrigin + direction * (q.CommittedRayT() + 0.002);
                continue;
            }

            if (mat.flags & MAT_FLAG_HAS_PBR) {
                float3 pbr = SamplePBR(mat, hitUV);
                metallic = pbr.r;
                roughness = pbr.g;
            }
            baked = ShadeBakedFromHemi(hemi, albedo, g_HemiColor.rgb);
            if ((mat.flags & MAT_FLAG_EMISSIVE) && mat.emissiveIntensity > 0.0)
                emissive = albedo * mat.emissiveIntensity;
        }

        result.position = rayOrigin + direction * q.CommittedRayT();
        result.normal = hitN;
        result.geoNormal = geoN;
        result.albedo = albedo;
        result.baked = baked;
        result.emissive = emissive;
        result.metallic = metallic;
        result.roughness = roughness;
        result.hemi = hemi;
        result.t = q.CommittedRayT();
        result.valid = true;
        return result;
    }
    return result;
}

float4 SampleTerrainTexture(uint index, float2 uv)
{
    if (index == INVALID_TEXTURE_INDEX)
        return float4(0.5, 0.5, 0.5, 1.0);
    return GetBindlessTexture(index).SampleLevel(smp_linear, uv, 0);
}

float3 SampleTerrainAlbedo(TerrainMaterialData mat, float2 uv, float hitDist)
{
    float2 baseUV = uv;
    float4 baseSample = SampleTerrainTexture(mat.baseAlbedoIndex, baseUV);
    float4 mask = SampleTerrainTexture(mat.blendMaskIndex, baseUV);
    float maskSum = dot(mask, float4(1, 1, 1, 1));
    mask = maskSum > 0.001 ? mask / maskSum : float4(0.25, 0.25, 0.25, 0.25);
    if (hitDist > g_LodDist * 0.5)
    {
        float m = max(max(mask.r, mask.g), max(mask.b, mask.a));
        float3 d = baseSample.rgb;
        if (mask.r >= m) d = SampleTerrainTexture(mat.detailR_Index, uv * mat.detailScale).rgb;
        else if (mask.g >= m) d = SampleTerrainTexture(mat.detailG_Index, uv * mat.detailScale).rgb;
        else if (mask.b >= m) d = SampleTerrainTexture(mat.detailB_Index, uv * mat.detailScale).rgb;
        else d = SampleTerrainTexture(mat.detailA_Index, uv * mat.detailScale).rgb;
        return baseSample.rgb * d * 2.0;
    }
    float2 detailUV = uv * mat.detailScale;
    float3 detailR = SampleTerrainTexture(mat.detailR_Index, detailUV).rgb;
    float3 detailG = SampleTerrainTexture(mat.detailG_Index, detailUV).rgb;
    float3 detailB = SampleTerrainTexture(mat.detailB_Index, detailUV).rgb;
    float3 detailA = SampleTerrainTexture(mat.detailA_Index, detailUV).rgb;
    float3 blendedDetail = detailR * mask.r + detailG * mask.g + detailB * mask.b + detailA * mask.a;
    return baseSample.rgb * blendedDetail * 2.0;
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = max(roughness * roughness, 0.001);
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(max(1.0 - cosTheta * cosTheta, 0.0));

    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

float3 EvaluateSecondaryLo(
    BounceHit h,
    float3 primaryPos,
    float3 sunDir,
    float3 sunColor,
    float2 pixelPos,
    float primaryViewDist,
    inout uint rngState)
{
    float3 hitBiased = h.position + h.geoNormal * 0.005;
    float3 hitV = normalize(primaryPos - h.position);
    float hitShadow = TraceShadow(hitBiased, sunDir, 10000.0, true, 0.0);
    float3 Lo = ShadeHitDirect(h.albedo, h.normal, hitV, h.metallic, h.roughness, sunDir, sunColor, hitShadow, h.baked);
    Lo += h.emissive;
    float hitLinDepth = abs(mul(g_WorldToView, float4(h.position, 1.0)).z);
    float farLod = saturate(primaryViewDist / max(g_LodDist, 1.0));
    uint localS = (farLod > 0.7) ? 0u : ((rand_float(rngState) < 0.5) ? 1u : 0u);
    if (localS > 0)
        Lo += EvaluateLocalLightsRT(h.position, h.normal, hitV, h.albedo, h.metallic, h.roughness,
            pixelPos, hitLinDepth, localS, rngState);
    if (g_Bounces >= 2u && farLod < 0.65 && rand_float(rngState) < 0.75) {
        float2 u2 = float2(rand_float(rngState), rand_float(rngState));
        float3 dir2 = cosine_weighted_hemisphere(u2, h.normal);
        BounceHit h2 = TraceBounce(hitBiased, dir2, rngState, false);
        if (h2.valid) {
            float3 hit2Biased = h2.position + h2.geoNormal * 0.005;
            float3 hit2V = normalize(h.position - h2.position);
            float sh2 = TraceShadow(hit2Biased, sunDir, 10000.0, true, 0.0);
            float3 Lo2 = ShadeHitDirect(h2.albedo, h2.normal, hit2V, h2.metallic, h2.roughness, sunDir, sunColor, sh2, h2.baked);
            Lo2 += h2.emissive;
            float3 cached2 = QueryIrradianceCacheRW(u_IrradianceCache, h2.position, g_CacheCellSize, g_CacheSize, g_FrameIndex, g_CacheMaxAge);
            if (any(cached2 > 0))
                Lo2 = max(Lo2, cached2 * h2.albedo * 0.65);
            Lo += Lo2 * h2.albedo * (1.0 / 0.75) * 0.55;
            UpdateIrradianceCache(u_IrradianceCache, h2.position, Lo2, g_CacheCellSize, g_CacheSize, g_FrameIndex);
        } else {
            Lo += SampleSkyDiffuse(dir2) * (1.0 / 0.75) * 0.45;
        }
    }
    float3 cached = QueryIrradianceCacheRW(u_IrradianceCache, h.position, g_CacheCellSize, g_CacheSize, g_FrameIndex, g_CacheMaxAge);
    if (any(cached > 0))
    {
        float cacheW = saturate(0.55 + 0.35 * (1.0 - saturate(Luminance(Lo) * 4.0)));
        Lo = lerp(Lo, max(Lo, cached * h.albedo), cacheW);
    }
    UpdateIrradianceCache(u_IrradianceCache, h.position, Lo, g_CacheCellSize, g_CacheSize, g_FrameIndex);
    return min(Lo, RESTIR_MAX_RADIANCE);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    float4 worldPosData = t_WorldPos.Load(int3(pixel, 0));
    if (depth <= 0.0) {
        u_DirectLighting[pixel] = 0;
        u_ReservoirA[pixel] = 0;
        u_ReservoirB[pixel] = 0;
        u_NoisySpecular[pixel] = 0;
        u_DIReservoir[pixel] = PackDIReservoir(EmptyDIReservoir());
        u_SpecReservoirA[pixel] = 0;
        u_SpecReservoirB[pixel] = 0;
        u_ReservoirC[pixel] = 0;
        return;
    }

    float3 worldPos = worldPosData.xyz;
    if (all(worldPos == 0.0)) {
        float2 uv = (float2(pixel) + 0.5) / g_ScreenSize;
        float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
        float4 worldH = mul(g_InvViewProj, float4(ndc, depth, 1.0));
        worldPos = worldH.xyz / max(worldH.w, 1e-5);
        worldPosData.xyz = worldPos;
    }

    const bool isHud = IsHudSurfMark(worldPosData.w);
    const bool isChar = IsCharSurfMark(worldPosData.w);
    const float skinnedSelfMax = isChar ? 0.65 : 0.0;
    float4 normalData = t_Normal.Load(int3(pixel, 0));
    float4 baseColorData = t_BaseColor.Load(int3(pixel, 0));

    float3 N = normalize(normalData.xyz);
    float roughness = max(normalData.w, MIN_ROUGHNESS);
    float3 albedo = baseColorData.rgb;
    float metallic = UnpackMetallicFromBaseA(baseColorData.a);
    float3 V = normalize(g_CameraPos.xyz - worldPos);
    float3 sunDir = normalize(-g_SunDir_Intensity.xyz);
    float sunIntensity = g_SunDir_Intensity.w;
    float3 sunColor = g_SunColor_SkyWeight.xyz * sunIntensity;

    float3 biasedPos = worldPos + N * (isHud ? 0.03 : (isChar ? 0.035 : 0.01));
    float linearDepth = abs(mul(g_WorldToView, float4(worldPos, 1.0)).z);
    float viewDist = length(worldPos - g_CameraPos.xyz);
    float2 pixelPos = float2(pixel) + 0.5;

    uint rng = pcg_hash(pixel.x + pixel.y * 1973u + g_FrameIndex * 26699u);
    float shadow = TraceSoftShadowSun(biasedPos, sunDir, viewDist, isHud, skinnedSelfMax, rng);
    float3 direct = 0;
    if (shadow > 0.001)
        direct = PBRDirectLighting(albedo, N, V, sunDir, sunColor * shadow, metallic, roughness, 1);

    float sssMask = UnpackSSSMaskFromBaseA(baseColorData.a);
    if (!isHud && sssMask > 0.01) {
        float3 sssTint = SkinSSSTint();
        float sssThickness = saturate(0.55 + sssMask * 0.35);
        direct += EvaluateFoliageSSS(
            albedo, N, V, sunDir, sunColor, max(shadow, 0.35),
            sssTint, sssThickness, sssMask);
    }
    DIReservoir diRes = SampleDIReservoir(
        worldPos, N, V, albedo, metallic, roughness,
        pixelPos, linearDepth, max(g_DICandidates, 1u), isHud, rng);
    u_DIReservoir[pixel] = PackDIReservoir(diRes);

    GIReservoir reservoir = EmptyReservoir();
    GIReservoir specReservoir = EmptyReservoir();
    u_DirectLighting[pixel] = float4(direct, 1.0);

    float3 F0primary = CalculateF0(albedo, metallic);
    float3 kDprimary = (1.0 - metallic);
    bool diffLobeActive = any(kDprimary * albedo > 1e-4);

    if (diffLobeActive) {
        float2 u = float2(rand_float(rng), rand_float(rng));
        float3 bounceDir = cosine_weighted_hemisphere(u, N);
        float cosPDF = max(dot(bounceDir, N), 0) / PI;
        BounceHit hit = TraceBounce(biasedPos, bounceDir, rng, isHud || isChar);

        float3 LoSurf = 0;
        float3 LoSky = min(SampleSkyDiffuse(bounceDir), RESTIR_MAX_RADIANCE);
        float3 samplePos = worldPos + bounceDir * g_ClusterScales.y;
        float3 sampleN = -bounceDir;
        bool usedSurf = false;

        if (hit.valid && cosPDF > 1e-6) {
            LoSurf = EvaluateSecondaryLo(hit, worldPos, sunDir, sunColor, pixelPos, viewDist, rng);
            if (isHud)
                LoSurf = min(LoSurf, sunColor * 0.35 + 0.08);
            samplePos = hit.position;
            sampleN = hit.normal;
            usedSurf = true;
            if (Luminance(LoSurf) < 0.015)
            {
                float3 pCache = QueryIrradianceCacheRW(u_IrradianceCache, worldPos, g_CacheCellSize, g_CacheSize, g_FrameIndex, g_CacheMaxAge);
                if (any(pCache > 0))
                    LoSurf = max(LoSurf, pCache * albedo * 0.5);
            }
        } else if (isHud) {
            LoSky = min(LoSky, sunColor * 0.25 + 0.06);
        }

        float3 Lo = usedSurf ? LoSurf : LoSky;
        float3 wi = normalize(samplePos - worldPos);
        float cosTheta = max(dot(N, wi), 0);
        float3 kD = (1.0 - F_Schlick(cosTheta, F0primary)) * (1.0 - metallic);
        float3 brdfCos = kD * albedo / PI * cosTheta;
        float targetLum = Luminance(Lo * brdfCos);
        if (targetLum > 0 && cosPDF > 1e-6) {
            float mis = 1.0;
            if (usedSurf && Luminance(LoSky) > 1e-4) {
                float pSurf = cosPDF;
                float pSky = 1.0 / (2.0 * PI);
                mis = pSurf / max(pSurf + pSky, 1e-6);
            }
            float w = (targetLum / cosPDF) * mis;
            ReservoirUpdate(reservoir, w, samplePos, sampleN, Lo, rng);
            float pHat = max(targetLum, 1e-6);
            reservoir.W = (reservoir.M > 0) ? reservoir.w_sum / (pHat * (float)reservoir.M) : 0;
            reservoir.age = 0;
        }
    }

    float3 Fenv = NRD_EnvironmentTerm_Rtg(F0primary, abs(dot(N, V)), roughness);
    float sampleRough = max(roughness, 0.06);
    uint specSamples = 1u;

    float3 specular = 0;
    float specHitDist = 0;
    float3 bestSamplePos = worldPos;
    float3 bestSampleN = N;
    float3 bestLo = 0;
    float accW = 0;

    [loop] for (uint si = 0; si < specSamples; si++) {
        float2 uSpec = float2(rand_float(rng), rand_float(rng));
        float3 H = ImportanceSampleGGX(uSpec, N, sampleRough);
        float3 R = normalize(2.0 * max(dot(V, H), 0.0) * H - V);
        if (dot(R, N) <= 0.0)
            R = reflect(-V, N);
        if (dot(R, N) <= 0.0)
            continue;

        BounceHit specHit = TraceBounce(biasedPos, R, rng, isHud || isChar);
        float3 Lo = 0;
        float3 samplePos = worldPos + R * g_ClusterScales.y;
        float3 sampleN = -R;
        float hitDist = g_ClusterScales.y;
        if (specHit.valid) {
            Lo = EvaluateSecondaryLo(specHit, worldPos, sunDir, sunColor, pixelPos, viewDist, rng);
            if (isHud)
                Lo = min(Lo, sunColor * 0.3 + 0.05);
            samplePos = specHit.position;
            sampleN = specHit.normal;
            hitDist = length(specHit.position - worldPos);
            if (hitDist < 1e-3)
                hitDist = 0;
        } else {
            Lo = min(SampleSkySpec(R, sampleRough), RESTIR_MAX_RADIANCE);
            if (isHud)
                Lo = min(Lo, sunColor * 0.2 + 0.04);
        }
        Lo = min(Lo, RESTIR_MAX_RADIANCE);
        float w = 1.0 / (float)specSamples;
        specular += Lo * Fenv * w;
        specHitDist += hitDist * w;
        accW += w;
        if (Luminance(Lo) >= Luminance(bestLo)) {
            bestLo = Lo;
            bestSamplePos = samplePos;
            bestSampleN = sampleN;
        }
    }

    if (accW > 1e-4) {
        float targetLum = Luminance(specular);
        if (targetLum > 0) {
            ReservoirUpdate(specReservoir, targetLum, bestSamplePos, bestSampleN, bestLo, rng);
            specReservoir.W = (specReservoir.M > 0) ? specReservoir.w_sum / (targetLum * (float)specReservoir.M) : 0;
            specReservoir.age = 0;
        }
    }

    float dLum = max(Luminance(direct), 0.05);
    float specClamp = lerp(4.0, 12.0, saturate(roughness * 5.0));
    if (isHud)
        specClamp = min(specClamp, 1.5);
    if (Luminance(specular) > dLum * specClamp)
        specular *= (dLum * specClamp) / max(Luminance(specular), 1e-4);
    u_NoisySpecular[pixel] = float4(specular, specHitDist);

    float4 resA, resB;
    PackReservoir(reservoir, resA, resB);
    u_ReservoirA[pixel] = resA;
    u_ReservoirB[pixel] = resB;
    u_ReservoirC[pixel] = PackReservoirC(reservoir);

    float4 sA, sB;
    PackReservoir(specReservoir, sA, sB);
    u_SpecReservoirA[pixel] = sA;
    u_SpecReservoirB[pixel] = sB;
}
