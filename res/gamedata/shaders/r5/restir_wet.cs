#include "common.h"
#include "bindless_common.h"
#include "rt_common.h"
#include "rt_material_alpha.h"
#include "restir_gi_common.h"
#include "shared/surface_marks.h"

cbuffer WetParams : register(b5) {
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float g_DeltaTime;
    float g_RainFactor;
    float g_DryRate;
    float g_MaxWet;
    uint2 g_Pad;
};

Texture2D<float> t_Depth : register(t0);
Texture2D<float4> t_Normal : register(t1);
Texture2D<float4> t_WorldPos : register(t6);
RaytracingAccelerationStructure g_SceneTLAS : register(t2);
StructuredBuffer<RTBatchInfo> g_BatchInfo : register(t3);
ByteAddressBuffer g_MegaVB : register(t4);
ByteAddressBuffer g_MegaIB : register(t5);
RWTexture2D<float> u_WetAccum : register(u0);
RWTexture2D<float> u_SkyOpen : register(u1);

bool IsSkyOpen(float3 origin)
{
    float3 rayOrigin = origin;
    float remaining = 400.0;
    for (uint si = 0; si < 4u; si++) {
        RayDesc ray;
        ray.Origin = rayOrigin;
        ray.Direction = float3(0.0, 1.0, 0.0);
        ray.TMin = 0.05;
        ray.TMax = remaining;

        RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
        q.TraceRayInline(g_SceneTLAS, RAY_FLAG_NONE, RT_MASK_SHADOW_MAPPED, ray);
        while (q.Proceed()) {
            if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE) {
                uint candBatch = q.CandidateInstanceID() + q.CandidateGeometryIndex();
                if (MegaMaterialOpaque(g_MegaVB, g_MegaIB, g_BatchInfo, candBatch,
                        q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics()))
                    q.CommitNonOpaqueTriangleHit();
            }
        }

        if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
            return true;

        uint batchIdx = q.CommittedInstanceID() + q.CommittedGeometryIndex();
        RTBatchInfo info = g_BatchInfo[batchIdx];
        MaterialData mat = g_Materials[info.materialID];
        float hitT = q.CommittedRayT();
        if ((mat.flags & MAT_FLAG_WATER) != 0) {
            rayOrigin = rayOrigin + float3(0.0, 1.0, 0.0) * (hitT + 0.02);
            remaining = max(remaining - hitT - 0.02, 0.0);
            continue;
        }
        if ((mat.flags & MAT_FLAG_EMISSIVE) != 0)
            return false;
        float2 hitUV = GetHitUV(g_MegaVB, g_MegaIB, info,
            q.CommittedPrimitiveIndex(), q.CommittedTriangleBarycentrics());
        float4 diffuse = SampleDiffuseLevel(mat, hitUV);
        if (!MaterialDiffuseOpaque(mat, diffuse)) {
            rayOrigin = rayOrigin + float3(0.0, 1.0, 0.0) * (hitT + 0.02);
            remaining = max(remaining - hitT - 0.02, 0.0);
            continue;
        }
        return false;
    }
    return false;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= (uint)g_ScreenSize.x || pixel.y >= (uint)g_ScreenSize.y)
        return;

    float depth = t_Depth.Load(int3(pixel, 0));
    float wet = u_WetAccum[pixel];
    if (depth <= 0.0 || depth >= 1.0) {
        u_WetAccum[pixel] = max(wet - g_DryRate * g_DeltaTime, 0.0);
        u_SkyOpen[pixel] = 1.0;
        return;
    }

    float3 N = normalize(t_Normal.Load(int3(pixel, 0)).xyz);
    float surfMark = t_WorldPos.Load(int3(pixel, 0)).w;
    float upward = saturate(N.y);
    if (IsVegSurfMark(surfMark))
        upward = max(upward, 0.8);
    float2 uv = (float2(pixel) + 0.5) / g_ScreenSize;
    float3 worldPos = ReconstructWorldPosReverseZ(uv, depth, g_InvViewProj);
    float3 biased = worldPos + float3(0.0, 0.05, 0.0);

    float cover = IsSkyOpen(biased) ? 1.0 : 0.0;
    float prevOpen = u_SkyOpen[pixel];
    float openBlend = (cover < prevOpen) ? 0.55 : 0.25;
    u_SkyOpen[pixel] = lerp(prevOpen, cover, openBlend);
    float add = g_RainFactor * upward * cover * g_DeltaTime;
    wet = saturate(wet + add - g_DryRate * g_DeltaTime);
    u_WetAccum[pixel] = min(wet, g_MaxWet);
}
