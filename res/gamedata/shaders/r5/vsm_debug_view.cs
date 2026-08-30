#define SM_5_0
#include "common.h"
#include "vsm_common.h"
#include "vsm_params.h"

cbuffer VsmDebugParams : register(b5)
{
    float4x4 g_InvViewProj;
    float4 g_Screen;
    uint g_Mode;
    uint3 g_DebugPad;
};

Texture2D<float> g_Depth : register(t0);
StructuredBuffer<uint> g_Needed : register(t1);
StructuredBuffer<uint> g_PageTable : register(t2);
StructuredBuffer<uint> g_SlotDirty : register(t3);
Texture2D<float4> g_Mask : register(t30);
RWTexture2D<float4> g_Output : register(u0);

static const float3 kLevelColors[VSM_LEVELS] = {
    float3(0.90, 0.20, 0.20), float3(0.90, 0.60, 0.10), float3(0.90, 0.90, 0.20),
    float3(0.20, 0.80, 0.20), float3(0.20, 0.80, 0.90), float3(0.30, 0.40, 0.90)
};

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    int2 px = int2(dtID.xy);
    if (px.x >= int(g_Screen.x) || px.y >= int(g_Screen.y))
        return;

    float3 color = float3(0.02, 0.02, 0.03);
    float zndc = g_Depth.Load(int3(px, 0));
    if (g_Mode == 4u)
    {
        float m = g_Mask.Load(int3(px, 0)).r;
        g_Output[px] = float4(m, m, m, 1.0);
        return;
    }
    if (zndc > 0.0)
    {
        float2 uv = (float2(px) + 0.5) * g_Screen.zw;
        float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - 2.0 * uv.y, zndc, 1.0);
        float4 world = mul(g_InvViewProj, clip);
        world.xyz /= world.w;
        float3 lp = mul(vsm_view, float4(world.xyz, 1.0)).xyz;

        float2 luv;
        int2 page;
        int L = vsmSelect(lp.xy, vsm_level, luv, page);
        if (L < 0)
        {
            color = float3(1.0, 0.0, 1.0);
        }
        else
        {
            float pageTexel = vsm_level[L].z / float(VSM_PAGES_AXIS);
            int2 absPage = int2(floor(lp.xy / pageTexel));
            float check = ((absPage.x + absPage.y) & 1) != 0 ? 1.0 : 0.65;
            uint vp = uint(vsmPageIndex(L, page));
            if (g_Mode == 3u)
            {
                uint slot = g_PageTable[vp];
                uint dirty = slot == VSM_UNMAPPED ? 0u : g_SlotDirty[slot];
                if (slot == VSM_UNMAPPED)
                    color = float3(0.35, 0.35, 0.35) * check;
                else if (dirty == 1u)
                    color = float3(1.0, 1.0, 1.0);
                else if (dirty == 2u)
                    color = float3(0.95, 0.55, 0.1);
                else if (dirty != 0u)
                    color = float3(0.9, 0.2, 0.9);
                else
                    color = float3(0.15, 0.6, 0.2) * check;
            }
            else
            {
                color = kLevelColors[L] * check;
                if (g_Needed[vp] == 0u)
                    color *= 0.25;
            }
            float2 f = frac(lp.xy / pageTexel);
            if (min(f.x, f.y) < 0.03 || max(f.x, f.y) > 0.97)
                color *= 0.5;
        }
    }
    g_Output[px] = float4(color, 1.0);
}
