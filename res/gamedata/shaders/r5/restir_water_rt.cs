#include "bindless_common.h"
#include "shared/terrain_blend.h"
#include "rt_common.h"
#include "rt_grass_alpha.h"
#include "rt_material_alpha.h"
#include "rt_shade_hit.h"
#include "shared/surface_marks.h"
#include "restir_gi_common.h"

cbuffer ReSTIRWaterParams : register(b5) {
    float4x4 g_InvViewProj;
    float4x4 g_ViewProj;
    float4 g_CameraPos;
    float4 g_SunDir_Intensity;
    float4 g_SunColor_SkyWeight;
    float4 g_SkyColor;
    float2 g_ScreenSize;
    float g_GIIntensity;
    uint g_IdentityStaticCount;
    uint g_TerrainBatchCount;
    uint g_SkinnedBatchStart;
    uint g_GrassBatchStart;
    uint g_DetailAtlasIndex;
    uint g_HudSkinnedStart;
    float g_LodDist;
    uint g_Pad1;
    uint g_Pad2;
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
Texture2D<float4> t_UnderColor : register(t22);

RWTexture2D<float4> u_SceneColor : register(u0);

bool IsHudSkinnedBatch(uint batchIdx)
{
    return g_HudSkinnedStart != 0xFFFFFFFFu && batchIdx >= g_HudSkinnedStart &&
           (g_GrassBatchStart == 0xFFFFFFFFu || batchIdx < g_GrassBatchStart);
}

float RayAttenBorder(float2 pos, float value)
{
    float borderDist = min(1.0 - max(pos.x, pos.y), min(pos.x, pos.y));
    return saturate(borderDist > value ? 1.0 : borderDist / value);
}

bool WorldToUvVP(float3 worldPos, out float2 uv, out float w)
{
    uv = 0.0;
    float4 clip = mul(g_ViewProj, float4(worldPos, 1.0));
    w = clip.w;
    if (w <= 1e-4)
        return false;
    float2 ndc = clip.xy / w;
    uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
    return true;
}

float3 ReconstructWorldDepth(float2 uv, float rawDepth)
{
    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    float4 worldH = mul(g_InvViewProj, float4(ndc, rawDepth, 1.0));
    return worldH.xyz / max(worldH.w, 1e-5);
}

bool IsSkinnedBatch(uint batchIdx)
{
    return g_SkinnedBatchStart != 0xFFFFFFFFu && batchIdx >= g_SkinnedBatchStart &&
           (g_GrassBatchStart == 0xFFFFFFFFu || batchIdx < g_GrassBatchStart);
}

bool IsGrassBatch(uint batchIdx)
{
    return g_GrassBatchStart != 0xFFFFFFFFu && batchIdx == g_GrassBatchStart;
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
    return lerp(s0, s1, w) * g_SkyColor.rgb * 0.95;
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
    float4 mask = TerrainNormalizeMask(SampleTerrainTexture(mat.blendMaskIndex, baseUV, mip * 0.35));
    float2 detailUV = uv * mat.detailScale;
    float detailMip = mip + 0.1;
    float4 detailR = SampleTerrainTexture(mat.detailR_Index, detailUV, detailMip);
    float4 detailG = SampleTerrainTexture(mat.detailG_Index, detailUV, detailMip);
    float4 detailB = SampleTerrainTexture(mat.detailB_Index, detailUV, detailMip);
    float4 detailA = SampleTerrainTexture(mat.detailA_Index, detailUV, detailMip);
    float3 blendedDetail = TerrainBlendRGB(detailR.rgb, detailG.rgb, detailB.rgb, detailA.rgb, mask);
    return baseSample.rgb * blendedDetail * 2.0;
}

float3 ShadeBakedFromMaterialLmap(MaterialData mat, float2 lmUV, float3 albedo, float hemi)
{
    if ((mat.flags & MAT_FLAG_HAS_LMAP) != 0 && mat.lmapIndex != INVALID_TEXTURE_INDEX)
    {
        float2 luv = (dot(lmUV, lmUV) > 1e-8) ? lmUV : float2(0.5, 0.5);
        float4 lmh = GetBindlessTexture(mat.lmapIndex).SampleLevel(smp_rtlinear, luv, 0);
        return ShadeBakedFromHemi(max(hemi, lmh.a), albedo, g_HemiColor.rgb);
    }
    return ShadeBakedFromHemi(hemi, albedo, g_HemiColor.rgb);
}

bool HudOrActorAt(int2 ip)
{
    float w = t_WorldPos.Load(int3(ip, 0)).w;
    return IsHudSurfMark(w) || IsCharSurfMark(w);
}

bool HudNear(int2 ip)
{
    [unroll]
    for (int y = -3; y <= 3; ++y)
    {
        [unroll]
        for (int x = -3; x <= 3; ++x)
        {
            int2 p = clamp(ip + int2(x, y), int2(0, 0), int2(g_ScreenSize) - 1);
            if (HudOrActorAt(p))
                return true;
        }
    }
    return false;
}

float3 SampleWaterSSRColor(int2 ip)
{
    float3 under = t_UnderColor.Load(int3(ip, 0)).rgb;
    if (dot(under, float3(0.2126, 0.7152, 0.0722)) > 1e-6)
        return under;
    if (HudOrActorAt(ip) || HudNear(ip))
        return 0.0;
    return t_SceneColorIn.Load(int3(ip, 0)).rgb;
}

float4 ScreenReflectWater(float3 origin, float3 dir, float3 waterPos, float3 skyRefl, int steps)
{
    float2 startUV = 0.0;
    float startW = 1.0;
    if (!WorldToUvVP(origin, startUV, startW))
        return float4(skyRefl, 0.0);

    steps = clamp(steps, 1, 48);
    const float maxDist = 420.0;
    const float tMin = 1.5;
    const float thickness = 1.35;
    float3 cam = g_CameraPos.xyz;
    float waterY = waterPos.y;

    float2 hitUV = startUV;
    float hitConf = 0.0;
    bool hit = false;
    float tPrev = tMin;
    float2 prevUV = startUV;
    {
        float pw = 1.0;
        WorldToUvVP(origin + dir * tMin, prevUV, pw);
    }

    [loop]
    for (int i = 1; i <= steps; ++i)
    {
        float u = float(i) / float(steps);
        float t = max(tMin, maxDist * u * u);
        float3 marchPos = origin + dir * t;
        float2 uv = 0.0;
        float w = 1.0;
        if (!WorldToUvVP(marchPos, uv, w))
            break;

        float border = RayAttenBorder(uv, 0.10);
        if (border <= 1e-3)
            break;

        int2 ip = int2(uv * g_ScreenSize);
        ip = clamp(ip, int2(0, 0), int2(g_ScreenSize) - 1);
        float rawDepth = t_Depth.Load(int3(ip, 0));
        if (rawDepth <= 1e-7)
        {
            prevUV = uv;
            tPrev = t;
            continue;
        }

        float4 wpMark = t_WorldPos.Load(int3(ip, 0));
        if (HudNear(ip) || IsWaterSurfMark(wpMark.w))
        {
            prevUV = uv;
            tPrev = t;
            continue;
        }

        float4 underWP = t_UnderWorldPos.Load(int3(ip, 0));
        float3 scenePos = (dot(underWP.xyz, underWP.xyz) > 1e-2)
            ? underWP.xyz
            : ReconstructWorldDepth(uv, rawDepth);
        if (length(scenePos - cam) < 1.5)
        {
            prevUV = uv;
            tPrev = t;
            continue;
        }
        if (scenePos.y < waterY - 0.35)
        {
            prevUV = uv;
            tPrev = t;
            continue;
        }

        float3 toScene = scenePos - origin;
        float along = dot(toScene, dir);
        if (along < tMin)
        {
            prevUV = uv;
            tPrev = t;
            continue;
        }

        float perp = length(scenePos - (origin + dir * along));
        float marchCam = length(marchPos - cam);
        float sceneCam = length(scenePos - cam);
        float delta = sceneCam - marchCam;
        float thick = thickness * (1.0 + along * 0.01);
        if (delta > thick || delta < -thick * 2.5 || perp > thick * 1.5)
        {
            if (delta < -thick * 2.5)
                break;
            prevUV = uv;
            tPrev = t;
            continue;
        }

        if (length(uv - startUV) < 0.006)
        {
            prevUV = uv;
            tPrev = t;
            continue;
        }

        hitUV = uv;
        hitConf = saturate(1.0 - abs(delta) / max(thick * 2.0, 1e-3));
        hitConf *= saturate(1.0 - perp / max(thick * 2.5, 1e-3));
        hitConf *= border;
        hitConf *= saturate((along - tMin) * 0.35);
        hit = hitConf > 0.06;
        break;
    }

    if (!hit)
        return float4(skyRefl, 0.0);

    int2 hitIp = clamp(int2(hitUV * g_ScreenSize), int2(0, 0), int2(g_ScreenSize) - 1);
    if (HudNear(hitIp))
        return float4(skyRefl, 0.0);
    float3 img = SampleWaterSSRColor(hitIp);
    if (dot(img, float3(0.2126, 0.7152, 0.0722)) < 1e-5)
        return float4(skyRefl, 0.0);
    return float4(img, saturate(hitConf));
}

bool TraceWaterReflect(float3 origin, float3 dir, float3 skyRefl, uint maxSteps, float3 waterPos, out float3 outCol)
{
    outCol = skyRefl;
    float3 rayOrigin = origin;
    float remain = 20000.0;
    maxSteps = clamp(maxSteps, 1u, 16u);
    float3 cam = g_CameraPos.xyz;
    uint shadeSteps = 0;

    [loop]
    for (uint iter = 0; iter < 48u && shadeSteps < maxSteps; ++iter)
    {
        if (remain <= 0.05)
            break;

        RayDesc ray;
        ray.Origin = rayOrigin;
        ray.Direction = dir;
        ray.TMin = 0.08;
        ray.TMax = remain;

        RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
        q.TraceRayInline(g_SceneTLAS, RAY_FLAG_NONE, RT_MASK_SHADOW, ray);
        while (q.Proceed())
        {
            if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
            {
                uint candBatch = q.CandidateInstanceID() + q.CandidateGeometryIndex();
                if (IsHudSkinnedBatch(candBatch) || IsSkinnedBatch(candBatch))
                    continue;
                if (IsGrassBatch(candBatch)) {
                    if (GrassTexelOpaque(g_GrassVB, g_GrassIB, g_BatchInfo, candBatch,
                            q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics(),
                            g_DetailAtlasIndex))
                        q.CommitNonOpaqueTriangleHit();
                    continue;
                }
                else
                {
                    RTBatchInfo candInfo = g_BatchInfo[candBatch];
                    MaterialData candMat = g_Materials[candInfo.materialID];
                    if ((candMat.flags & MAT_FLAG_WATER) != 0)
                        continue;
                    float2 candUV = GetHitUV(g_MegaVB, g_MegaIB, candInfo,
                        q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics());
                    float4 candDiff = SampleDiffuseLevel(candMat, candUV);
                    bool keep = true;
                    if ((candMat.flags & MAT_FLAG_ALPHA_TEST) != 0)
                        keep = candDiff.a >= candMat.alphaRef;
                    else if ((candMat.flags & MAT_FLAG_ALPHA_BLEND) != 0)
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
        float3 hitPos = rayOrigin + dir * tHit;
        float hitCamDist = length(hitPos - cam);
        if (IsHudSkinnedBatch(batchIdx) || IsSkinnedBatch(batchIdx) ||
            hitCamDist < 1.5 || hitPos.y < waterPos.y - 2.0)
        {
            float adv = max(tHit + 0.05, 0.5);
            rayOrigin = rayOrigin + dir * adv;
            remain -= adv;
            continue;
        }

        RTBatchInfo info = g_BatchInfo[batchIdx];
        uint primIdx = q.CommittedPrimitiveIndex();
        float2 bary = q.CommittedTriangleBarycentrics();
        float3x4 objectToWorld = q.CommittedObjectToWorld3x4();

        float3 albedo = float3(0.5, 0.5, 0.5);
        float3 hitN = float3(0.0, 1.0, 0.0);
        float hemi = 0.5;
        float2 lmUV = 0;
        bool skipHit = false;
        bool waterHit = false;
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
                if (texel.a < GRASS_ALPHA_CLIP)
                    skipHit = true;
                else
                    albedo = texel.rgb;
            }
            else
            {
                albedo = lerp(float3(0.08, 0.18, 0.03), float3(0.15, 0.35, 0.06), 1.0 - hitUV.y);
            }
            baked = ShadeBakedFromHemi(0.55, albedo, g_HemiColor.rgb);
        }
        else if (IsSkinnedBatch(batchIdx))
        {
            float adv = max(tHit + 0.05, 0.5);
            rayOrigin = rayOrigin + dir * adv;
            remain -= adv;
            continue;
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
            hasLmap = false;
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
            if ((mat.flags & MAT_FLAG_WATER) != 0)
                waterHit = true;
            else if (!MaterialDiffuseOpaque(mat, diffuse))
                skipHit = true;
            else
                albedo = diffuse.rgb;
            if (!waterHit && !skipHit)
            {
                baked = ShadeBakedFromMaterialLmap(mat, lmUV, albedo, hemi);
                hasLmap = (mat.flags & MAT_FLAG_HAS_LMAP) != 0 && mat.lmapIndex != INVALID_TEXTURE_INDEX;
            }
        }

        if (waterHit || skipHit)
        {
            float adv = waterHit ? max(tHit + 0.1, 2.5) : max(tHit + 0.05, 0.35);
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
        hitCol += SampleSkyW(hitN) * albedo * 0.22;
        outCol = lerp(skyRefl, hitCol, 0.52);
        shadeSteps++;
        return true;
    }

    return false;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float4 fwd4 = t_SceneColorIn.Load(int3(pixel, 0));
    float depth = t_Depth.Load(int3(pixel, 0));
    float4 guideWP = t_WorldPos.Load(int3(pixel, 0));
    if (depth <= 0.0 || IsHudSurfMark(guideWP.w))
    {
        u_SceneColor[pixel] = fwd4;
        return;
    }

    float4 classifyData = t_ClassifyWorldPos.Load(int3(pixel, 0));
    if (!IsWaterSurfMark(classifyData.w))
    {
        u_SceneColor[pixel] = fwd4;
        return;
    }

    float2 uv = (float2(pixel) + 0.5) / g_ScreenSize;
    float border = RayAttenBorder(uv, 0.015);

    float3 waterPos = classifyData.xyz;
    float3 cam = g_CameraPos.xyz;
    float waterDist = length(waterPos - cam);
    float lodDist = max(g_LodDist, 10.0);

    float3 underCol = t_UnderColor.Load(int3(pixel, 0)).rgb;

    float3 Nflat = float3(0.0, 1.0, 0.0);
    float3 N = Nflat;
    float3 nMap = DecodeNormal(t_Normal.Load(int3(pixel, 0)).xyz);
    if (dot(nMap, nMap) > 1e-4)
    {
        if (nMap.y < 0.0)
            nMap = -nMap;
        N = normalize(lerp(Nflat, nMap, 0.10));
    }

    float3 V = normalize(cam - waterPos);
    float3 Rflat = normalize(reflect(-V, Nflat));
    float3 R = normalize(reflect(-V, N));
    float towardCam = saturate(dot(R, V));
    R = normalize(lerp(R, Rflat, max(towardCam * towardCam, 0.4)));

    float cosTheta = saturate(dot(Nflat, V));
    float F = 0.04 + 0.48 * pow(1.0 - cosTheta, 5.0);
    F = saturate(F * lerp(0.97, 1.0, border));

    float3 skyRefl = SampleSkyW(R);
    float3 reflected = skyRefl;
    const bool nearHud = HudNear(int2(pixel));
    if (R.y > -0.08 && !nearHud)
    {
        float lift = lerp(0.16, 0.05, saturate(R.y * 5.0));
        float lod = saturate(waterDist / lodDist);
        if (waterDist > lodDist * 2.0)
        {
            reflected = skyRefl;
        }
        else
        {
            uint steps = (lod > 0.35) ? 14u : 9u;
            int ssrSteps = (waterDist > lodDist) ? 8 : 48;
            float3 rtCol = skyRefl;
            bool rtHit = TraceWaterReflect(waterPos + Nflat * lift, R, skyRefl, steps, waterPos, rtCol);
            float4 ssr = ScreenReflectWater(waterPos + Nflat * lift, R, waterPos, skyRefl, ssrSteps);
            if (rtHit)
                reflected = lerp(lerp(skyRefl, ssr.rgb, ssr.a * 0.55), rtCol, 0.82);
            else
                reflected = lerp(skyRefl, ssr.rgb, saturate(ssr.a));
        }
    }

    float3 sunDir = normalize(-g_SunDir_Intensity.xyz);
    float3 sunColor = g_SunColor_SkyWeight.rgb * saturate(g_SunDir_Intensity.w);
    reflected += sunColor * pow(saturate(dot(R, sunDir)), 256.0) * 0.7;

    float3 skyDown = SampleSkyW(float3(0.0, 1.0, 0.0));
    float3 refr = underCol * 1.35 + skyDown * 0.20;
    if (!nearHud)
        refr = lerp(refr, fwd4.rgb, 0.18);
    refr = max(refr, skyDown * 0.12);
    float3 color = lerp(refr, reflected, F);
    u_SceneColor[pixel] = float4(color, 1.0);
}
