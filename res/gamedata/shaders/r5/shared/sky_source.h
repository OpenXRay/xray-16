#ifndef SHARED_SKY_SOURCE_H
#define SHARED_SKY_SOURCE_H

float3 SkyCubeFaceDirection(uint face, float2 uv)
{
    float3 direction;
    if (face == 0u)
        direction = float3(1.0, -uv.y, -uv.x);
    else if (face == 1u)
        direction = float3(-1.0, -uv.y, uv.x);
    else if (face == 2u)
        direction = float3(uv.x, 1.0, uv.y);
    else if (face == 3u)
        direction = float3(uv.x, -1.0, -uv.y);
    else if (face == 4u)
        direction = float3(uv.x, -uv.y, 1.0);
    else
        direction = float3(-uv.x, -uv.y, -1.0);
    return normalize(direction);
}

float3 SkyRotateY(float3 direction, float angle)
{
    float s, c;
    sincos(angle, s, c);
    return float3(c * direction.x + s * direction.z, direction.y, c * direction.z - s * direction.x);
}

float3 SkyHalfBoxWarp(float3 direction)
{
    float3 a = abs(direction);
    float3 p = direction / max(a.x, max(a.y, a.z));
    if (a.y >= a.x && a.y >= a.z)
        return p.y > 0.0 ? p : float3(p.x, -1.01, p.z);
    float y = p.y >= -0.01 ? (2.0 * p.y - 0.99) / 1.01 : -1.01 + 0.01 * (p.y + 1.0) / 0.99;
    return float3(p.x, y, p.z);
}

float3 SkyHorizonDirection(float3 direction)
{
    float2 planar = direction.xz;
    if (dot(planar, planar) < 1e-8)
        planar = float2(1.0, 0.0);
    return float3(planar.x, 0.0, planar.y);
}

#endif
