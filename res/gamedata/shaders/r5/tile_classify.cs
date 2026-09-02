#define SM_6_0
#define CLUSTERED_LIGHTING_FORWARD
#define SUN_SHADOW_RECEIVER
#include "common.h"
#include "bindless_common.h"
#include "deferred_tiles.h"

Texture2D<float> g_GBufferDepth : register(t30);
Texture2D<float4> g_GBufferNormal : register(t31);
RWStructuredBuffer<uint> g_TileLists : register(u0);
RWByteAddressBuffer g_TileArgs : register(u1);

groupshared uint s_flags;

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 gid : SV_GroupID, uint3 gtid : SV_GroupThreadID, uint gi : SV_GroupIndex)
{
    if (gi == 0)
        s_flags = 0;
    GroupMemoryBarrierWithGroupSync();

    uint2 p = gid.xy * TILE_SIZE + gtid.xy;
    uint width, height;
    g_GBufferDepth.GetDimensions(width, height);
    if (p.x < width && p.y < height)
    {
        float4 n = g_GBufferNormal[p];
        float depth = g_GBufferDepth[p];
        if (dot(n.xyz, n.xyz) >= 0.25)
        {
            uint bits = TILE_BIT_GEOMETRY;
            if (forceMixed != 0 || (cascade_splits.x > 0.5 && cascade_splits.x < 1.5))
                bits |= TILE_BIT_SUN_MIXED;
            else if (cascade_splits.x > 1.5 && g_SunShadowMask.Load(int3(int2(p), 0)).r < 1.0)
                bits |= TILE_BIT_SUN_MIXED;
            if (cluster_params.w > 0.0)
            {
                float2 pixel = float2(p) + 0.5;
                float3 worldPos = reconstruct_world_pos(pixel, depth);
                float linearDepth = mul(m_V, float4(worldPos, 1.0)).z;
                uint clusterIdx = GetClusterIndex(pixel, linearDepth, cluster_params.xyz, cluster_scales);
                if (g_ClusterGrid[clusterIdx].y > 0)
                    bits |= TILE_BIT_LIGHTS;
            }
            InterlockedOr(s_flags, bits);
        }
    }
    GroupMemoryBarrierWithGroupSync();

    if (gi == 0 && (s_flags & TILE_BIT_GEOMETRY) != 0)
    {
        uint cls = ((s_flags & TILE_BIT_SUN_MIXED) != 0 ? 1u : 0u) | ((s_flags & TILE_BIT_LIGHTS) != 0 ? 2u : 0u);
        uint slot;
        g_TileArgs.InterlockedAdd(cls * 12u, 1u, slot);
        g_TileLists[cls * maxTiles + slot] = gid.y * tilesX + gid.x;
    }
}
