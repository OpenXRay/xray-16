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
    uint g_Pad2;
};

RaytracingAccelerationStructure g_SceneTLAS : register(t1);
StructuredBuffer<RTBatchInfo> g_BatchInfo : register(t2);
ByteAddressBuffer g_MegaVB : register(t3);
ByteAddressBuffer g_MegaIB : register(t4);
TextureCube<float4> g_Sky0 : register(t5);
TextureCube<float4> g_Sky1 : register(t6);

RWTexture2D<float4> g_Accumulation : register(u0);
RWTexture2D<float4> g_Output : register(u1);

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

float3 GetHitAlbedo(RTBatchInfo info, float2 hitUV, uint batchIdx)
{
    if (IsTerrainBatch(batchIdx)) {
        TerrainMaterialData tmat = g_TerrainMaterials[info.materialID];
        return SampleTerrainAlbedo(tmat, hitUV);
    }

    MaterialData mat = g_Materials[info.materialID];
    return SampleDiffuse(mat, hitUV).rgb;
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

        RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
        q.TraceRayInline(g_SceneTLAS, RAY_FLAG_NONE, 0xFF, ray);
        q.Proceed();

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
        float2 hitUV = GetHitUV(g_MegaVB, g_MegaIB, info, primIdx, bary);
        float3 localN = GetHitNormal(g_MegaVB, g_MegaIB, info, primIdx, bary);
        float3 hitN = TransformNormalToWorld(localN, objectToWorld);

        if (dot(hitN, direction) > 0)
            hitN = -hitN;

        float3 albedo = GetHitAlbedo(info, hitUV, batchIdx);
        float3 hitPos = origin + direction * hitT;

        {
            RayDesc shadowRay;
            shadowRay.Origin = hitPos + hitN * 0.002;
            shadowRay.Direction = sunDir;
            shadowRay.TMin = 0.001;
            shadowRay.TMax = 10000.0;

            RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> shadowQ;
            shadowQ.TraceRayInline(g_SceneTLAS, RAY_FLAG_NONE, 0xFF, shadowRay);
            shadowQ.Proceed();

            if (shadowQ.CommittedStatus() != COMMITTED_TRIANGLE_HIT) {
                float NdotL = max(0.0, dot(hitN, sunDir));
                radiance += throughput * albedo * NdotL * sunColor;
            }
        }

        float2 u = float2(rand_float(rng), rand_float(rng));
        float3 bounceDir = cosine_weighted_hemisphere(u, hitN);
        throughput *= albedo;

        if (bounce >= 3) {
            float p = max(throughput.r, max(throughput.g, throughput.b));
            if (p < 0.01) break;
            if (rand_float(rng) > p) break;
            throughput /= p;
        }

        origin = hitPos + hitN * 0.002;
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
