#include "bindless_common.h"
#include "rt_common.h"
#include "shared/pbr_brdf.h"
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
    uint3 g_Pad;
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
Texture2D<float4> t_WorldPos : register(t17);

RWTexture2D<float4> u_DirectLighting : register(u0);
RWTexture2D<float4> u_ReservoirA : register(u1);
RWTexture2D<float4> u_ReservoirB : register(u2);

static const uint MAX_SHADOW_SKIPS = 8;

bool IsSkinnedBatch(uint batchIdx)
{
    return g_SkinnedBatchStart > 0 && batchIdx >= g_SkinnedBatchStart &&
           !(g_GrassBatchStart > 0 && batchIdx >= g_GrassBatchStart);
}

bool IsGrassBatch(uint batchIdx)
{
    return g_GrassBatchStart > 0 && batchIdx >= g_GrassBatchStart;
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

float TraceShadow(float3 origin, float3 sunDir)
{
    float atten = 1.0;
    float3 shadowOrigin = origin;

    for (uint si = 0; si < MAX_SHADOW_SKIPS; si++) {
        RayDesc ray;
        ray.Origin = shadowOrigin;
        ray.Direction = sunDir;
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
            break;

        uint sBatchIdx = q.CommittedInstanceID() + q.CommittedGeometryIndex();
        RTBatchInfo sInfo = g_BatchInfo[sBatchIdx];

        if (IsGrassBatch(sBatchIdx)) {
            if (g_DetailAtlasIndex > 0) { atten = 0; break; }
            atten *= 0.5;
            shadowOrigin = shadowOrigin + sunDir * (q.CommittedRayT() + 0.002);
            continue;
        }

        MaterialData sMat = g_Materials[sInfo.materialID];

        if (sMat.flags & MAT_FLAG_WATER) {
            atten *= 0.85;
            shadowOrigin = shadowOrigin + sunDir * (q.CommittedRayT() + 0.002);
            continue;
        }

        if (IsTerrainBatch(sBatchIdx) || IsSkinnedBatch(sBatchIdx)) { atten = 0; break; }

        float2 sUV;
        if (IsSkinnedBatch(sBatchIdx))
            sUV = GetSkinnedHitUV(g_SkinnedVB, g_SkinnedIB, sInfo, q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics());
        else
            sUV = GetHitUV(g_MegaVB, g_MegaIB, sInfo, q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics());

        float4 sDiffuse = SampleDiffuseLevel(sMat, sUV);

        if ((sMat.flags & MAT_FLAG_ALPHA_TEST) && sDiffuse.a < sMat.alphaRef) {
            shadowOrigin = shadowOrigin + sunDir * (q.CommittedRayT() + 0.002);
            continue;
        }
        if ((sMat.flags & MAT_FLAG_ALPHA_BLEND) && sDiffuse.a < 0.5) {
            atten *= (1.0 - sDiffuse.a);
            shadowOrigin = shadowOrigin + sunDir * (q.CommittedRayT() + 0.002);
            continue;
        }

        atten = 0;
        break;
    }
    return atten;
}

struct BounceHit {
    float3 position;
    float3 normal;
    float3 geoNormal;
    float3 albedo;
    float metallic;
    float roughness;
    float t;
    bool valid;
};

BounceHit TraceBounce(float3 origin, float3 direction, inout uint rng)
{
    BounceHit result;
    result.valid = false;

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
        }

        if (dot(geoN, direction) > 0) geoN = -geoN;
        if (dot(hitN, geoN) < 0) hitN = -hitN;

        float3 albedo = float3(0.5, 0.5, 0.5);
        float metallic = 0;
        float roughness = 1.0;

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
        } else if (IsTerrainBatch(batchIdx)) {
            TerrainMaterialData tmat = g_TerrainMaterials[info.materialID];
            albedo = SampleTerrainAlbedo(tmat, hitUV);
        } else {
            MaterialData mat = g_Materials[info.materialID];
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
        }

        result.position = rayOrigin + direction * q.CommittedRayT();
        result.normal = hitN;
        result.geoNormal = geoN;
        result.albedo = albedo;
        result.metallic = metallic;
        result.roughness = roughness;
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

float3 SampleTerrainAlbedo(TerrainMaterialData mat, float2 uv)
{
    float2 baseUV = uv;
    float2 detailUV = uv * mat.detailScale;
    float4 baseSample = SampleTerrainTexture(mat.baseAlbedoIndex, baseUV);
    float4 mask = SampleTerrainTexture(mat.blendMaskIndex, baseUV);
    float maskSum = dot(mask, float4(1, 1, 1, 1));
    mask = maskSum > 0.001 ? mask / maskSum : float4(0.25, 0.25, 0.25, 0.25);
    float3 detailR = SampleTerrainTexture(mat.detailR_Index, detailUV).rgb;
    float3 detailG = SampleTerrainTexture(mat.detailG_Index, detailUV).rgb;
    float3 detailB = SampleTerrainTexture(mat.detailB_Index, detailUV).rgb;
    float3 detailA = SampleTerrainTexture(mat.detailA_Index, detailUV).rgb;
    float3 blendedDetail = detailR * mask.r + detailG * mask.g + detailB * mask.b + detailA * mask.a;
    return baseSample.rgb * blendedDetail * 2.0;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    if (depth >= 1.0) {
        u_DirectLighting[pixel] = 0;
        u_ReservoirA[pixel] = 0;
        u_ReservoirB[pixel] = 0;
        return;
    }

    float3 worldPos = t_WorldPos.Load(int3(pixel, 0)).xyz;
    float4 normalData = t_Normal.Load(int3(pixel, 0));
    float4 baseColorData = t_BaseColor.Load(int3(pixel, 0));

    float3 N = normalize(normalData.xyz);
    float roughness = normalData.w;
    float3 albedo = baseColorData.rgb;
    float metallic = baseColorData.a;
    float3 V = normalize(g_CameraPos.xyz - worldPos);
    float3 sunDir = normalize(-g_SunDir_Intensity.xyz);
    float sunIntensity = g_SunDir_Intensity.w;
    float3 sunColor = g_SunColor_SkyWeight.xyz * sunIntensity;

    float3 biasedPos = worldPos + N * 0.01;

    // === DIRECT LIGHTING (shadow ray to sun) ===
    float shadow = TraceShadow(biasedPos, sunDir);
    float3 direct = 0;
    if (shadow > 0.001) {
        float NdotL = max(0.0, dot(N, sunDir));
        direct = albedo * NdotL * sunColor * shadow;
    }

    // === INDIRECT LIGHTING (1 cosine bounce + NEE) ===
    uint rng = pcg_hash(pixel.x + pixel.y * 1973u + g_FrameIndex * 26699u);

    float2 u = float2(rand_float(rng), rand_float(rng));
    float3 bounceDir = cosine_weighted_hemisphere(u, N);
    float cosPDF = max(dot(bounceDir, N), 0) / PI;

    BounceHit hit = TraceBounce(biasedPos, bounceDir, rng);

    GIReservoir reservoir = EmptyReservoir();

    if (!hit.valid) {
        direct += albedo * SampleSky(bounceDir);
    }

    u_DirectLighting[pixel] = float4(direct, 1.0);

    if (hit.valid) {
        float3 hitBiased = hit.position + hit.geoNormal * 0.005;
        float3 hitV = normalize(worldPos - hit.position);

        float hitShadow = TraceShadow(hitBiased, sunDir);
        float3 secondaryDirect = 0;
        if (hitShadow > 0.001) {
            float hitNdotL = max(0.0, dot(hit.normal, sunDir));
            secondaryDirect = hit.albedo * hitNdotL * sunColor * hitShadow;
        }
        float3 Lo = secondaryDirect;
        Lo = min(Lo, RESTIR_MAX_RADIANCE);

        float3 wi = normalize(hit.position - worldPos);
        float cosTheta = max(dot(N, wi), 0);
        float3 F0 = CalculateF0(albedo, metallic);
        float3 kD = (1.0 - F_Schlick(cosTheta, F0)) * (1.0 - metallic);
        float3 brdfCos = kD * albedo / PI * cosTheta;
        float3 target = Lo * brdfCos;
        float targetLum = Luminance(target);

        if (targetLum > 0 && cosPDF > 1e-6) {
            float w = targetLum / cosPDF;
            ReservoirUpdate(reservoir, w, hit.position, hit.normal, Lo, rng);
            reservoir.W = 1.0 / cosPDF;
        }
    }

    float4 resA, resB;
    PackReservoir(reservoir, resA, resB);
    u_ReservoirA[pixel] = resA;
    u_ReservoirB[pixel] = resB;
}
