#ifndef LOCAL_SHADOW_ROUTE_H
#define LOCAL_SHADOW_ROUTE_H

struct LocalRoute
{
    float4 position;
    float4 clip;
};

LocalRoute LocalRouteTile(uint slot, float3 worldPos)
{
    LocalShadowTile t = g_LocalShadowTiles[slot];
    float4 c = mul(t.viewProj, float4(worldPos, 1.0));
    float s = t.rect.z / LOCAL_SHADOW_ATLAS;
    float2 ctr = (t.rect.xy + 0.5 * t.rect.z) / LOCAL_SHADOW_ATLAS * 2.0 - 1.0;
    LocalRoute r;
    r.position = float4(ctr.x * c.w + c.x * s, -(ctr.y * c.w + c.y * s), c.z, c.w);
    r.clip = float4(c.w + c.x, c.w - c.x, c.w + c.y, c.w - c.y);
    return r;
}

#endif
