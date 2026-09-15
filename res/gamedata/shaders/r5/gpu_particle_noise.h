#ifndef GPU_PARTICLE_NOISE_H
#define GPU_PARTICLE_NOISE_H

uint GpuPapiPermutation(uint index)
{
    return asuint(g_PapiNoise[index].w);
}

float GpuPapiNoise(float3 position, bool floorCoordinates)
{
    float3 translated = position + 10000.0;
    int3 integral = int3(floorCoordinates ? floor(translated) : trunc(translated));
    uint3 lower = uint3(integral) & 255u;
    uint3 upper = (lower + 1u) & 255u;
    float3 r0 = translated - float3(integral);
    float3 r1 = r0 - 1.0;
    uint i = GpuPapiPermutation(lower.x);
    uint j = GpuPapiPermutation(upper.x);
    uint b00 = GpuPapiPermutation(i + lower.y);
    uint b10 = GpuPapiPermutation(j + lower.y);
    uint b01 = GpuPapiPermutation(i + upper.y);
    uint b11 = GpuPapiPermutation(j + upper.y);
    float3 s = r0 * r0 * (3.0 - 2.0 * r0);
    float a = lerp(dot(float3(r0.x, r0.y, r0.z), g_PapiNoise[b00 + lower.z].xyz),
        dot(float3(r1.x, r0.y, r0.z), g_PapiNoise[b10 + lower.z].xyz), s.x);
    float b = lerp(dot(float3(r0.x, r1.y, r0.z), g_PapiNoise[b01 + lower.z].xyz),
        dot(float3(r1.x, r1.y, r0.z), g_PapiNoise[b11 + lower.z].xyz), s.x);
    float c = lerp(a, b, s.y);
    a = lerp(dot(float3(r0.x, r0.y, r1.z), g_PapiNoise[b00 + upper.z].xyz),
        dot(float3(r1.x, r0.y, r1.z), g_PapiNoise[b10 + upper.z].xyz), s.x);
    b = lerp(dot(float3(r0.x, r1.y, r1.z), g_PapiNoise[b01 + upper.z].xyz),
        dot(float3(r1.x, r1.y, r1.z), g_PapiNoise[b11 + upper.z].xyz), s.x);
    return 1.5 * lerp(c, lerp(a, b, s.y), s.z);
}

float GpuPapiFractalSum(float3 position, float frequency, uint octaves, bool floorCoordinates)
{
    float sum = 0.0;
    float boost = frequency;
    for (uint i = 0u; i < octaves; ++i)
    {
        sum += GpuPapiNoise(position * frequency, floorCoordinates) / frequency;
        frequency *= 2.059;
    }
    return sum * boost;
}

#endif
