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
    float4 g_HudScale;
};

Texture2D<float> g_Depth : register(t0);
Texture2D<float> g_Atlas : register(t1);
StructuredBuffer<uint> g_PageTable : register(t2);
Texture2D<float4> g_History : register(t3);
Texture2D<float> g_AtlasDyn : register(t14);
StructuredBuffer<uint> g_DynPageTable : register(t15);
StructuredBuffer<float4> g_SlotPivot : register(t17);
StructuredBuffer<float4> g_SlotSun : register(t18);
RWTexture2D<float4> g_Mask : register(u0);

static const float2 kDimS = float2(float(VSM_ATLAS_W_S), float(VSM_ATLAS_H_S));
static const float2 kDimD = float2(float(VSM_ATLAS_W), float(VSM_ATLAS_H));
static const float kTexel = 1.0 / float(VSM_PAGE_SIZE);
static const float kInset = 0.5 / float(VSM_PAGE_SIZE);
static const float kMaxNormalOffset = 0.25;

bool dynOn()
{
    return g_Params4.w > 0.5;
}

struct VsmFrame
{
    float3 right;
    float3 up;
    float3 fwd;
    float3 pivot;
    float2 origin;
};

VsmFrame vsmSlotFrame(uint slot)
{
    float4 a = g_SlotPivot[slot];
    float4 b = g_SlotSun[slot];
    VsmFrame f;
    f.fwd = b.xyz;
    float3 worldUp = (abs(f.fwd.y) > 0.99) ? float3(0.0, 0.0, 1.0) : float3(0.0, 1.0, 0.0);
    f.up = normalize(worldUp - f.fwd * dot(worldUp, f.fwd));
    f.right = cross(f.up, f.fwd);
    f.pivot = a.xyz;
    f.origin = float2(a.w, b.w);
    return f;
}

float3 vsmFrameLp(VsmFrame f, float3 wp)
{
    float3 d = wp - f.pivot;
    return float3(dot(f.right, d), dot(f.up, d), dot(f.fwd, d));
}

float2 vsmFramePl(VsmFrame f, float2 lxy, float pw)
{
    return clamp((lxy - f.origin) / pw, float2(kInset, kInset), float2(1.0 - kInset, 1.0 - kInset));
}

bool reconWorldAt(int2 p, out float3 wp)
{
    wp = float3(0.0, 0.0, 0.0);
    p = clamp(p, int2(0, 0), int2(g_Screen.xy) - int2(1, 1));
    float zndc = g_Depth.Load(int3(p, 0));
    if (zndc <= 0.0)
        return false;
    float2 uv = (float2(p) + 0.5) * g_Screen.zw;
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - 2.0 * uv.y, zndc, 1.0);
    wp = vsmReconstructPos(g_InvViewProj, clip, g_HudScale);
    return true;
}

float3 planeDelta(float3 wp, bool okP, float3 wP, bool okN, float3 wN)
{
    float3 dP = wP - wp;
    float3 dN = wp - wN;
    if (okP && okN)
        return (dot(dP, dP) < dot(dN, dN)) ? dP : dN;
    if (okP)
        return dP;
    if (okN)
        return dN;
    return float3(0.0, 0.0, 0.0);
}

float vsmDepthS(uint slot, float2 pl)
{
    float2 base = float2(float(slot % uint(VSM_ATLAS_W_S)), float(slot / uint(VSM_ATLAS_W_S)));
    return g_Atlas.SampleLevel(smp_nofilter, (base + pl) / kDimS, 0).r;
}

float vsmDepthD(uint slot, float2 pl)
{
    float2 base = float2(float(slot % uint(VSM_ATLAS_W)), float(slot / uint(VSM_ATLAS_W)));
    return g_AtlasDyn.SampleLevel(smp_nofilter, (base + pl) / kDimD, 0).r;
}

float sampleVSM(float3 wp, float3 nrm, float tanT, out float dynOcc, out float dynHit, out float statLit, out bool noPage)
{
    dynOcc = 0.0;
    dynHit = 0.0;
    statLit = 1.0;
    noPage = false;
    if (g_Params2.w > 0.0 && dot(nrm, nrm) > 0.5)
    {
        float3 lp0 = mul(vsm_view, float4(wp, 1.0)).xyz;
        float2 uv0;
        int2 pg0;
        int L0 = vsmSelect(lp0.xy, vsm_level, uv0, pg0);
        if (L0 >= 0)
            wp += nrm * (2.0 * vsm_level[L0].z * (1.0 / float(VSM_VIRTUAL_RES)));
    }
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

    int idx = vsmPageIndex(L, page);
    uint slotD = dynOn() ? g_DynPageTable[idx] : VSM_UNMAPPED;
    bool resD = slotD < uint(VSM_MAX_PHYS);
    bool hasD = resD;
    bool dbgD = resD && (g_PrevCamPos.w > 0.5);

    float2 pageLocal = luv * float(VSM_PAGES_AXIS) - float2(page);
    VsmFrame fr = vsmSlotFrame(slot);
    float pwL = vsm_level[L].z / float(VSM_PAGES_AXIS);
    float3 lpG = vsmFrameLp(fr, wp);
    float2 pageLocalG = vsmFramePl(fr, lpG.xy, pwL);
    float zHere = 1.0 - (lpG.z - vsm_zparams.x) * vsm_zparams.y;
    float zHereD = 1.0 - (lp.z - vsm_zparams.x) * vsm_zparams.y;
    float bias = vsm_zparams.z;
    if (g_Params2.w > 0.0)
    {
        float texelW = vsm_level[L].z * (1.0 / float(VSM_VIRTUAL_RES));
        float slopeN = 2.2 * texelW * tanT * vsm_zparams.y;
        bias = g_Params2.w + min(slopeN, 0.15 * vsm_zparams.y);
    }
    float biasD = vsm_zparams.w;

    float lit = 0.0;
    float litS = 0.0;
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx)
    {
        float2 tap = float2(float(dx), float(dy)) * kTexel;
        float2 pl = clamp(pageLocal + tap, float2(kInset, kInset), float2(1.0 - kInset, 1.0 - kInset));
        float2 plG = clamp(pageLocalG + tap, float2(kInset, kInset), float2(1.0 - kInset, 1.0 - kInset));
        float occS = vsmDepthS(slot, plG);
        bool shS = occS > zHere + bias;
        bool sh = shS;
        if (hasD || dbgD)
        {
            float d = vsmDepthD(slotD, pl);
            if (hasD && d > zHereD + biasD)
            {
                sh = true;
                dynHit += 1.0 / 9.0;
            }
            if (dbgD)
            {
                if (g_PrevCamPos.w > 2.5)
                {
                    if (d > 0.001)
                        dynOcc += 1.0 / 9.0;
                }
                else if ((g_PrevCamPos.w > 1.5 || !shS) && (d > zHereD + biasD))
                    dynOcc += 1.0 / 9.0;
            }
        }
        lit += sh ? 0.0 : 1.0;
        litS += shS ? 0.0 : 1.0;
    }
    statLit = litS * (1.0 / 9.0);
    return lit * (1.0 / 9.0);
}

float2 vsmVogel(int i, int n, float phi)
{
    float r = sqrt((float(i) + 0.5) / float(n));
    float a = float(i) * 2.39996323 + phi;
    return r * float2(cos(a), sin(a));
}

float vsmIGN(float2 px, float phase)
{
    return frac(52.9829189 * frac(dot(px + 5.588238 * phase, float2(0.06711056, 0.00583715))));
}

bool vsmPageAt(float2 lxy, int L, out float2 pl, out uint slotS, out uint slotD)
{
    pl = float2(0.0, 0.0);
    slotS = VSM_UNMAPPED;
    slotD = VSM_UNMAPPED;
    float2 t = (lxy - vsm_level[L].xy) / vsm_level[L].z;
    if (any(t < 0.0) || any(t >= 1.0))
        return false;
    int2 pg = int2(floor(t * float(VSM_PAGES_AXIS)));
    int idx = vsmPageIndex(L, pg);
    slotS = g_PageTable[idx];
    slotD = dynOn() ? g_DynPageTable[idx] : VSM_UNMAPPED;
    pl = clamp(t * float(VSM_PAGES_AXIS) - float2(pg), float2(kInset, kInset), float2(1.0 - kInset, 1.0 - kInset));
    return slotS < uint(VSM_MAX_PHYS_S);
}

float sampleVSMSoft(float3 wp, float3 nrm, float tanT, float rot,
                    out float dynOcc, out float dynHit, out float statLit, out bool noPage)
{
    dynOcc = 0.0;
    dynHit = 0.0;
    statLit = 1.0;
    noPage = false;
    bool haveN = dot(nrm, nrm) > 0.5;

    float3 lp = mul(vsm_view, float4(wp, 1.0)).xyz;
    float2 uv0;
    int2 pg0;
    int L = vsmSelect(lp.xy, vsm_level, uv0, pg0);
    if (L < 0)
        return 1.0;
    for (; L < VSM_LEVELS; ++L)
    {
        float2 t = (lp.xy - vsm_level[L].xy) / vsm_level[L].z;
        if (any(t < 0.0) || any(t >= 1.0))
            continue;
        int2 pg = int2(floor(t * float(VSM_PAGES_AXIS)));
        if (g_PageTable[vsmPageIndex(L, pg)] < uint(VSM_MAX_PHYS_S))
            break;
    }
    if (L >= VSM_LEVELS)
    {
        noPage = true;
        return 1.0;
    }

    float texelW = vsm_level[L].z / float(VSM_VIRTUAL_RES);
    float zScale = vsm_zparams.y;
    float tanS = g_Params3.z;
    float rMax = max(tanS * g_Params3.w, 2.0 * texelW);

    float2 plC;
    uint slotSC;
    uint slotDC;
    vsmPageAt(lp.xy, L, plC, slotSC, slotDC);
    bool resD = slotDC < uint(VSM_MAX_PHYS);
    bool useD = resD;
    bool dbgD = resD && (g_PrevCamPos.w > 0.5);

    VsmFrame frC = vsmSlotFrame(slotSC);
    float pwL = vsm_level[L].z / float(VSM_PAGES_AXIS);
    float3 rightNow = vsm_view[0].xyz;
    float3 upNow = vsm_view[1].xyz;
    float3 wpS = haveN ? wp + nrm * (2.0 * texelW) : wp;
    float3 lpS = mul(vsm_view, float4(wpS, 1.0)).xyz;
    float3 lpSG = vsmFrameLp(frC, wpS);
    float zS = 1.0 - (lpSG.z - vsm_zparams.x) * zScale;
    float zSD = 1.0 - (lpS.z - vsm_zparams.x) * zScale;
    float biasC = (g_Params2.w > 0.0) ? g_Params2.w : vsm_zparams.z;
    float biasD = vsm_zparams.w;

    int ns = max(int(g_Params3.y), 1);
    float sumD = 0.0;
    float cntD = 0.0;
    for (int s = 0; s < ns; ++s)
    {
        float2 off = (s == 0) ? float2(0.0, 0.0) : vsmVogel(s, ns, rot * 6.2831853) * rMax;
        float2 pl;
        uint sS;
        uint sD;
        if (!vsmPageAt(lpS.xy + off, L, pl, sS, sD))
            continue;
        float r = length(off);
        float slope = min(r * tanT * zScale, 0.15 * zScale);
        float3 wpT = wpS + rightNow * off.x + upNow * off.y;
        VsmFrame frT = (sS == slotSC) ? frC : vsmSlotFrame(sS);
        float3 lpT = vsmFrameLp(frT, wpT);
        float2 plG = vsmFramePl(frT, lpT.xy, pwL);
        float zT = 1.0 - (lpT.z - vsm_zparams.x) * zScale;
        float dBest = -1.0;
        float dS = vsmDepthS(sS, plG);
        if (dS > zT + (biasC + slope))
            dBest = dS;
        if (useD && sD < uint(VSM_MAX_PHYS))
        {
            float dD = vsmDepthD(sD, pl);
            if (dD > zSD + biasD)
                dBest = max(dBest, dD);
        }
        if (dBest > -0.5)
        {
            sumD += dBest;
            cntD += 1.0;
        }
    }

    if (cntD < 0.5)
        return 1.0;
    float distB = max(sumD / cntD - zS, 0.0) / max(zScale, 1e-9);
    float w = clamp(tanS * distB, 0.75 * texelW, rMax);

    float3 wpF = haveN ? wp + nrm * min(max(w, 2.0 * texelW), kMaxNormalOffset) : wp;
    float3 lpF = mul(vsm_view, float4(wpF, 1.0)).xyz;
    float zFD = 1.0 - (lpF.z - vsm_zparams.x) * zScale;

    int nf = max(int(g_Params3.x), 1);
    float lit = 0.0;
    float litS = 0.0;
    float taken = 0.0;
    float hitD = 0.0;
    float occDbg = 0.0;
    float rotF = rot * 6.2831853 + 1.61803399;
    for (int f = 0; f < nf; ++f)
    {
        float2 off = vsmVogel(f, nf, rotF) * w;
        float2 pl;
        uint sS;
        uint sD;
        if (!vsmPageAt(lpF.xy + off, L, pl, sS, sD))
            continue;
        taken += 1.0;
        float r = length(off);
        float slope = min(r * tanT * zScale, 0.15 * zScale);
        float bias = biasC + slope;
        float3 wpT = wpF + rightNow * off.x + upNow * off.y;
        VsmFrame frT = (sS == slotSC) ? frC : vsmSlotFrame(sS);
        float3 lpT = vsmFrameLp(frT, wpT);
        float2 plG = vsmFramePl(frT, lpT.xy, pwL);
        float zT = 1.0 - (lpT.z - vsm_zparams.x) * zScale;
        float occS = vsmDepthS(sS, plG);
        bool shS = occS > zT + bias;
        bool sh = shS;
        if ((useD || dbgD) && sD < uint(VSM_MAX_PHYS))
        {
            float d = vsmDepthD(sD, pl);
            if (useD && d > zFD + biasD)
            {
                sh = true;
                hitD += 1.0;
            }
            if (dbgD)
            {
                if (g_PrevCamPos.w > 2.5)
                {
                    if (d > 0.001)
                        occDbg += 1.0;
                }
                else if ((g_PrevCamPos.w > 1.5 || !shS) && (d > zFD + biasD))
                    occDbg += 1.0;
            }
        }
        lit += sh ? 0.0 : 1.0;
        litS += shS ? 0.0 : 1.0;
    }
    if (taken < 0.5)
    {
        noPage = true;
        return 1.0;
    }
    float inv = 1.0 / taken;
    dynHit = hitD * inv;
    dynOcc = occDbg * inv;
    statLit = litS * inv;
    return lit * inv;
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
    float3 wp = vsmReconstructPos(g_InvViewProj, clip, g_HudScale);

    float tanT = 0.0;
    float3 nrm = float3(0.0, 0.0, 0.0);
    bool softOn = g_Params3.x >= 1.0;
    float3 sunTravel = normalize(vsm_view[2].xyz);
    if (g_Params2.w > 0.0 || softOn)
    {
        float3 wxp, wxn, wyp, wyn;
        bool okxp = reconWorldAt(px + int2(1, 0), wxp);
        bool okxn = reconWorldAt(px - int2(1, 0), wxn);
        bool okyp = reconWorldAt(px + int2(0, 1), wyp);
        bool okyn = reconWorldAt(px - int2(0, 1), wyn);
        float3 dx = planeDelta(wp, okxp, wxp, okxn, wxn);
        float3 dy = planeDelta(wp, okyp, wyp, okyn, wyn);
        float3 n = cross(dx, dy);
        float nl = length(n);
        if (nl > 1e-8)
        {
            nrm = n / nl;
            if (dot(nrm, g_CurCamPos.xyz - wp) < 0.0)
                nrm = -nrm;
            float c = abs(dot(nrm, sunTravel));
            tanT = min(sqrt(max(1.0 - c * c, 0.0)) / max(c, 0.05), 12.0);
        }
        else
        {
            tanT = 12.0;
        }
    }

    float dynOcc;
    float dynHit;
    float statLit;
    bool noPage;
    float rot = vsmIGN(float2(px), g_Params4.x);
    float cur = softOn ? sampleVSMSoft(wp, nrm, tanT, rot, dynOcc, dynHit, statLit, noPage)
                       : sampleVSM(wp, nrm, tanT, dynOcc, dynHit, statLit, noPage);

    if (dynOcc > 0.0 && g_PrevCamPos.w < 1.5 && dot(nrm, nrm) > 0.5)
    {
        if (dot(nrm, -sunTravel) < 0.05)
            dynOcc = 0.0;
    }
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
    float status = 0.0;
    if (noPage)
    {
        a = histOK ? g_Params4.y : 0.0;
        outShadow = lerp(cur, hist.r, a);
        status = histOK ? 1.0 : 2.0;
    }
    else if (histOK)
    {
        a = (dynHit > 0.0 || hist.a > 0.0) ? min(g_Params.x, g_CurCamPos.w) : g_Params.x;
        float mfade = saturate(motionPx / max(g_Params2.y, 1e-3));
        a = lerp(a, min(a, g_Params2.z), mfade);
        float hClamped = clamp(hist.r, cur - g_Params2.x, cur + g_Params2.x);
        outShadow = lerp(cur, hClamped, a);
    }
    float sel = g_Params4.z;
    float bOut = (sel > 1.5) ? dynOcc : ((sel > 0.5) ? status : a);
    g_Mask[px] = float4(outShadow, dist, bOut, dynHit);
}
