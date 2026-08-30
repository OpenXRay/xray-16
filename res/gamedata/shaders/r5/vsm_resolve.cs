#define SM_5_0
#include "common.h"
#include "vsm_common.h"
#include "vsm_params.h"

cbuffer VsmResolveParams : register(b5)
{
    float4x4 g_InvViewProj;
    float4x4 g_PrevViewProj;
    float4 g_PrevCamPos;
    float4 g_CurCamPos;
    float4 g_Screen;
    float4 g_Params;
    float4 g_Params2;
    float4 g_Params3;
    float4 g_Params4;
};

Texture2D<float> g_Depth : register(t0);
Texture2D<float> g_Atlas : register(t1);
StructuredBuffer<uint> g_PageTable : register(t2);
Texture2D<float4> g_History : register(t3);
RWTexture2D<float4> g_Mask : register(u0);

static const float2 kDimS = float2(float(VSM_ATLAS_W_S), float(VSM_ATLAS_H_S));
static const float kTexel = 1.0 / float(VSM_PAGE_SIZE);
static const float kInset = 0.5 / float(VSM_PAGE_SIZE);

float sampleVSM(float3 wp, out bool noPage)
{
    noPage = false;
    float3 lp = mul(vsm_view, float4(wp, 1.0)).xyz;
    float2 luv;
    int2 page;
    int L = vsmSelect(lp.xy, vsm_level, luv, page);
    if (L < 0)
        return 1.0;
    uint slot = VSM_UNMAPPED;
    for (; L < VSM_LEVELS; ++L)
    {
        float2 t = (lp.xy - vsm_level[L].xy) / vsm_level[L].z;
        page = clamp(int2(floor(t * float(VSM_PAGES_AXIS))), int2(0, 0), int2(VSM_PAGES_AXIS - 1, VSM_PAGES_AXIS - 1));
        slot = g_PageTable[vsmPageIndex(L, page)];
        if (slot < uint(VSM_MAX_PHYS_S))
        {
            luv = t;
            break;
        }
    }
    if (L >= VSM_LEVELS)
    {
        noPage = true;
        return 1.0;
    }

    float2 pageLocal = luv * float(VSM_PAGES_AXIS) - float2(page);
    float2 base = float2(float(slot % uint(VSM_ATLAS_W_S)), float(slot / uint(VSM_ATLAS_W_S)));
    float zHere = 1.0 - (lp.z - vsm_zparams.x) * vsm_zparams.y;
    float bias = vsm_zparams.z;

    float lit = 0.0;
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx)
    {
        float2 pl = clamp(pageLocal + float2(float(dx), float(dy)) * kTexel, float2(kInset, kInset), float2(1.0 - kInset, 1.0 - kInset));
        float occ = g_Atlas.SampleLevel(smp_nofilter, (base + pl) / kDimS, 0).r;
        lit += (occ > zHere + bias) ? 0.0 : 1.0;
    }
    return lit * (1.0 / 9.0);
}

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    int2 px = int2(dtID.xy);
    if (px.x >= int(g_Screen.x) || px.y >= int(g_Screen.y))
        return;

    float zndc = g_Depth.Load(int3(px, 0));
    if (zndc <= 0.0)
    {
        g_Mask[px] = float4(1.0, 1e6, 1.0, 0.0);
        return;
    }

    float2 uv = (float2(px) + 0.5) * g_Screen.zw;
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - 2.0 * uv.y, zndc, 1.0);
    float4 world = mul(g_InvViewProj, clip);
    float3 wp = world.xyz / world.w;

    bool noPage;
    float cur = sampleVSM(wp, noPage);
    float dist = length(wp - g_CurCamPos.xyz);

    float4 hist = float4(0.0, 0.0, 0.0, 0.0);
    bool histOK = false;
    float motionPx = 0.0;
    if (g_Params.z > 0.5)
    {
        float4 pc = mul(g_PrevViewProj, float4(wp, 1.0));
        if (pc.w > 0.0)
        {
            float2 puv = (pc.xy / pc.w) * float2(0.5, -0.5) + 0.5;
            if (all(puv >= 0.0) && all(puv <= 1.0))
            {
                hist = g_History.SampleLevel(smp_rtlinear, puv, 0);
                float expectPrev = length(wp - g_PrevCamPos.xyz);
                if (abs(hist.r) <= 1.0001 && abs(hist.g - expectPrev) <= g_Params.y * expectPrev + 0.05)
                {
                    histOK = true;
                    motionPx = length((puv - uv) * g_Screen.xy);
                }
            }
        }
    }

    float outShadow = cur;
    float a = 0.0;
    if (noPage)
    {
        a = histOK ? g_Params4.y : 0.0;
        outShadow = lerp(cur, hist.r, a);
    }
    else if (histOK)
    {
        a = g_Params.x;
        float mfade = saturate(motionPx / max(g_Params2.y, 1e-3));
        a = lerp(a, min(a, g_Params2.z), mfade);
        float hClamped = clamp(hist.r, cur - g_Params2.x, cur + g_Params2.x);
        outShadow = lerp(cur, hClamped, a);
    }
    g_Mask[px] = float4(outShadow, dist, a, 0.0);
}
