#include "bindless_common.h"
#include "rt_common.h"
#include "rt_grass_alpha.h"
#include "rt_shade_hit.h"
#include "shared/surface_marks.h"
#include "restir_gi_common.h"

cbuffer ReSTIRWaterParams : register(b5) {
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float4 g_SunDir_Intensity;
    float4 g_SunColor_SkyWeight;
    float2 g_ScreenSize;
    float g_GIIntensity;
    uint g_IdentityStaticCount;
    uint g_TerrainBatchCount;
    uint g_SkinnedBatchStart;
    uint g_GrassBatchStart;
    uint g_DetailAtlasIndex;
    float4 g_HemiColor;
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
Texture2D<float> t_Depth : register(t14);
Texture2D<float4> t_Normal : register(t15);
Texture2D<float4> t_BaseColor : register(t16);
Texture2D<float4> t_WorldPos : register(t17);
ByteAddressBuffer g_MegaIB : register(t18);
Texture2D<float4> t_SceneColorIn : register(t19);
Texture2D<float4> t_ClassifyWorldPos : register(t20);
Texture2D<float4> t_UnderWorldPos : register(t21);

RWTexture2D<float4> u_SceneColor : register(u0);

bool IsSkinnedBatch(uint batchIdx)
{
    return g_SkinnedBatchStart != 0xFFFFFFFFu && batchIdx >= g_SkinnedBatchStart &&
           (g_GrassBatchStart == 0xFFFFFFFFu || batchIdx < g_GrassBatchStart);
}

bool IsGrassBatch(uint batchIdx)
{
    return g_GrassBatchStart != 0xFFFFFFFFu && batchIdx >= g_GrassBatchStart;
}

bool IsTerrainBatch(uint batchIdx)
{
    return batchIdx >= g_IdentityStaticCount &&
           batchIdx < g_IdentityStaticCount + g_TerrainBatchCount;
}

float3 SampleSkyW(float3 dir)
{
    float3 d = normalize(dir);
    float w = g_SunColor_SkyWeight.w;
    float3 s0 = g_Sky0.SampleLevel(smp_linear, d, 0).rgb;
    float3 s1 = g_Sky1.SampleLevel(smp_linear, d, 0).rgb;
    return max(lerp(s0, s1, w) * 1.15, float3(0.06, 0.07, 0.09));
}

float FresnelSchlickWater(float cosTheta)
{
    float F0 = 0.02;
    float m = 1.0 - saturate(cosTheta);
    float m2 = m * m;
    return F0 + (1.0 - F0) * m2 * m2 * m;
}

float3 DecodeNormal(float3 nEnc)
{
    float3 n = nEnc * 2.0 - 1.0;
    float lenSq = dot(n, n);
    if (lenSq < 1e-6)
        return float3(0.0, 1.0, 0.0);
    return n * rsqrt(lenSq);
}

float4 SampleTerrainTexture(uint index, float2 uv, float mip)
{
    if (index == INVALID_TEXTURE_INDEX)
        return float4(0.5, 0.5, 0.5, 1.0);
    return GetBindlessTexture(index).SampleLevel(smp_linear, uv, mip);
}

float3 SampleTerrainAlbedoWater(TerrainMaterialData mat, float2 uv, float hitDist)
{
    float lodDist = max(g_CameraPos.w, 10.0);
    float mip = saturate(hitDist / lodDist) * 0.45;
    float2 baseUV = uv;
    float4 baseSample = SampleTerrainTexture(mat.baseAlbedoIndex, baseUV, mip * 0.35);
    float4 mask = SampleTerrainTexture(mat.blendMaskIndex, baseUV, mip * 0.35);
    float maskSum = dot(mask, float4(1, 1, 1, 1));
    mask = maskSum > 0.001 ? mask / maskSum : float4(0.25, 0.25, 0.25, 0.25);
    float2 detailUV = uv * mat.detailScale;
    float detailMip = mip + 0.1;
    if (hitDist > lodDist * 0.65)
    {
        float m = max(max(mask.r, mask.g), max(mask.b, mask.a));
        float3 d = baseSample.rgb;
        if (mask.r >= m) d = SampleTerrainTexture(mat.detailR_Index, detailUV, detailMip).rgb;
        else if (mask.g >= m) d = SampleTerrainTexture(mat.detailG_Index, detailUV, detailMip).rgb;
        else if (mask.b >= m) d = SampleTerrainTexture(mat.detailB_Index, detailUV, detailMip).rgb;
        else d = SampleTerrainTexture(mat.detailA_Index, detailUV, detailMip).rgb;
        return baseSample.rgb * d * 2.0;
    }
    float3 detailR = SampleTerrainTexture(mat.detailR_Index, detailUV, detailMip).rgb;
    float3 detailG = SampleTerrainTexture(mat.detailG_Index, detailUV, detailMip).rgb;
    float3 detailB = SampleTerrainTexture(mat.detailB_Index, detailUV, detailMip).rgb;
    float3 detailA = SampleTerrainTexture(mat.detailA_Index, detailUV, detailMip).rgb;
    float3 blendedDetail = detailR * mask.r + detailG * mask.g + detailB * mask.b + detailA * mask.a;
    return baseSample.rgb * blendedDetail * 2.0;
}

float3 ShadeBakedFromMaterialLmap(MaterialData mat, float2 lmUV, float3 albedo, float hemi)
{
    if ((mat.flags & MAT_FLAG_HAS_LMAP) != 0 && mat.lmapIndex != INVALID_TEXTURE_INDEX)
    {
        float2 luv = (dot(lmUV, lmUV) > 1e-8) ? lmUV : float2(0.5, 0.5);
        float3 L = GetBindlessTexture(mat.lmapIndex).SampleLevel(smp_linear, luv, 0).rgb;
        return L * albedo;
    }
    return ShadeBakedFromHemi(hemi, albedo, g_HemiColor.rgb);
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = max(roughness * roughness, 0.001);
    float phi = 6.2831853 * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(max(1.0 - cosTheta * cosTheta, 0.0));
    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

float3 TraceWaterReflect(float3 origin, float3 dir, float3 skyRefl, uint maxSteps)
{
    float3 rayOrigin = origin;
    float remain = 8000.0;
    maxSteps = clamp(maxSteps, 1u, 8u);

    [loop]
    for (uint step = 0; step < maxSteps; ++step)
    {
        if (remain <= 0.05)
            break;

        RayDesc ray;
        ray.Origin = rayOrigin;
        ray.Direction = dir;
        ray.TMin = 0.02;
        ray.TMax = remain;

        RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
        q.TraceRayInline(g_SceneTLAS, RAY_FLAG_NONE, 0xFF, ray);
        while (q.Proceed())
        {
            if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
            {
                uint candBatch = q.CandidateInstanceID() + q.CandidateGeometryIndex();
                if (IsGrassBatch(candBatch))
                {
                    if (GrassTexelOpaque(g_GrassVB, g_GrassIB, g_BatchInfo, candBatch,
                            q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics(),
                            g_DetailAtlasIndex))
                        q.CommitNonOpaqueTriangleHit();
                }
                else
                {
                    RTBatchInfo candInfo = g_BatchInfo[candBatch];
                    MaterialData candMat = g_Materials[candInfo.materialID];
                    float2 candUV = GetHitUV(g_MegaVB, g_MegaIB, candInfo,
                        q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics());
                    float4 candDiff = SampleDiffuseLevel(candMat, candUV);
                    bool keep = true;
                    if (candMat.flags & MAT_FLAG_ALPHA_TEST)
                        keep = candDiff.a >= candMat.alphaRef;
                    else if (candMat.flags & MAT_FLAG_ALPHA_BLEND)
                        keep = candDiff.a >= 0.5;
                    if (keep)
                        q.CommitNonOpaqueTriangleHit();
                }
            }
        }

        if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
            break;

        float tHit = q.CommittedRayT();
        uint batchIdx = q.CommittedInstanceID() + q.CommittedGeometryIndex();
        RTBatchInfo info = g_BatchInfo[batchIdx];
        uint primIdx = q.CommittedPrimitiveIndex();
        float2 bary = q.CommittedTriangleBarycentrics();
        float3x4 objectToWorld = q.CommittedObjectToWorld3x4();

        float3 albedo = float3(0.5, 0.5, 0.5);
        float3 hitN = float3(0.0, 1.0, 0.0);
        float hemi = 0.5;
        float2 lmUV = 0;
        bool skipHit = false;
        bool hasLmap = false;
        float3 baked = 0;

        if (IsGrassBatch(batchIdx))
        {
            float2 hitUV = GetSkinnedHitUV(g_GrassVB, g_GrassIB, info, primIdx, bary);
            hitN = normalize(TransformNormalToWorld(
                GetSkinnedHitNormal(g_GrassVB, g_GrassIB, info, primIdx, bary), objectToWorld));
            if (g_DetailAtlasIndex > 0)
            {
                float4 texel = GetBindlessTexture(g_DetailAtlasIndex).SampleLevel(smp_linear, hitUV, 0);
                if (texel.a < 0.3)
                    skipHit = true;
                else
                    albedo = texel.rgb;
            }
            else
            {
                albedo = lerp(float3(0.08, 0.18, 0.03), float3(0.15, 0.35, 0.06), 1.0 - hitUV.y);
            }
            baked = g_HemiColor.rgb * albedo * 0.45;
        }
        else if (IsSkinnedBatch(batchIdx))
        {
            float2 hitUV = GetSkinnedHitUV(g_SkinnedVB, g_SkinnedIB, info, primIdx, bary);
            hitN = normalize(TransformNormalToWorld(
                GetSkinnedHitNormal(g_SkinnedVB, g_SkinnedIB, info, primIdx, bary), objectToWorld));
            MaterialData mat = g_Materials[info.materialID];
            float4 diffuse = SampleDiffuseLevel(mat, hitUV);
            if ((mat.flags & MAT_FLAG_ALPHA_TEST) && diffuse.a < mat.alphaRef)
                skipHit = true;
            else if ((mat.flags & MAT_FLAG_ALPHA_BLEND) && diffuse.a < 0.5)
                skipHit = true;
            else if (mat.flags & MAT_FLAG_WATER)
                skipHit = true;
            else
                albedo = diffuse.rgb;
            hemi = 0.55;
            baked = ShadeBakedFromHemi(hemi, albedo, g_HemiColor.rgb);
            if ((mat.flags & MAT_FLAG_EMISSIVE) && mat.emissiveIntensity > 0.0)
                baked += albedo * mat.emissiveIntensity;
        }
        else if (IsTerrainBatch(batchIdx))
        {
            float2 hitUV = GetHitUV(g_MegaVB, g_MegaIB, info, primIdx, bary);
            lmUV = GetHitLightmapUV(g_MegaVB, g_MegaIB, info, primIdx, bary);
            hemi = GetHitHemi(g_MegaVB, g_MegaIB, info, primIdx, bary);
            hitN = normalize(TransformNormalToWorld(
                GetHitNormal(g_MegaVB, g_MegaIB, info, primIdx, bary), objectToWorld));
            TerrainMaterialData tmat = g_TerrainMaterials[info.materialID];
            albedo = SampleTerrainAlbedoWater(tmat, hitUV, tHit);
            baked = ShadeBakedFromTerrainLmap(tmat, lmUV, albedo, g_HemiColor.rgb, hemi);
            hasLmap = (tmat.flags & MAT_FLAG_HAS_LMAP) != 0 && tmat.lmapIndex != INVALID_TEXTURE_INDEX;
        }
        else
        {
            float2 hitUV = GetHitUV(g_MegaVB, g_MegaIB, info, primIdx, bary);
            lmUV = GetHitLightmapUV(g_MegaVB, g_MegaIB, info, primIdx, bary);
            hemi = GetHitHemi(g_MegaVB, g_MegaIB, info, primIdx, bary);
            hitN = normalize(TransformNormalToWorld(
                GetHitNormal(g_MegaVB, g_MegaIB, info, primIdx, bary), objectToWorld));
            MaterialData mat = g_Materials[info.materialID];
            float4 diffuse = SampleDiffuseLevel(mat, hitUV);
            if (mat.flags & MAT_FLAG_WATER)
                skipHit = true;
            else if ((mat.flags & MAT_FLAG_ALPHA_TEST) && diffuse.a < mat.alphaRef)
                skipHit = true;
            else if ((mat.flags & MAT_FLAG_ALPHA_BLEND) && diffuse.a < 0.5)
                skipHit = true;
            else
                albedo = diffuse.rgb;
            baked = ShadeBakedFromMaterialLmap(mat, lmUV, albedo, hemi);
            hasLmap = (mat.flags & MAT_FLAG_HAS_LMAP) != 0 && mat.lmapIndex != INVALID_TEXTURE_INDEX;
            if ((mat.flags & MAT_FLAG_EMISSIVE) && mat.emissiveIntensity > 0.0)
                baked += albedo * mat.emissiveIntensity;
        }

        if (skipHit)
        {
            float adv = tHit + 0.003;
            rayOrigin = rayOrigin + dir * adv;
            remain -= adv;
            continue;
        }

        if (dot(hitN, dir) > 0.0)
            hitN = -hitN;

        float3 sunDir = normalize(-g_SunDir_Intensity.xyz);
        float sunI = saturate(g_SunDir_Intensity.w);
        float3 sunCol = max(g_SunColor_SkyWeight.rgb, float3(0.25, 0.25, 0.25)) * sunI;
        float ndl = saturate(dot(hitN, sunDir));
        float3 hitCol = baked;
        if (!hasLmap)
            hitCol += albedo * sunCol * ndl * 0.85;
        else
            hitCol += albedo * sunCol * ndl * 0.12;
        hitCol += SampleSkyW(hitN) * albedo * 0.08;
        return lerp(hitCol, skyRefl, 0.05);
    }

    return skyRefl;
}

float3 TraceWaterReflectGlossy(float3 origin, float3 N, float3 V, uint maxSteps, uint spp, inout uint rng)
{
    float3 R = reflect(-V, N);
    float rough = 0.045;
    spp = clamp(spp, 1u, 2u);
    float3 acc = 0;
    float wSum = 0;
    [loop]
    for (uint i = 0; i < spp; ++i)
    {
        float2 xi = float2(rand_float(rng), rand_float(rng));
        float3 H = ImportanceSampleGGX(xi, N, rough);
        float3 L = normalize(reflect(-V, H));
        if (dot(L, N) <= 0.0)
            L = R;
        float3 skyRefl = SampleSkyW(L);
        float3 c = TraceWaterReflect(origin, L, skyRefl, maxSteps);
        float w = max(dot(L, N), 0.05);
        acc += c * w;
        wSum += w;
    }
    float3 glossy = acc / max(wSum, 1e-3);
    float3 mirrorSky = SampleSkyW(R);
    return lerp(glossy, mirrorSky, 0.02);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float4 under4 = t_SceneColorIn.Load(int3(pixel, 0));
    float guideMark = t_WorldPos.Load(int3(pixel, 0)).w;
    float4 classifyData = t_ClassifyWorldPos.Load(int3(pixel, 0));

    if (IsHudSurfMark(guideMark) || !IsWaterSurfMark(classifyData.w))
    {
        u_SceneColor[pixel] = under4;
        return;
    }

    float depth = t_Depth.Load(int3(pixel, 0));
    if (depth <= 0.0)
    {
        u_SceneColor[pixel] = under4;
        return;
    }

    float3 waterPos = classifyData.xyz;
    float waterDist = length(waterPos - g_CameraPos.xyz);
    float lodDist = max(g_CameraPos.w, 10.0);

    float4 underWP = t_UnderWorldPos.Load(int3(pixel, 0));
    float waterDepth = 8.0;
    bool hasUnder = underWP.w > 0.5 && !IsWaterSurfMark(underWP.w);
    if (hasUnder)
    {
        float underDist = length(underWP.xyz - g_CameraPos.xyz);
        waterDepth = max(0.0, underDist - waterDist);
    }

    float edgeFade = hasUnder ? smoothstep(0.0, 1.6, waterDepth) : 0.75;
    if (edgeFade < 0.03)
    {
        u_SceneColor[pixel] = under4;
        return;
    }

    float3 N = DecodeNormal(t_Normal.Load(int3(pixel, 0)).xyz);
    N = normalize(lerp(float3(0.0, 1.0, 0.0), float3(N.x, abs(N.y) + 0.05, N.z), 0.2));

    float3 V = normalize(g_CameraPos.xyz - waterPos);
    float3 R = normalize(reflect(-V, N));
    float F = FresnelSchlickWater(abs(dot(N, V)));

    float3 skyRefl = SampleSkyW(R);
    float3 reflected = skyRefl;
    float lod = saturate(waterDist / lodDist);
    if (lod < 0.95)
    {
        uint steps = (lod < 0.4) ? 3u : ((lod < 0.7) ? 2u : 1u);
        uint spp = (lod < 0.5) ? 2u : 1u;
        uint rng = pcg_hash(pixel.x + pixel.y * 7919u + asuint(waterPos.x) * 97u);
        reflected = TraceWaterReflectGlossy(waterPos + N * 0.08, N, V, steps, spp, rng);
    }

    float3 sunDir = normalize(-g_SunDir_Intensity.xyz);
    float3 sunColor = g_SunColor_SkyWeight.rgb * saturate(g_SunDir_Intensity.w);
    reflected += sunColor * pow(saturate(dot(R, sunDir)), 384.0) * 0.7;

    float reflA = saturate(0.5 + 0.45 * F) * edgeFade;
    float3 body = t_BaseColor.Load(int3(pixel, 0)).rgb;
    float3 under = lerp(body * float3(0.45, 0.55, 0.5), under4.rgb, 0.2);
    float3 color = lerp(under, reflected, reflA);
    u_SceneColor[pixel] = float4(color, under4.a);
}
