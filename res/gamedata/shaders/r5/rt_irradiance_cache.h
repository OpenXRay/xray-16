#ifndef RT_IRRADIANCE_CACHE_H
#define RT_IRRADIANCE_CACHE_H

struct IrradianceCacheEntry
{
    float3 irradiance;
    uint stamp;
};

uint IrradianceCacheHash(float3 pos, float cellSize, uint cacheSize)
{
    int3 cell = int3(floor(pos / max(cellSize, 0.05)));
    uint h = (uint)cell.x * 73856093u ^ (uint)cell.y * 19349663u ^ (uint)cell.z * 83492791u;
    return h % max(cacheSize, 1u);
}

float3 QueryIrradianceCacheRW(
    RWStructuredBuffer<IrradianceCacheEntry> cache,
    float3 pos,
    float cellSize,
    uint cacheSize,
    uint frameIndex,
    uint maxAge)
{
    if (cacheSize == 0)
        return 0;
    uint idx = IrradianceCacheHash(pos, cellSize, cacheSize);
    IrradianceCacheEntry e = cache[idx];
    if (e.stamp == 0)
        return 0;
    uint age = frameIndex - e.stamp;
    if (age > maxAge)
        return 0;
    float fade = 1.0 - saturate((float)age / (float)max(maxAge, 1u));
    return e.irradiance * fade;
}

void UpdateIrradianceCache(
    RWStructuredBuffer<IrradianceCacheEntry> cache,
    float3 pos,
    float3 irradiance,
    float cellSize,
    uint cacheSize,
    uint frameIndex)
{
    if (cacheSize == 0 || !any(irradiance > 0))
        return;
    uint idx = IrradianceCacheHash(pos, cellSize, cacheSize);
    IrradianceCacheEntry prev = cache[idx];
    float3 blended = irradiance;
    if (prev.stamp != 0 && (frameIndex - prev.stamp) < 64u)
        blended = lerp(prev.irradiance, irradiance, 0.35);
    IrradianceCacheEntry e;
    e.irradiance = blended;
    e.stamp = max(frameIndex, 1u);
    cache[idx] = e;
}

#endif
