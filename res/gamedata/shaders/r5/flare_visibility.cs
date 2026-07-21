cbuffer FlareVisParams : register(b0)
{
    float2 sun_pos_px;
    float radius_px;
    float ema_alpha;
    uint valid;
    uint3 pad;
};

Texture2D<float> g_Depth : register(t0);
RWStructuredBuffer<float> g_Vis : register(u0);

groupshared uint gs_visible;

[numthreads(64, 1, 1)]
void main(uint3 tid : SV_GroupThreadID)
{
    if (tid.x == 0)
        gs_visible = 0;
    GroupMemoryBarrierWithGroupSync();

    float dim_x, dim_y;
    g_Depth.GetDimensions(dim_x, dim_y);

    float r = sqrt((float(tid.x) + 0.5) / 64.0) * radius_px;
    float a = float(tid.x) * 2.39996323;
    float2 p = sun_pos_px + r * float2(cos(a), sin(a));

    uint vis = 1;
    if (p.x >= 0.0 && p.y >= 0.0 && p.x < dim_x && p.y < dim_y)
        vis = (g_Depth.Load(int3(int2(p), 0)) <= 0.0) ? 1 : 0;

    InterlockedAdd(gs_visible, vis);
    GroupMemoryBarrierWithGroupSync();

    if (tid.x == 0)
    {
        float raw = (valid != 0) ? (float(gs_visible) / 64.0) : 0.0;
        g_Vis[0] = lerp(g_Vis[0], raw, ema_alpha);
    }
}
