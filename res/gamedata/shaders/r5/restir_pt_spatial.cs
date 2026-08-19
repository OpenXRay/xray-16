#include "common.h"
#include "rt_common.h"
#include "shared/pbr_brdf.h"
#include "shared/basecolor_pack.h"
#include "shared/surface_marks.h"
#include "restir_gi_common.h"
#include "restir_pt_common.h"

cbuffer ReSTIRPTSpatial : register(b5) {
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
    uint g_FrameIndex;
    uint g_PairIndex;
    uint g_FlipX;
    uint g_FlipY;
    int g_OffX;
    int g_OffY;
    uint g_Pass;
    uint g_Pad;
};

Texture2D<uint4> t_CurrA : register(t0);
Texture2D<uint4> t_CurrB : register(t1);
Texture2D<float> t_Depth : register(t2);
Texture2D<float4> t_Normal : register(t3);
Texture2D<float4> t_WorldPos : register(t4);
Texture2D<float2> t_Pair : register(t5);
Texture2D<float4> t_BaseColor : register(t6);

RWTexture2D<uint4> u_OutA : register(u0);
RWTexture2D<uint4> u_OutB : register(u1);
RWTexture2D<float4> u_NoisyDiffuse : register(u2);
RWTexture2D<float4> u_NoisySpecular : register(u3);

int2 PairNeighbor(uint2 pixel, uint w, uint h)
{
    uint2 dims;
    t_Pair.GetDimensions(dims.x, dims.y);
    int2 tc = int2(pixel) + int2(g_OffX, g_OffY);
    if (g_FlipX)
        tc.x = (int)w - 1 - tc.x;
    if (g_FlipY)
        tc.y = (int)h - 1 - tc.y;
    int2 uv = int2((uint)tc.x % max(dims.x, 1u), (uint)tc.y % max(dims.y, 1u));
    float2 d = t_Pair.Load(int3(uv, 0));
    int2 nb = int2(pixel) + int2(round(d.x), round(d.y));
    return clamp(nb, int2(0, 0), int2(w, h) - 1);
}

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
    PTReservoir curr = UnpackPTReservoir(t_CurrA[pixel], t_CurrB[pixel]);
    if (depth <= 0 || depth >= 1) {
        u_OutA[pixel] = t_CurrA[pixel];
        u_OutB[pixel] = t_CurrB[pixel];
        return;
    }

    float2 uv = (float2(pixel) + 0.5) * g_InvScreenSize;
    float4 wp = RestirLoadTex4(t_WorldPos, pixel, giSize, fullSize);
    float3 worldPos = ResolveGBufferWorldPos(uv, depth, wp, g_InvViewProj);
    float3 N = normalize(RestirLoadTex4(t_Normal, pixel, giSize, fullSize).xyz);
    int2 np = PairNeighbor(pixel, w, h);
    PTReservoir nb = UnpackPTReservoir(t_CurrA[np], t_CurrB[np]);
    uint rng = pcg_hash(pixel.x + pixel.y * 577u + g_FrameIndex * 31u + g_PairIndex);

    float4 bc = RestirLoadTex4(t_BaseColor, pixel, giSize, fullSize);
    float surf = SurfMarkFromGBuffer(wp.w, bc.a);
    float4 nbWp = RestirLoadTex4(t_WorldPos, uint2(np), giSize, fullSize);
    float nbSurf = SurfMarkFromGBuffer(nbWp.w, RestirLoadTex4(t_BaseColor, uint2(np), giSize, fullSize).a);
    bool skipNb = IsCharSurfMark(surf) || IsHudSurfMark(surf) || IsCharSurfMark(nbSurf) || IsHudSurfMark(nbSurf);

    if (!skipNb && IsPTReservoirValid(nb)) {
        float nd = RestirLoadDepth(t_Depth, uint2(np), giSize, fullSize);
        float3 nN = normalize(RestirLoadTex4(t_Normal, uint2(np), giSize, fullSize).xyz);
        if (nd > 0 && nd < 1 && dot(N, nN) > 0.9 && abs(nd - depth) * 80.0 < 1.0) {
            float2 nuv = (float2(np) + 0.5) * g_InvScreenSize;
            float3 npos = ResolveGBufferWorldPos(nuv, nd, nbWp, g_InvViewProj);
            float jac = HybridShiftJacobian(nb.rcN, worldPos, npos, nb.rcPos);
            float3 wi = normalize(nb.rcPos - worldPos);
            if (dot(N, wi) > 0.02) {
                float w = Luminance(nb.Lo) * nb.W * jac;
                if (!IsPTReservoirValid(curr))
                    curr = nb;
                else
                    PTReservoirUpdate(curr, w, nb, rng);
            }
        }
    }

    if (IsPTReservoirValid(curr) && curr.M > 0) {
        float t = max(Luminance(curr.Lo), 1e-4);
        curr.W = min(curr.targetPdf / (t * curr.M), 4.0);
    }
    uint4 A, B;
    PackPTReservoir(curr, A, B);
    u_OutA[pixel] = A;
    u_OutB[pixel] = B;
    float sss = 0;
    float metallic = UnpackGBufferMetallic(bc.a, IsHudSurfMark(surf) || IsCharSurfMark(surf), sss);
    float3 diff = ShadePTReservoir(curr, worldPos, N, bc.rgb, metallic);
    u_NoisyDiffuse[pixel] = float4(min(diff, RESTIR_MAX_RADIANCE), 1);
    u_NoisySpecular[pixel] = float4(min(curr.Lo * curr.W * 0.15, RESTIR_MAX_RADIANCE), curr.hitDist);
}
