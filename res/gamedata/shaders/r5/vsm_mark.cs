#define SM_5_0
#include "common.h"
#include "vsm_common.h"
#include "vsm_params.h"

cbuffer VsmMarkParams : register(b5)
{
    float4x4 g_InvViewProj;
    float4 g_Screen;
    uint g_MarkStep;
    uint g_LodBias;
    uint2 g_MarkPad;
};

Texture2D<float> g_Depth : register(t0);
RWStructuredBuffer<uint> g_Needed : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    int2 px = int2(dtID.xy) * int(g_MarkStep);
    if (px.x >= int(g_Screen.x) || px.y >= int(g_Screen.y))
        return;

    float zndc = g_Depth.Load(int3(px, 0));
    if (zndc <= 0.0)
        return;

    float2 uv = (float2(px) + 0.5) * g_Screen.zw;
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - 2.0 * uv.y, zndc, 1.0);
    float3 wp = vsmReconstructPos(g_InvViewProj, clip);
    float3 lp = mul(vsm_view, float4(wp, 1.0)).xyz;

    float2 luv;
    int2 page;
    int L = vsmSelect(lp.xy, vsm_level, luv, page);
    if (L < 0)
        return;

    int Lb = min(L + int(g_LodBias), VSM_LEVELS - 1);
    if (Lb != L)
    {
        luv = (lp.xy - vsm_level[Lb].xy) / vsm_level[Lb].z;
        page = clamp(int2(floor(luv * float(VSM_PAGES_AXIS))), int2(0, 0), int2(VSM_PAGES_AXIS - 1, VSM_PAGES_AXIS - 1));
        L = Lb;
    }

    for (int level = L; level < VSM_LEVELS; ++level)
    {
        float2 t = (lp.xy - vsm_level[level].xy) / vsm_level[level].z;
        if (any(t < 0.0) || any(t >= 1.0))
            continue;
        int2 p = int2(floor(t * float(VSM_PAGES_AXIS)));
        uint prev;
        InterlockedOr(g_Needed[vsmPageIndex(level, p)], 1u, prev);
    }
}
