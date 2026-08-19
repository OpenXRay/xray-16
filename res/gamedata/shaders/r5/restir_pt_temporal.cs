#include "common.h"
#include "rt_common.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "shared/surface_marks.h"
#include "restir_gi_common.h"
#include "restir_pt_common.h"

cbuffer ReSTIRPTTemporal : register(b5) {
    float4x4 g_InvViewProj;
    float4x4 g_PrevInvViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    uint g_FrameIndex;
    float g_CCap;
    uint g_HasPrev;
    uint g_Pad;
};

Texture2D<uint4> t_CurrA : register(t0);
Texture2D<uint4> t_CurrB : register(t1);
Texture2D<uint4> t_PrevA : register(t2);
Texture2D<uint4> t_PrevB : register(t3);
Texture2D<float4> t_MotionVectors : register(t4);
Texture2D<float> t_Depth : register(t5);
Texture2D<float4> t_Normal : register(t6);
Texture2D<float4> t_PrevNormal : register(t7);
Texture2D<float4> t_WorldPos : register(t8);
Texture2D<float> t_PrevDepth : register(t9);
Texture2D<float> t_DupMap : register(t10);
Texture2D<float4> t_BaseColor : register(t11);
Texture2D<float> t_SkyOpen : register(t12);

RWTexture2D<uint4> u_OutA : register(u0);
RWTexture2D<uint4> u_OutB : register(u1);
RWTexture2D<float4> u_NoisyDiffuse : register(u2);
RWTexture2D<float4> u_NoisySpecular : register(u3);
RWTexture2D<float> u_HitDistance : register(u4);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint2 pixel = id.xy;
    uint w = (uint)g_ScreenSize.x;
    uint h = (uint)g_ScreenSize.y;
    if (pixel.x >= w || pixel.y >= h)
        return;

    float2 giSize = g_ScreenSize;
    uint fw = 0, fh = 0;
    t_Depth.GetDimensions(fw, fh);
    float2 fullSize = float2(max(fw, 1u), max(fh, 1u));
    float depth = RestirLoadDepth(t_Depth, pixel, giSize, fullSize);
    if (depth <= 0.0 || depth >= 1.0) {
        u_OutA[pixel] = 0;
        u_OutB[pixel] = 0;
        return;
    }

    float2 uv = (float2(pixel) + 0.5) * g_InvScreenSize;
    float4 wp = RestirLoadTex4(t_WorldPos, pixel, giSize, fullSize);
    float4 bc = RestirLoadTex4(t_BaseColor, pixel, giSize, fullSize);
    float3 worldPos = ResolveGBufferWorldPos(uv, depth, wp, g_InvViewProj);
    float3 N = normalize(RestirLoadTex4(t_Normal, pixel, giSize, fullSize).xyz);
    float surf = SurfMarkFromGBuffer(wp.w, bc.a);
    PTReservoir curr = UnpackPTReservoir(t_CurrA[pixel], t_CurrB[pixel]);
    PTReservoir output = curr;
    uint rng = pcg_hash(pixel.x + pixel.y * 9001u + g_FrameIndex * 17u);

    int2 fullPx = RestirFullPixel(pixel, giSize, fullSize);
    float4 mv4 = t_MotionVectors.Load(int3(fullPx, 0));
    float2 motion = mv4.xy;
    float2 dual = mv4.zw;
    float D = t_DupMap.Load(int3(pixel, 0));
    float cap = PTCapFromDup(D, g_CCap);
    float motionPx = length(motion * fullSize);
    float viewDist = length(worldPos - g_CameraPos.xyz);

    if (g_HasPrev != 0 && !IsCharSurfMark(surf) && !IsHudSurfMark(surf) && motionPx < 16.0) {
        float2 prevUV = uv + motion;
        if (depth <= 1e-7 && (any(prevUV < 0) || any(prevUV >= 1)))
            prevUV = uv + dual;
        if (all(prevUV >= 0) && all(prevUV < 1)) {
            int2 pp = clamp(int2(prevUV * giSize), int2(0, 0), int2(w, h) - 1);
            int2 pf = clamp(int2(prevUV * fullSize), int2(0, 0), int2(fullSize) - 1);
            float pd = t_PrevDepth.Load(int3(pf, 0));
            float3 pN = normalize(t_PrevNormal.Load(int3(pf, 0)).xyz);
            float2 puv = (float2(pf) + 0.5) / fullSize;
            float3 prevWorld = ReconstructWorldPosReverseZ(puv, pd, g_PrevInvViewProj);
            float posTol = (motionPx < 1.0) ? 0.05 : 0.035;
            bool valid = pd > 0 && pd < 1 && dot(N, pN) > 0.9 &&
                length(worldPos - prevWorld) < posTol * max(min(viewDist, 4.0), 1.0);
            float skyOpenC = saturate(RestirLoadTex1(t_SkyOpen, pixel, giSize, fullSize));
            float skyOpenP = saturate(t_SkyOpen.Load(int3(pf, 0)));
            valid = valid && abs(skyOpenC - skyOpenP) <= 0.25;
            if (valid) {
                PTReservoir prev = UnpackPTReservoir(t_PrevA[pp], t_PrevB[pp]);
                if (IsPTReservoirValid(prev)) {
                    float jac = HybridShiftJacobian(prev.rcN, worldPos, prevWorld, prev.rcPos);
                    float3 wi = normalize(prev.rcPos - worldPos);
                    if (dot(N, wi) > 0.02) {
                        uint m = min(prev.M, (uint)max(cap, 1.0));
                        if (motionPx > 6.0)
                            m = max(1u, m / 4u);
                        else if (motionPx > 2.0)
                            m = max(1u, m / 2u);
                        float w = Luminance(prev.Lo) * prev.W * m * jac;
                        if (!IsPTReservoirValid(output)) {
                            output = prev;
                            output.M = m;
                            output.W = prev.W * jac;
                        } else {
                            PTReservoirUpdate(output, w, prev, rng);
                            output.M = min(output.M + m, (uint)max(cap, 1.0));
                        }
                    }
                }
            }
        }
    }

    if (IsPTReservoirValid(output) && output.M > 0) {
        float t = max(Luminance(output.Lo), 1e-4);
        output.W = min(output.targetPdf / (t * output.M), 4.0);
        output.age = min(output.age + 1, 255);
    }
    uint4 A, B;
    PackPTReservoir(output, A, B);
    u_OutA[pixel] = A;
    u_OutB[pixel] = B;

    float sss = 0;
    float metallic = UnpackGBufferMetallic(bc.a, IsHudSurfMark(surf) || IsCharSurfMark(surf), sss);
    float3 diff = ShadePTReservoir(output, worldPos, N, bc.rgb, metallic);
    u_NoisyDiffuse[pixel] = float4(min(diff, RESTIR_MAX_RADIANCE), 1);
    u_NoisySpecular[pixel] = float4(min(output.Lo * output.W * 0.15, RESTIR_MAX_RADIANCE), output.hitDist);
    u_HitDistance[pixel] = output.hitDist;
}
