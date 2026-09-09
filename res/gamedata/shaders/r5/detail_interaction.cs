#include "common_samplers.h"

struct InteractionEntity
{
    float3 pos;
    float radius;
    float3 vel;
    float weight;
};

cbuffer InteractionParams : register(b0)
{
    float2 g_origin_cur;
    float2 g_origin_prev;
    int2 g_prev_shift;
    float g_texel_size;
    uint g_size;
    float4 g_spring;
    float g_contact_rate;
    float g_dt;
    float g_max_velocity;
    uint g_entity_count;
    float g_heightmap_min_x;
    float g_heightmap_min_z;
    float g_heightmap_texel_size;
    uint g_prev_valid;
};

StructuredBuffer<InteractionEntity> g_entities : register(t0);
Texture2D<float4> g_prev : register(t1);
Texture2D<float> g_heightmap : register(t2);
RWTexture2D<float4> g_cur : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (any(dtid.xy >= g_size))
        return;

    float4 state = float4(0.0, 0.0, 0.0, 0.0);
    if (g_prev_valid != 0u)
    {
        int2 pt = int2(dtid.xy) + g_prev_shift;
        if (all(pt >= 0) && all(pt < int(g_size)))
            state = g_prev.Load(int3(pt, 0));
    }
    float2 x = state.xy;
    float2 v = state.zw;

    float2 world = g_origin_cur + (float2(dtid.xy) + 0.5) * g_texel_size;

    uint hmWidth, hmHeight;
    g_heightmap.GetDimensions(hmWidth, hmHeight);
    float2 hmUv = ((world - float2(g_heightmap_min_x, g_heightmap_min_z)) / g_heightmap_texel_size) / float2(hmWidth, hmHeight);
    float terrainY = g_heightmap.SampleLevel(smp_nofilter, hmUv, 0).r;

    float2 target = float2(0.0, 0.0);
    float contact = 0.0;
    for (uint i = 0; i < g_entity_count; ++i)
    {
        InteractionEntity e = g_entities[i];
        float2 d = world - e.pos.xz;
        float dist = length(d);
        if (dist >= e.radius)
            continue;
        float pen = 1.0 - dist / e.radius;
        float heightLimit = saturate(1.0 - abs(e.pos.y - terrainY) * 0.8);
        float s = min(pow(pen, 0.75) * heightLimit * e.weight, 1.0);
        if (s <= contact)
            continue;
        float speed = length(e.vel.xz);
        float2 moveDir = (speed > 1e-3) ? e.vel.xz / speed : float2(0.0, 1.0);
        float2 radial = (dist > 1e-3) ? d / dist : moveDir;
        float moveBias = saturate(speed * 0.5) * 0.6;
        float2 dir = lerp(radial, moveDir, moveBias);
        float dirLen = length(dir);
        dir = (dirLen > 1e-3) ? dir / dirLen : radial;
        contact = s;
        target = dir * s;
    }

    if (contact > 0.0)
    {
        float2 xNew = lerp(x, target, g_contact_rate);
        float2 vNew = (g_dt > 1e-5) ? (xNew - x) / g_dt : float2(0.0, 0.0);
        float vLen = length(vNew);
        if (vLen > g_max_velocity)
            vNew *= g_max_velocity / vLen;
        x = xNew;
        v = vNew;
    }
    else
    {
        float2 xNew = g_spring.x * x + g_spring.y * v;
        float2 vNew = g_spring.z * x + g_spring.w * v;
        x = xNew;
        v = vNew;
    }

    float xLen = length(x);
    if (xLen > 1.25)
        x *= 1.25 / xLen;
    if (xLen < 1e-4 && dot(v, v) < 1e-8)
    {
        x = float2(0.0, 0.0);
        v = float2(0.0, 0.0);
    }

    g_cur[dtid.xy] = float4(x, v);
}
