#include "common.h"
#include "rt_common.h"
#include "shared/pbr_brdf.h"
#include "shared/nrd_helpers.h"
#include "shared/basecolor_pack.h"
#include "shared/surface_marks.h"
#include "restir_gi_common.h"

cbuffer ReSTIRSpecParams : register(b5) {
    float4x4 g_InvViewProj;
    float4x4 g_PrevInvViewProj;
    float4x4 g_PrevViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    uint g_FrameIndex;
    uint g_SpatialSamples;
    float g_SpatialRadius;
    uint g_HasPrev;
};

Texture2D<float4> t_CurrA : register(t0);
Texture2D<float4> t_CurrB : register(t1);
Texture2D<float4> t_PrevA : register(t2);
Texture2D<float4> t_PrevB : register(t3);
Texture2D<float2> t_MotionVectors : register(t4);
Texture2D<float> t_Depth : register(t5);
Texture2D<float4> t_Normal : register(t6);
Texture2D<float4> t_PrevNormal : register(t7);
Texture2D<float4> t_BaseColor : register(t8);
Texture2D<float4> t_WorldPos : register(t9);
Texture2D<float> t_PrevDepth : register(t10);

RWTexture2D<float4> u_OutA : register(u0);
RWTexture2D<float4> u_OutB : register(u1);
RWTexture2D<float4> u_NoisySpecular : register(u2);

float2 ProjectToUv(float4x4 viewProj, float3 worldPos)
{
    float4 clip = mul(viewProj, float4(worldPos, 1.0));
    float2 ndc = clip.xy / max(abs(clip.w), 1e-5);
    ndc.y = -ndc.y;
    return ndc * 0.5 + 0.5;
}

float SpecReuseScale(float roughness)
{
    if (roughness < 0.3)
        return 1.0;
    if (roughness > 0.6)
        return 0.25;
    return lerp(1.0, 0.25, saturate((roughness - 0.3) / 0.3));
}

float SpecTargetLum(GIReservoir r, float3 worldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness)
{
    if (!IsReservoirValid(r))
        return 0;
    float3 F0 = CalculateF0(albedo, metallic);
    float3 Fenv = NRD_EnvironmentTerm_Rtg(F0, abs(dot(N, V)), roughness);
    return Luminance(r.Lo * Fenv);
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
    uint fullW = 0, fullH = 0;
    t_Depth.GetDimensions(fullW, fullH);
    float2 fullSize = float2(max(fullW, 1u), max(fullH, 1u));
    float depth = RestirLoadDepth(t_Depth, pixel, giSize, fullSize);
    if (depth <= 0.0 || depth >= 1.0) {
        u_OutA[pixel] = 0;
        u_OutB[pixel] = 0;
        u_NoisySpecular[pixel] = 0;
        return;
    }

    float4 baseColorData = RestirLoadTex4(t_BaseColor, pixel, giSize, fullSize);
    float4 worldPosData = RestirLoadTex4(t_WorldPos, pixel, giSize, fullSize);
    float surfMark = SurfMarkFromGBuffer(worldPosData.w, baseColorData.a);
    if (IsWaterSurfMark(surfMark)) {
        u_OutA[pixel] = 0;
        u_OutB[pixel] = 0;
        u_NoisySpecular[pixel] = 0;
        return;
    }

    float2 uv = (float2(pixel) + 0.5) * g_InvScreenSize;
    float3 worldPos = ResolveGBufferWorldPos(uv, depth, worldPosData, g_InvViewProj);
    float4 normalData = RestirLoadTex4(t_Normal, pixel, giSize, fullSize);
    float3 N = normalize(normalData.xyz);
    float roughness = max(abs(normalData.w), MIN_ROUGHNESS);
    float3 albedo = baseColorData.rgb;
    float sss = 0;
    float metallic = UnpackGBufferMetallic(baseColorData.a, IsHudSurfMark(surfMark) || IsCharSurfMark(surfMark), sss);
    float3 V = normalize(g_CameraPos.xyz - worldPos);
    float reuse = SpecReuseScale(roughness);

    GIReservoir curr = UnpackReservoirAB(t_CurrA.Load(int3(pixel, 0)), t_CurrB.Load(int3(pixel, 0)));
    uint rng = pcg_hash(pixel.x + pixel.y * 7919u + g_FrameIndex * 33461u);
    GIReservoir output = EmptyReservoir();
    float targetCurr = SpecTargetLum(curr, worldPos, N, V, albedo, metallic, roughness);
    if (targetCurr > 0) {
        output = curr;
        output.w_sum = targetCurr * curr.W;
        output.M = 1;
    }

    int2 fullPx = RestirFullPixel(pixel, giSize, fullSize);
    float2 motion = t_MotionVectors.Load(int3(fullPx, 0));
    if (g_HasPrev != 0 && reuse > 0.05 && !IsCharSurfMark(surfMark)) {
        float3 virt = curr.samplePos;
        if (!IsReservoirValid(curr))
            virt = worldPos + V * 4.0;
        float2 prevUV = ProjectToUv(g_PrevViewProj, virt);
        if (any(prevUV < 0.0) || any(prevUV >= 1.0))
            prevUV = uv + motion;
        if (all(prevUV >= 0.0) && all(prevUV < 1.0)) {
            int2 prevPixel = clamp(int2(prevUV * giSize), int2(0, 0), int2(width, height) - 1);
            int2 prevFull = clamp(int2(prevUV * fullSize), int2(0, 0), int2(fullSize) - 1);
            float prevDepth = t_PrevDepth.Load(int3(prevFull, 0));
            float3 prevN = normalize(t_PrevNormal.Load(int3(prevFull, 0)).xyz);
            if (prevDepth > 0.0 && prevDepth < 1.0 && dot(N, prevN) > 0.85) {
                float2 prevNdc = (float2(prevFull) + 0.5) / fullSize;
                float3 prevWorld = ReconstructWorldPosReverseZ(prevNdc, prevDepth, g_PrevInvViewProj);
                GIReservoir prev = UnpackReservoirAB(t_PrevA.Load(int3(prevPixel, 0)), t_PrevB.Load(int3(prevPixel, 0)));
                float targetPrev = SpecTargetLum(prev, worldPos, N, V, albedo, metallic, roughness);
                if (targetPrev > 0) {
                    float jac = clamp(JacobianReconnectionShift(prev.sampleNormal, worldPos, prevWorld, prev.samplePos), 0.25, 4.0);
                    uint cap = TemporalMClamp(prev.M, prev.age, 8u);
                    cap = max(1u, (uint)round((float)cap * reuse));
                    float w = targetPrev * prev.W * cap * jac;
                    if (!IsReservoirValid(output)) {
                        output = prev;
                        output.w_sum = targetPrev * prev.W * jac;
                        output.M = cap;
                    } else {
                        ReservoirUpdate(output, w, prev.samplePos, prev.sampleNormal, prev.Lo, prev.lightId, rng);
                        output.M += cap - 1;
                    }
                }
            }
        }
    }

    if (g_SpatialSamples > 0 && reuse > 0.2) {
        uint n = min(g_SpatialSamples, 4u);
        float rad = g_SpatialRadius * lerp(0.35, 1.0, saturate(1.0 - roughness * 1.5));
        [loop]
        for (uint i = 0; i < n; i++) {
            float2 u = float2(rand_float(rng), rand_float(rng));
            float ang = u.x * 6.2831853;
            float r = sqrt(u.y) * rad;
            int2 np = int2(pixel) + int2(round(float2(cos(ang), sin(ang)) * r));
            if (np.x < 0 || np.y < 0 || np.x >= (int)width || np.y >= (int)height)
                continue;
            float4 nn = RestirLoadTex4(t_Normal, uint2(np), giSize, fullSize);
            if (dot(N, normalize(nn.xyz)) < 0.9)
                continue;
            if (abs(abs(nn.w) - roughness) > 0.12)
                continue;
            GIReservoir nb = UnpackReservoirAB(t_CurrA.Load(int3(np, 0)), t_CurrB.Load(int3(np, 0)));
            float tnb = SpecTargetLum(nb, worldPos, N, V, albedo, metallic, roughness);
            if (tnb <= 0)
                continue;
            float4 nwp = RestirLoadTex4(t_WorldPos, uint2(np), giSize, fullSize);
            float nd = RestirLoadDepth(t_Depth, uint2(np), giSize, fullSize);
            float2 nuv = (float2(np) + 0.5) * g_InvScreenSize;
            float3 npos = ResolveGBufferWorldPos(nuv, nd, nwp, g_InvViewProj);
            float jac = clamp(JacobianReconnectionShift(nb.sampleNormal, worldPos, npos, nb.samplePos), 0.25, 4.0);
            ReservoirUpdate(output, tnb * nb.W * jac, nb.samplePos, nb.sampleNormal, nb.Lo, nb.lightId, rng);
        }
    }

    float outT = SpecTargetLum(output, worldPos, N, V, albedo, metallic, roughness);
    output.Lo = min(output.Lo, RESTIR_MAX_RADIANCE);
    output.W = (outT > 0 && output.M > 0) ? min(output.w_sum / (outT * output.M), 4.0) : 0;
    output.age = min(output.age + 1, 127);
    float4 A, B;
    PackReservoirAB(output, A, B);
    u_OutA[pixel] = A;
    u_OutB[pixel] = B;

    float3 F0 = CalculateF0(albedo, metallic);
    float3 Fenv = NRD_EnvironmentTerm_Rtg(F0, abs(dot(N, V)), roughness);
    float3 spec = IsReservoirValid(output) ? min(output.Lo * Fenv * output.W, RESTIR_MAX_RADIANCE) : 0;
    float hitDist = IsReservoirValid(output) ? length(output.samplePos - worldPos) : 0;
    u_NoisySpecular[pixel] = float4(spec, hitDist);
}
