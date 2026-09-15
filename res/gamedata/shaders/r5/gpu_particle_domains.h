#ifndef GPU_PARTICLE_DOMAINS_H
#define GPU_PARTICLE_DOMAINS_H

float GpuPapiRandom(inout uint state)
{
    state = state * 1664525u + 1013904223u;
    return float(state >> 8u) * (1.0 / 16777216.0);
}

float3 GpuPapiSafeNormal(float3 value)
{
    float squared = dot(value, value);
    return squared > 1.175494351e-38 ? value * sqrt(1.0 / squared) : value;
}

float GpuPapiNormalRandom(float sigma, inout uint state)
{
    if (sigma == 0.0)
        return 0.0;
    float y;
    do
    {
        y = -log(GpuPapiRandom(state));
    }
    while (GpuPapiRandom(state) > exp(-(y - 1.0) * (y - 1.0) * 0.5));
    GpuPapiRandom(state);
    return ((state & 0x80000000u) != 0u ? y : -y) * sigma / 0.7975;
}

float3 GpuPapiDirection(GpuPapiEmitter emitter, uint flags, float3 value)
{
    if ((flags & 2u) == 0u)
        return value;
    return emitter.actionBasisX.xyz * value.x + emitter.actionBasisY.xyz * value.y + emitter.actionBasisZ.xyz * value.z;
}

float3 GpuPapiPoint(GpuPapiEmitter emitter, uint flags, float3 value)
{
    return GpuPapiDirection(emitter, flags, value) + emitter.actionOrigin.xyz;
}

GpuPapiDomain GpuPapiTransformDomain(GpuPapiDomain domain, GpuPapiEmitter emitter, uint flags, bool direction)
{
    if ((emitter.flags & GPU_PAPI_PARENT_SET) == 0u)
        return domain;
    float3 translation = direction ? 0.0 : emitter.actionOrigin.xyz;
    if (domain.type == 4u)
    {
        float3 center = (domain.p1.xyz + domain.p2.xyz) * 0.5;
        float3 extent = (domain.p2.xyz - domain.p1.xyz) * 0.5;
        center = GpuPapiDirection(emitter, flags, center) + translation;
        if ((flags & 2u) != 0u)
            extent = abs(emitter.actionBasisX.xyz) * extent.x + abs(emitter.actionBasisY.xyz) * extent.y + abs(emitter.actionBasisZ.xyz) * extent.z;
        domain.p1.xyz = center - extent;
        domain.p2.xyz = center + extent;
        return domain;
    }
    domain.p1.xyz = GpuPapiDirection(emitter, flags, domain.p1.xyz) + translation;
    if (domain.type == 1u || domain.type == 2u || domain.type == 3u || domain.type == 6u ||
        domain.type == 7u || domain.type == 9u || domain.type == 10u)
        domain.p2.xyz = GpuPapiDirection(emitter, flags, domain.p2.xyz);
    if (domain.type == 2u || domain.type == 6u || domain.type == 7u || domain.type == 9u || domain.type == 10u)
    {
        domain.u.xyz = GpuPapiDirection(emitter, flags, domain.u.xyz);
        domain.v.xyz = GpuPapiDirection(emitter, flags, domain.v.xyz);
    }
    if (domain.type == 3u)
        domain.radii.x = -dot(domain.p1.xyz, domain.p2.xyz);
    return domain;
}

bool GpuPapiWithin(GpuPapiDomain domain, float3 position, inout uint state)
{
    float3 offset = position - domain.p1.xyz;
    float radiusSquared = dot(offset, offset);
    switch (domain.type)
    {
    case 4u: return all(position >= domain.p1.xyz) && all(position <= domain.p2.xyz);
    case 3u: return dot(position, domain.p2.xyz) >= -domain.radii.x;
    case 5u: return radiusSquared <= domain.radii.z && radiusSquared >= domain.radii.w;
    case 6u:
    case 7u:
    {
        float distance = dot(domain.p2.xyz, offset) * domain.radii.w;
        if (distance < 0.0 || distance > 1.0)
            return false;
        float3 radial = offset - domain.p2.xyz * distance;
        float radialSquared = dot(radial, radial);
        float2 radii = domain.radii.xy * (domain.type == 7u ? distance : 1.0);
        float outerSquared = domain.type == 7u ? radii.x * radii.x : domain.radii.z;
        return radialSquared <= outerSquared && radialSquared >= radii.y * radii.y;
    }
    case 8u: return GpuPapiRandom(state) < exp(radiusSquared * domain.radii.w) * domain.radii.y;
    default: return false;
    }
}

float3 GpuPapiGenerate(GpuPapiDomain domain, inout uint state)
{
    switch (domain.type)
    {
    case 0u:
    case 3u: return domain.p1.xyz;
    case 1u: return domain.p1.xyz + domain.p2.xyz * GpuPapiRandom(state);
    case 4u:
    {
        float x = GpuPapiRandom(state);
        float y = GpuPapiRandom(state);
        float z = GpuPapiRandom(state);
        return domain.p1.xyz + (domain.p2.xyz - domain.p1.xyz) * float3(x, y, z);
    }
    case 2u:
    case 10u:
    {
        float x = GpuPapiRandom(state);
        float y = GpuPapiRandom(state);
        if (domain.type == 2u && x + y >= 1.0)
        {
            x = 1.0 - x;
            y = 1.0 - y;
        }
        return domain.p1.xyz + domain.u.xyz * x + domain.v.xyz * y;
    }
    case 5u:
    {
        float x = GpuPapiRandom(state);
        float y = GpuPapiRandom(state);
        float z = GpuPapiRandom(state);
        float3 unit = GpuPapiSafeNormal(float3(x, y, z) - 0.5);
        float radius = domain.radii.x;
        if (domain.radii.x != domain.radii.y)
            radius = domain.radii.y + GpuPapiRandom(state) * (domain.radii.x - domain.radii.y);
        return domain.p1.xyz + unit * radius;
    }
    case 6u:
    case 7u:
    case 9u:
    {
        float distance = domain.type == 9u ? 0.0 : GpuPapiRandom(state);
        float theta = GpuPapiRandom(state) * 6.283185307179586;
        float radius = domain.radii.y + GpuPapiRandom(state) * (domain.radii.x - domain.radii.y);
        float x = radius * cos(theta);
        float y = radius * sin(theta);
        if (domain.type == 7u)
        {
            x *= distance;
            y *= distance;
        }
        return domain.p1.xyz + domain.p2.xyz * distance + domain.u.xyz * x + domain.v.xyz * y;
    }
    case 8u:
    {
        float x = GpuPapiNormalRandom(domain.radii.x, state);
        float y = GpuPapiNormalRandom(domain.radii.x, state);
        float z = GpuPapiNormalRandom(domain.radii.x, state);
        return domain.p1.xyz + float3(x, y, z);
    }
    default: return 0.0;
    }
}

uint GpuPapiPackColor(float4 color)
{
    uint4 value = uint4(clamp(floor(color * 255.0), 0.0, 255.0));
    return (value.w << 24u) | (value.x << 16u) | (value.y << 8u) | value.z;
}

float4 GpuPapiUnpackColor(uint color)
{
    return float4((color >> 16u) & 255u, (color >> 8u) & 255u, color & 255u, color >> 24u) / 255.0;
}

#endif
