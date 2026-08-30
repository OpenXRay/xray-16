#ifndef VSM_PAGE_ROUTE_H
#define VSM_PAGE_ROUTE_H

struct VsmRoute
{
    float4 position;
    float4 clip;
};

VsmRoute VsmRoutePage(uint slot, float3 worldPos)
{
    uint4 pg = g_PageList[slot];
    int L = int(pg.x);
    int2 page = int2(pg.yz);
    float3 lp = mul(vsm_view, float4(worldPos, 1.0)).xyz;
    float2 origin = vsm_level[L].xy;
    float pw = vsm_level[L].z / float(VSM_PAGES_AXIS);
    float2 pmin = origin + float2(page) * pw;
    float2 pmax = pmin + float2(pw, pw);
    float2 nxy = (lp.xy - pmin) / pw * 2.0 - 1.0;
    float nz = 1.0 - (lp.z - vsm_zparams.x) * vsm_zparams.y;
    uint ax = slot % uint(VSM_ATLAS_W_S);
    uint ay = slot / uint(VSM_ATLAS_W_S);
    float hX = 1.0 / float(VSM_ATLAS_W_S);
    float hY = 1.0 / float(VSM_ATLAS_H_S);
    float cx = (float(ax) + 0.5) * 2.0 * hX - 1.0;
    float cy = (float(ay) + 0.5) * 2.0 * hY - 1.0;
    VsmRoute r;
    r.position = float4(cx + nxy.x * hX, -(cy + nxy.y * hY), nz, 1.0);
    r.clip = float4(lp.x - pmin.x, pmax.x - lp.x, lp.y - pmin.y, pmax.y - lp.y);
    return r;
}

#endif
