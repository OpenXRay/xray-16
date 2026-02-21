#include "bindless_common.h"
#include "rt_common.h"

cbuffer PathTracerParams : register(b5) {
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float4 g_SunDir_Intensity;
    float4 g_SunColor_SkyWeight;
    float g_ScreenWidth;
    float g_ScreenHeight;
    uint g_SampleIndex;
    uint g_MaxBounces;
    uint g_IdentityStaticCount;
    uint g_TerrainBatchCount;
    uint g_TransparentBatchCount;
    uint g_SkinnedBatchStart;
    uint g_GrassBatchStart;
    uint g_DetailAtlasIndex;
    uint2 g_Pad;
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

RWTexture2D<float4> g_Accumulation : register(u0);
RWTexture2D<float4> g_Output : register(u1);

static const uint MAX_ALPHA_SKIPS = 8;

bool IsSkinnedBatch(uint batchIdx)
{
    return g_SkinnedBatchStart > 0 && batchIdx >= g_SkinnedBatchStart &&
           !(g_GrassBatchStart > 0 && batchIdx >= g_GrassBatchStart);
}

bool IsGrassBatch(uint batchIdx)
{
    return g_GrassBatchStart > 0 && batchIdx >= g_GrassBatchStart;
}

float4 SampleTerrainTexture(uint index, float2 uv)
{
    if (index == INVALID_TEXTURE_INDEX)
        return float4(0.5, 0.5, 0.5, 1.0);
    return GetBindlessTexture(index).SampleLevel(g_LinearSampler, uv, 0);
}

float3 SampleTerrainAlbedo(TerrainMaterialData mat, float2 uv)
{
    float2 baseUV = uv;
    float2 detailUV = uv * mat.detailScale;

    float4 baseSample = SampleTerrainTexture(mat.baseAlbedoIndex, baseUV);

    float4 mask = SampleTerrainTexture(mat.blendMaskIndex, baseUV);
    float maskSum = dot(mask, float4(1, 1, 1, 1));
    if (maskSum > 0.001)
        mask /= maskSum;
    else
        mask = float4(0.25, 0.25, 0.25, 0.25);

    float3 detailR = SampleTerrainTexture(mat.detailR_Index, detailUV).rgb;
    float3 detailG = SampleTerrainTexture(mat.detailG_Index, detailUV).rgb;
    float3 detailB = SampleTerrainTexture(mat.detailB_Index, detailUV).rgb;
    float3 detailA = SampleTerrainTexture(mat.detailA_Index, detailUV).rgb;

    float3 blendedDetail = detailR * mask.r + detailG * mask.g + detailB * mask.b + detailA * mask.a;
    return baseSample.rgb * blendedDetail * 2.0;
}

bool IsTerrainBatch(uint batchIdx)
{
    return batchIdx >= g_IdentityStaticCount &&
           batchIdx < g_IdentityStaticCount + g_TerrainBatchCount;
}

struct HitMaterial {
    float3 albedo;
    float alpha;
    float alphaRef;
    float metallic;
    float roughness;
    uint flags;
};

HitMaterial GetHitMaterial(RTBatchInfo info, float2 hitUV, uint batchIdx)
{
    HitMaterial result;
    result.alpha = 1.0;
    result.alphaRef = 0.0;
    result.metallic = 0.0;
    result.roughness = 1.0;
    result.flags = 0;

    if (IsGrassBatch(batchIdx)) {
        if (g_DetailAtlasIndex > 0) {
            float4 texel = GetBindlessTexture(g_DetailAtlasIndex).SampleLevel(g_LinearSampler, hitUV, 0);
            result.albedo = texel.rgb;
            result.alpha = texel.a;
        } else {
            float height_t = 1.0 - hitUV.y;
            result.albedo = lerp(float3(0.08, 0.18, 0.03), float3(0.15, 0.35, 0.06), height_t);
        }
        return result;
    }

    if (IsTerrainBatch(batchIdx)) {
        TerrainMaterialData tmat = g_TerrainMaterials[info.materialID];
        result.albedo = SampleTerrainAlbedo(tmat, hitUV);
        result.flags = tmat.flags;
        return result;
    }

    MaterialData mat = g_Materials[info.materialID];
    float4 diffuse = SampleDiffuseLevel(mat, hitUV);
    result.albedo = diffuse.rgb;
    result.alpha = diffuse.a;
    result.alphaRef = mat.alphaRef;
    result.flags = mat.flags;

    if (mat.flags & MAT_FLAG_HAS_PBR) {
        float3 pbr = SamplePBR(mat, hitUV);
        result.metallic = pbr.r;
        result.roughness = pbr.g;
    }

    return result;
}

float3 SampleSky(float3 dir)
{
    float w = g_SunColor_SkyWeight.w;
    float3 s0 = g_Sky0.SampleLevel(g_LinearSampler, dir, 0).rgb;
    float3 s1 = g_Sky1.SampleLevel(g_LinearSampler, dir, 0).rgb;
    return lerp(s0, s1, w);
}

float3 GenerateCameraRay(uint2 pixel, inout uint rng, out float3 origin)
{
    float2 jitter = float2(rand_float(rng), rand_float(rng));
    float2 uv = (float2(pixel) + jitter) / float2(g_ScreenWidth, g_ScreenHeight);
    float4 clip = float4(uv * 2.0 - 1.0, 0.0, 1.0);
    clip.y = -clip.y;

    float4 nearWorld = mul(g_InvViewProj, clip);
    nearWorld.xyz /= nearWorld.w;

    float4 farClip = float4(clip.xy, 1.0, 1.0);
    float4 farWorld = mul(g_InvViewProj, farClip);
    farWorld.xyz /= farWorld.w;

    origin = g_CameraPos.xyz;
    return normalize(farWorld.xyz - nearWorld.xyz);
}

struct HitResult {
    float2 uv;
    float3 normal;
    float3 geoNormal;
};

HitResult GetHitAttributes(uint batchIdx, RTBatchInfo info, uint primIdx, float2 bary, float3x4 objectToWorld)
{
    HitResult h;
    if (IsGrassBatch(batchIdx)) {
        h.uv = GetSkinnedHitUV(g_GrassVB, g_GrassIB, info, primIdx, bary);
        h.normal = GetSkinnedHitNormal(g_GrassVB, g_GrassIB, info, primIdx, bary);
        h.geoNormal = GetSkinnedHitGeoNormal(g_GrassVB, g_GrassIB, info, primIdx);
        return h;
    }
    if (IsSkinnedBatch(batchIdx)) {
        h.uv = GetSkinnedHitUV(g_SkinnedVB, g_SkinnedIB, info, primIdx, bary);
        h.normal = GetSkinnedHitNormal(g_SkinnedVB, g_SkinnedIB, info, primIdx, bary);
        h.geoNormal = GetSkinnedHitGeoNormal(g_SkinnedVB, g_SkinnedIB, info, primIdx);
    } else {
        h.uv = GetHitUV(g_MegaVB, g_MegaIB, info, primIdx, bary);
        float3 localN = GetHitNormal(g_MegaVB, g_MegaIB, info, primIdx, bary);
        h.normal = TransformNormalToWorld(localN, objectToWorld);
        float3 localGeoN = GetHitGeometricNormal(g_MegaVB, g_MegaIB, info, primIdx);
        h.geoNormal = TransformNormalToWorld(localGeoN, objectToWorld);
    }
    return h;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenWidth || pixel.y >= (uint)g_ScreenHeight)
        return;

    uint rng = pcg_hash(pixel.x + pixel.y * 1973u + g_SampleIndex * 26699u);

    float3 origin;
    float3 direction = GenerateCameraRay(pixel, rng, origin);

    float3 radiance = 0;
    float3 throughput = 1;

    float3 sunDir = normalize(-g_SunDir_Intensity.xyz);
    float sunIntensity = g_SunDir_Intensity.w;
    float3 sunColor = g_SunColor_SkyWeight.xyz * sunIntensity;

    for (uint bounce = 0; bounce < g_MaxBounces; bounce++) {
        RayDesc ray;
        ray.Origin = origin;
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
                    float4 texel = GetBindlessTexture(g_DetailAtlasIndex).SampleLevel(g_LinearSampler, candUV, 0);
                    if (texel.a >= 0.3)
                        q.CommitNonOpaqueTriangleHit();
                }
            }
        }

        if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT) {
            radiance += throughput * SampleSky(direction);
            break;
        }

        uint batchIdx = q.CommittedInstanceID() + q.CommittedGeometryIndex();
        uint primIdx = q.CommittedPrimitiveIndex();
        float2 bary = q.CommittedTriangleBarycentrics();
        float hitT = q.CommittedRayT();
        float3x4 objectToWorld = q.CommittedObjectToWorld3x4();

        RTBatchInfo info = g_BatchInfo[batchIdx];
        HitResult hit = GetHitAttributes(batchIdx, info, primIdx, bary, objectToWorld);
        float3 hitN = hit.normal;
        float3 geoN = hit.geoNormal;

        if (dot(geoN, direction) > 0)
            geoN = -geoN;
        if (dot(hitN, geoN) < 0)
            hitN = -hitN;

        HitMaterial hitMat = GetHitMaterial(info, hit.uv, batchIdx);

        if ((hitMat.flags & MAT_FLAG_ALPHA_TEST) && hitMat.alpha < hitMat.alphaRef) {
            origin = origin + direction * hitT + direction * 0.002;
            bounce--;
            continue;
        }

        if ((hitMat.flags & MAT_FLAG_ALPHA_BLEND) && hitMat.alpha < 0.5) {
            throughput *= (1.0 - hitMat.alpha);
            origin = origin + direction * hitT + direction * 0.002;
            bounce--;
            continue;
        }

        float3 hitPos = origin + direction * hitT;
        float3 biasedPos = hitPos + geoN * 0.005;

        {
            float shadowAtten = 1.0;
            float3 shadowOrigin = biasedPos;
            for (uint si = 0; si < MAX_ALPHA_SKIPS; si++) {
                RayDesc shadowRay;
                shadowRay.Origin = shadowOrigin;
                shadowRay.Direction = sunDir;
                shadowRay.TMin = 0.001;
                shadowRay.TMax = 10000.0;

                RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> shadowQ;
                shadowQ.TraceRayInline(g_SceneTLAS, RAY_FLAG_NONE, 0xFF, shadowRay);
                while (shadowQ.Proceed()) {
                    if (shadowQ.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE) {
                        uint candBatch = shadowQ.CandidateInstanceID() + shadowQ.CandidateGeometryIndex();
                        if (IsGrassBatch(candBatch) && g_DetailAtlasIndex > 0) {
                            RTBatchInfo candInfo = g_BatchInfo[candBatch];
                            float2 candUV = GetSkinnedHitUV(g_GrassVB, g_GrassIB, candInfo,
                                shadowQ.CandidatePrimitiveIndex(), shadowQ.CandidateTriangleBarycentrics());
                            float4 texel = GetBindlessTexture(g_DetailAtlasIndex).SampleLevel(g_LinearSampler, candUV, 0);
                            if (texel.a >= 0.3)
                                shadowQ.CommitNonOpaqueTriangleHit();
                        }
                    }
                }

                if (shadowQ.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
                    break;

                uint sBatchIdx = shadowQ.CommittedInstanceID() + shadowQ.CommittedGeometryIndex();
                RTBatchInfo sInfo = g_BatchInfo[sBatchIdx];

                if (IsGrassBatch(sBatchIdx)) {
                    if (g_DetailAtlasIndex > 0) {
                        shadowAtten = 0.0;
                        break;
                    }
                    shadowAtten *= 0.5;
                    shadowOrigin = shadowOrigin + sunDir * (shadowQ.CommittedRayT() + 0.002);
                    continue;
                }

                float2 sUV;
                if (IsSkinnedBatch(sBatchIdx))
                    sUV = GetSkinnedHitUV(g_SkinnedVB, g_SkinnedIB, sInfo, shadowQ.CommittedPrimitiveIndex(), shadowQ.CommittedTriangleBarycentrics());
                else
                    sUV = GetHitUV(g_MegaVB, g_MegaIB, sInfo, shadowQ.CommittedPrimitiveIndex(), shadowQ.CommittedTriangleBarycentrics());

                if (IsTerrainBatch(sBatchIdx)) {
                    shadowAtten = 0.0;
                    break;
                }

                if (IsSkinnedBatch(sBatchIdx)) {
                    shadowAtten = 0.0;
                    break;
                }

                MaterialData sMat = g_Materials[sInfo.materialID];
                float4 sDiffuse = SampleDiffuseLevel(sMat, sUV);

                if ((sMat.flags & MAT_FLAG_ALPHA_TEST) && sDiffuse.a < sMat.alphaRef) {
                    shadowOrigin = shadowOrigin + sunDir * (shadowQ.CommittedRayT() + 0.002);
                    continue;
                }

                if ((sMat.flags & MAT_FLAG_ALPHA_BLEND) && sDiffuse.a < 0.5) {
                    shadowAtten *= (1.0 - sDiffuse.a);
                    shadowOrigin = shadowOrigin + sunDir * (shadowQ.CommittedRayT() + 0.002);
                    continue;
                }

                shadowAtten = 0.0;
                break;
            }

            if (shadowAtten > 0.001) {
                float NdotL = max(0.0, dot(hitN, sunDir));
                radiance += throughput * hitMat.albedo * NdotL * sunColor * shadowAtten;
            }
        }

        float2 u = float2(rand_float(rng), rand_float(rng));
        float3 bounceDir = cosine_weighted_hemisphere(u, hitN);
        throughput *= hitMat.albedo;

        if (bounce >= 3) {
            float p = max(throughput.r, max(throughput.g, throughput.b));
            if (p < 0.01) break;
            if (rand_float(rng) > p) break;
            throughput /= p;
        }

        origin = biasedPos;
        direction = bounceDir;
    }

    float4 newSample = float4(radiance, 1.0);

    if (g_SampleIndex == 0) {
        g_Accumulation[pixel] = newSample;
    } else {
        float4 prev = g_Accumulation[pixel];
        g_Accumulation[pixel] = prev + (newSample - prev) / float(g_SampleIndex + 1);
    }

    g_Output[pixel] = float4(g_Accumulation[pixel].rgb, 1.0);
}
