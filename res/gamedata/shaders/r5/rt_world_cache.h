#ifndef RT_WORLD_CACHE_H
#define RT_WORLD_CACHE_H

#include "rt_common.h"

#define RT_WORLD_CACHE_EMPTY 0u
#define RT_WORLD_CACHE_SEARCH_STEPS 8u
#define RT_WORLD_CACHE_FLAG_ENABLED 1u
#define RT_WORLD_CACHE_FLAG_JITTER 2u
#define RT_WORLD_CACHE_DEBUG_SHIFT 4u
#define RT_WORLD_CACHE_DEBUG_MASK 0xF0u
#define RT_WORLD_CACHE_NO_CELL 0xFFFFFFFFu
#define RT_WORLD_CACHE_LOOKUP_ABSENT 0u
#define RT_WORLD_CACHE_LOOKUP_ALLOCATED 1u
#define RT_WORLD_CACHE_LOOKUP_UNSAMPLED 2u
#define RT_WORLD_CACHE_LOOKUP_VALID 3u
#define RT_WORLD_CACHE_MIN_VALID_SAMPLES 4.0
#define RT_WORLD_CACHE_MAX_LOBE_PDF 0.31830988
#define RT_WORLD_CACHE_EVENT_SUBSTITUTED 0u
#define RT_WORLD_CACHE_EVENT_UNSAMPLED 1u
#define RT_WORLD_CACHE_EVENT_ABSENT 2u
#define RT_WORLD_CACHE_EVENT_BYPASSED 3u
#define RT_WORLD_CACHE_EVENT_COUNT 4u
#define RT_WORLD_CACHE_STAT_LIVE 0u
#define RT_WORLD_CACHE_STAT_EVENTS 1u
#define RT_WORLD_CACHE_STAT_UPDATES 5u
#define RT_WORLD_CACHE_MIN_HISTORY 8.0
#define RT_WORLD_CACHE_UPDATE_GROUP_SIZE 64
#define RT_WORLD_CACHE_SPECULAR_ROUGHNESS_MIN 0.25
#define RT_WORLD_CACHE_SPECULAR_ROUGHNESS_MAX 0.5

cbuffer RTWorldCacheParams : register(b6)
{
    float4 g_WorldCacheCamera;
    float g_WorldCacheCellSize;
    float g_WorldCacheLodScale;
    uint g_WorldCacheCapacityMask;
    uint g_WorldCacheLifetime;
    uint g_WorldCacheFrame;
    uint g_WorldCacheUpdateTarget;
    uint g_WorldCacheMaxSamples;
    uint g_WorldCacheFlags;
    uint g_WorldCacheBounce;
    uint g_WorldCacheEpoch;
    uint g_WorldCachePad1;
    uint g_WorldCachePad2;
};

RWStructuredBuffer<uint> u_WorldCacheChecksum : register(u8);
RWStructuredBuffer<uint> u_WorldCacheLife : register(u9);
#if defined(RT_WORLD_CACHE_MAINTENANCE) || defined(RT_WORLD_CACHE_UPDATE)
RWStructuredBuffer<float4> u_WorldCacheRadiance : register(u10);
#endif
#ifndef RT_WORLD_CACHE_MAINTENANCE
StructuredBuffer<float4> t_WorldCacheRadianceInput : register(t19);
#endif
RWStructuredBuffer<float4> u_WorldCachePosition : register(u11);
RWStructuredBuffer<float4> u_WorldCacheNormal : register(u12);
RWStructuredBuffer<uint> u_WorldCacheStats : register(u13);
#ifdef RT_WORLD_CACHE_SELECT
RWStructuredBuffer<uint> u_WorldCacheUpdateList : register(u14);
RWStructuredBuffer<uint> u_WorldCacheUpdateArgs : register(u15);
#endif
#ifdef RT_WORLD_CACHE_UPDATE
StructuredBuffer<uint> t_WorldCacheUpdateList : register(t31);
#endif

struct RTWorldCacheLookup
{
    float3 radiance;
    float sampleCount;
    uint cell;
    uint state;
};

RTWorldCacheLookup RTWorldCacheEmptyLookup()
{
    RTWorldCacheLookup lookup;
    lookup.radiance = 0.0;
    lookup.sampleCount = 0.0;
    lookup.cell = RT_WORLD_CACHE_NO_CELL;
    lookup.state = RT_WORLD_CACHE_LOOKUP_ABSENT;
    return lookup;
}

bool RTWorldCacheEnabled()
{
    return (g_WorldCacheFlags & RT_WORLD_CACHE_FLAG_ENABLED) != 0u;
}

uint RTWorldCacheDebugMode()
{
    return (g_WorldCacheFlags & RT_WORLD_CACHE_DEBUG_MASK) >> RT_WORLD_CACHE_DEBUG_SHIFT;
}

uint RTWorldCacheMixHash(uint input)
{
    uint n = (input << 13u) ^ input;
    return n * (n * n * 15731u + 789221u) + 1376312589u;
}

float RTWorldCacheLod(float3 position, bool stochastic, inout uint rng)
{
    float distance = length(position - g_WorldCacheCamera.xyz) / max(g_WorldCacheLodScale, 0.01);
    float lod = log2(1.0 + distance);
    float lodFloor = floor(lod);
    if (stochastic)
    {
        float fraction = lod - lodFloor;
        if (rand_float(rng) < fraction * fraction * fraction)
            lodFloor += 1.0;
    }
    return lodFloor;
}

float RTWorldCacheCellSize(float lod)
{
    return g_WorldCacheCellSize * exp2(lod);
}

int3 RTWorldCacheQuantize(float3 position, float cellSize)
{
    return int3(floor(position / cellSize + 0.0001));
}

uint RTWorldCacheOctant(float3 normal)
{
    return (normal.x < 0.0 ? 1u : 0u) | (normal.y < 0.0 ? 2u : 0u) | (normal.z < 0.0 ? 4u : 0u);
}

uint RTWorldCacheKey(int3 cell, uint octant, uint lod)
{
    uint key = pcg_hash(asuint(cell.x));
    key = pcg_hash(key + asuint(cell.y));
    key = pcg_hash(key + asuint(cell.z));
    key = pcg_hash(key + octant);
    key = pcg_hash(key + lod);
    key = pcg_hash(key + g_WorldCacheEpoch);
    return key & g_WorldCacheCapacityMask;
}

uint RTWorldCacheChecksum(int3 cell, uint octant, uint lod)
{
    uint sum = RTWorldCacheMixHash(asuint(cell.x));
    sum = RTWorldCacheMixHash(sum + asuint(cell.y));
    sum = RTWorldCacheMixHash(sum + asuint(cell.z));
    sum = RTWorldCacheMixHash(sum + octant);
    sum = RTWorldCacheMixHash(sum + lod);
    sum = RTWorldCacheMixHash(sum + g_WorldCacheEpoch);
    return max(sum, 1u);
}

#ifndef RT_WORLD_CACHE_MAINTENANCE
RTWorldCacheLookup RTWorldCacheQuery(float3 position, float3 normal, uint life, bool insert, bool stochastic,
    inout uint rng)
{
    RTWorldCacheLookup lookup = RTWorldCacheEmptyLookup();
    if (!RTWorldCacheEnabled() || !all(isfinite(position)) || !all(isfinite(normal)))
        return lookup;

    float lod = RTWorldCacheLod(position, stochastic, rng);
    float cellSize = RTWorldCacheCellSize(lod);
    float3 samplePosition = position;
    if (stochastic && (g_WorldCacheFlags & RT_WORLD_CACHE_FLAG_JITTER) != 0u)
    {
        float3 axis = abs(normal.y) < 0.99 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
        float3 tangent = RTSafeNormalize(cross(normal, axis), float3(1.0, 0.0, 0.0));
        float3 bitangent = cross(normal, tangent);
        float2 offset = (float2(rand_float(rng), rand_float(rng)) * 2.0 - 1.0) * cellSize * 0.5;
        samplePosition += tangent * offset.x + bitangent * offset.y;
    }

    int3 cell = RTWorldCacheQuantize(samplePosition, cellSize);
    uint octant = RTWorldCacheOctant(normal);
    uint lodIndex = uint(lod);
    uint key = RTWorldCacheKey(cell, octant, lodIndex);
    uint checksum = RTWorldCacheChecksum(cell, octant, lodIndex);

    uint firstEmptyProbe = RT_WORLD_CACHE_SEARCH_STEPS;
    for (uint probe = 0u; probe < RT_WORLD_CACHE_SEARCH_STEPS; ++probe)
    {
        uint slot = (key + probe) & g_WorldCacheCapacityMask;
        uint existing = u_WorldCacheChecksum[slot];
        if (existing == checksum)
        {
            if (life != 0u)
                InterlockedMax(u_WorldCacheLife[slot], life);
            float4 radiance = t_WorldCacheRadianceInput[slot];
            bool sampled = all(isfinite(radiance)) && radiance.a >= RT_WORLD_CACHE_MIN_VALID_SAMPLES;
            lookup.radiance = sampled ? max(radiance.rgb, 0.0) : 0.0;
            lookup.sampleCount = isfinite(radiance.a) ? max(radiance.a, 0.0) : 0.0;
            lookup.cell = slot;
            lookup.state = sampled ? RT_WORLD_CACHE_LOOKUP_VALID : RT_WORLD_CACHE_LOOKUP_UNSAMPLED;
            return lookup;
        }
        if (existing == RT_WORLD_CACHE_EMPTY && firstEmptyProbe == RT_WORLD_CACHE_SEARCH_STEPS)
            firstEmptyProbe = probe;
    }
    if (!insert)
        return lookup;

    for (uint claim = firstEmptyProbe; claim < RT_WORLD_CACHE_SEARCH_STEPS; ++claim)
    {
        uint slot = (key + claim) & g_WorldCacheCapacityMask;
        uint existing;
        InterlockedCompareExchange(u_WorldCacheChecksum[slot], RT_WORLD_CACHE_EMPTY, checksum, existing);
        if (existing == RT_WORLD_CACHE_EMPTY)
        {
            u_WorldCachePosition[slot] = float4(position, 0.0);
            u_WorldCacheNormal[slot] = float4(normal, 0.0);
            InterlockedMax(u_WorldCacheLife[slot], g_WorldCacheLifetime);
            lookup.cell = slot;
            lookup.state = RT_WORLD_CACHE_LOOKUP_ALLOCATED;
            return lookup;
        }
        if (existing == checksum)
        {
            if (life != 0u)
                InterlockedMax(u_WorldCacheLife[slot], life);
            lookup.cell = slot;
            lookup.state = RT_WORLD_CACHE_LOOKUP_UNSAMPLED;
            return lookup;
        }
    }
    return lookup;
}

float RTWorldCacheRoughnessEligibility(float roughness)
{
    return saturate((saturate(roughness) - RT_WORLD_CACHE_SPECULAR_ROUGHNESS_MIN) /
        (RT_WORLD_CACHE_SPECULAR_ROUGHNESS_MAX - RT_WORLD_CACHE_SPECULAR_ROUGHNESS_MIN));
}

float3 RTWorldCacheDebugColor(RTWorldCacheLookup lookup, uint mode, float3 diffuseAlbedo, float eligibility)
{
    if (mode == 1u)
        return lookup.state == RT_WORLD_CACHE_LOOKUP_VALID ? diffuseAlbedo * lookup.radiance : 0.0;
    if (mode == 2u)
    {
        if (lookup.cell == RT_WORLD_CACHE_NO_CELL)
            return float3(0.5, 0.0, 0.5);
        uint hash = pcg_hash(lookup.cell);
        float3 color = float3(float(hash & 255u), float((hash >> 8u) & 255u), float((hash >> 16u) & 255u)) / 255.0;
        return color * 0.8 + 0.2;
    }
    if (mode == 3u)
    {
        if (lookup.cell == RT_WORLD_CACHE_NO_CELL)
            return float3(0.5, 0.0, 0.0);
        float fill = saturate(lookup.sampleCount / max(float(g_WorldCacheMaxSamples), 1.0));
        return float3(fill, fill, fill);
    }
    if (mode == 4u)
    {
        float3 color = float3(0.9, 0.1, 0.1);
        if (lookup.state == RT_WORLD_CACHE_LOOKUP_VALID)
            color = float3(0.1, 0.8, 0.1);
        else if (lookup.state != RT_WORLD_CACHE_LOOKUP_ABSENT)
            color = float3(0.9, 0.8, 0.1);
        return lerp(float3(0.1, 0.2, 0.9), color, saturate(eligibility));
    }
    return 0.0;
}
#endif

#endif
