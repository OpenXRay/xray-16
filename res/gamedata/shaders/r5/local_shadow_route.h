#ifndef LOCAL_SHADOW_ROUTE_H
#define LOCAL_SHADOW_ROUTE_H

struct LocalRoute
{
    float4 position;
    float4 clip;
};

LocalRoute LocalRouteRect(float4x4 viewProj, float3 rect, float atlas, float3 worldPos)
{
    float4 c = mul(viewProj, float4(worldPos, 1.0));
    float s = rect.z / atlas;
    float2 ctr = (rect.xy + 0.5 * rect.z) / atlas * 2.0 - 1.0;
    LocalRoute r;
    r.position = float4(ctr.x * c.w + c.x * s, -(ctr.y * c.w + c.y * s), c.z, c.w);
    r.clip = float4(c.w + c.x, c.w - c.x, c.w + c.y, c.w - c.y);
    return r;
}

LocalRoute LocalRouteTile(uint slot, float3 worldPos)
{
    LocalShadowView t = g_LocalShadowTiles[slot];
    return LocalRouteRect(t.viewProj, t.rect.xyz, LOCAL_SHADOW_ATLAS, worldPos);
}

LocalRoute LocalRouteHud(uint slot, float3 worldPos)
{
    LocalShadowView t = g_LocalShadowTiles[slot];
    return LocalRouteRect(t.hudViewProj, t.hud.yzw, LOCAL_SHADOW_HUD_ATLAS, worldPos);
}

#endif
