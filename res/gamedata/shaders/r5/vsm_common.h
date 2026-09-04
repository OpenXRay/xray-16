#ifndef VSM_COMMON_H
#define VSM_COMMON_H

#define VSM_LEVELS        6
#define VSM_VIRTUAL_RES   4096
#define VSM_PAGE_SIZE     128
#define VSM_PAGES_AXIS    32
#define VSM_PAGES_PER_LVL 1024
#define VSM_PAGE_COUNT    6144
#define VSM_MAX_PHYS      2048
#define VSM_ATLAS_W       64
#define VSM_ATLAS_H       32
#define VSM_PAGES_CAP     1024
#define VSM_UNMAPPED      0xFFFFFFFFu
#define VSM_MAX_PHYS_S    6144
#define VSM_ATLAS_W_S     64
#define VSM_ATLAS_H_S     96
#define VSM_SKIN_CAP      256
#define VSM_BUCKET_AXIS   48
#define VSM_BUCKET_MARGIN 8
#define VSM_BUCKETS_PER_LVL 2304

int vsmPageIndex(int level, int2 page)
{
    return level * VSM_PAGES_PER_LVL + page.y * VSM_PAGES_AXIS + page.x;
}

int vsmToroidalSlot(int level, int2 absPage)
{
    int sx = absPage.x & (VSM_PAGES_AXIS - 1);
    int sy = absPage.y & (VSM_PAGES_AXIS - 1);
    return level * VSM_PAGES_PER_LVL + sy * VSM_PAGES_AXIS + sx;
}

int vsmSelect(float2 lxy, float4 level[VSM_LEVELS], out float2 uv, out int2 page)
{
    for (int L = 0; L < VSM_LEVELS; ++L)
    {
        float2 t = (lxy - level[L].xy) / level[L].z;
        if (all(t >= 0.0) && all(t < 1.0))
        {
            uv = t;
            page = int2(floor(t * float(VSM_PAGES_AXIS)));
            return L;
        }
    }
    uv = float2(0.0, 0.0);
    page = int2(0, 0);
    return -1;
}

#define VSM_PAGE_RANGE(L, lp, R, lo, hi, p0, p1)                                              \
    float2 origin##L = vsm_level[L].xy;                                                       \
    float  pw##L     = vsm_level[L].z / float(VSM_PAGES_AXIS);                                \
    float2 lo = (lp - float2(R, R) - origin##L) / pw##L;                                      \
    float2 hi = (lp + float2(R, R) - origin##L) / pw##L;                                      \
    if (hi.x < 0.0 || hi.y < 0.0 || lo.x >= float(VSM_PAGES_AXIS) || lo.y >= float(VSM_PAGES_AXIS)) \
        continue;                                                                             \
    int2 p0 = clamp(int2(floor(lo)), int2(0, 0), int2(VSM_PAGES_AXIS - 1, VSM_PAGES_AXIS - 1)); \
    int2 p1 = clamp(int2(floor(hi)), int2(0, 0), int2(VSM_PAGES_AXIS - 1, VSM_PAGES_AXIS - 1))

float3 vsmReconstructPos(float4x4 invViewProj, float4 clip, float4 hudScale)
{
    bool hud = clip.z >= 0.9;
    if (hud)
        clip.z = (clip.z - 0.9) * 10.0;
    float4 world = mul(invViewProj, clip);
    float3 pos = world.xyz / world.w;
    if (hud)
        pos = hudScale.xyz + hudScale.w * (pos - hudScale.xyz);
    return pos;
}

#endif
