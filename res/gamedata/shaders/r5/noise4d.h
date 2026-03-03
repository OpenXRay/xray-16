// noise4d.h
// 4D Perlin gradient noise + fBm + turbulence displacement.
// Shared by smoke_trail_compact.cs and trail.vs.

#ifndef NOISE4D_H
#define NOISE4D_H

// ─────────────────────────────────────────────────────────
//  4D PERLIN GRADIENT NOISE
//  PCG hash → normalized 4D gradient → quintic Hermite interp
// ─────────────────────────────────────────────────────────

uint _pcg(uint v)
{
    uint state = v * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float4 _grad4(int4 ip)
{
    uint h = _pcg(asuint(ip.x));
    h = _pcg(h ^ asuint(ip.y));
    h = _pcg(h ^ asuint(ip.z));
    h = _pcg(h ^ asuint(ip.w));

    float4 g;
    g.x = float(h & 0xFFu) / 127.5f - 1.0f;
    g.y = float((h >> 8u) & 0xFFu) / 127.5f - 1.0f;
    g.z = float((h >> 16u) & 0xFFu) / 127.5f - 1.0f;
    g.w = float((h >> 24u) & 0xFFu) / 127.5f - 1.0f;
    return normalize(g);
}

float4 _quintic(float4 t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float perlin4D(float4 p)
{
    int4 i = int4(floor(p));
    float4 f = p - floor(p);
    float4 u = _quintic(f);

    float n0000 = dot(_grad4(i + int4(0,0,0,0)), f - float4(0,0,0,0));
    float n1000 = dot(_grad4(i + int4(1,0,0,0)), f - float4(1,0,0,0));
    float n0100 = dot(_grad4(i + int4(0,1,0,0)), f - float4(0,1,0,0));
    float n1100 = dot(_grad4(i + int4(1,1,0,0)), f - float4(1,1,0,0));
    float n0010 = dot(_grad4(i + int4(0,0,1,0)), f - float4(0,0,1,0));
    float n1010 = dot(_grad4(i + int4(1,0,1,0)), f - float4(1,0,1,0));
    float n0110 = dot(_grad4(i + int4(0,1,1,0)), f - float4(0,1,1,0));
    float n1110 = dot(_grad4(i + int4(1,1,1,0)), f - float4(1,1,1,0));
    float n0001 = dot(_grad4(i + int4(0,0,0,1)), f - float4(0,0,0,1));
    float n1001 = dot(_grad4(i + int4(1,0,0,1)), f - float4(1,0,0,1));
    float n0101 = dot(_grad4(i + int4(0,1,0,1)), f - float4(0,1,0,1));
    float n1101 = dot(_grad4(i + int4(1,1,0,1)), f - float4(1,1,0,1));
    float n0011 = dot(_grad4(i + int4(0,0,1,1)), f - float4(0,0,1,1));
    float n1011 = dot(_grad4(i + int4(1,0,1,1)), f - float4(1,0,1,1));
    float n0111 = dot(_grad4(i + int4(0,1,1,1)), f - float4(0,1,1,1));
    float n1111 = dot(_grad4(i + int4(1,1,1,1)), f - float4(1,1,1,1));

    float nx00_w0 = lerp(n0000, n1000, u.x);
    float nx10_w0 = lerp(n0100, n1100, u.x);
    float nx01_w0 = lerp(n0010, n1010, u.x);
    float nx11_w0 = lerp(n0110, n1110, u.x);
    float nx00_w1 = lerp(n0001, n1001, u.x);
    float nx10_w1 = lerp(n0101, n1101, u.x);
    float nx01_w1 = lerp(n0011, n1011, u.x);
    float nx11_w1 = lerp(n0111, n1111, u.x);

    float nxy0_w0 = lerp(nx00_w0, nx10_w0, u.y);
    float nxy1_w0 = lerp(nx01_w0, nx11_w0, u.y);
    float nxy0_w1 = lerp(nx00_w1, nx10_w1, u.y);
    float nxy1_w1 = lerp(nx01_w1, nx11_w1, u.y);

    float nxyz_w0 = lerp(nxy0_w0, nxy1_w0, u.z);
    float nxyz_w1 = lerp(nxy0_w1, nxy1_w1, u.z);

    return lerp(nxyz_w0, nxyz_w1, u.w);
}

// ─────────────────────────────────────────────────────────
//  fBm on 4D Perlin — 4 octaves, lacunarity=2, gain=0.5
// ─────────────────────────────────────────────────────────

float fbm4D(float4 p)
{
    float sum = 0.0f;
    float amp = 1.0f;
    float freq = 1.0f;
    [unroll] for (int i = 0; i < 4; i++)
    {
        sum += amp * perlin4D(p * freq);
        freq *= 2.0f;
        amp *= 0.5f;
    }
    return sum;
}

// ─────────────────────────────────────────────────────────
//  TURBULENCE DISPLACEMENT (AE-style)
//  3 independent fBm channels → XYZ displacement
//  Spherical falloff from center point
// ─────────────────────────────────────────────────────────

float3 turbulenceDisplace(float3 worldPos, float evolution,
                          float amount, float frequency,
                          float3 sphereCenter, float sphereRadius)
{
    float4 p = float4(worldPos * frequency, evolution);

    float3 disp;
    disp.x = fbm4D(p);
    disp.y = fbm4D(p + float4(31.416f, -47.853f, 12.679f, 0.0f));
    disp.z = fbm4D(p + float4(-17.236f, 53.147f, -28.912f, 0.0f));

    float dist = length(worldPos - sphereCenter);
    float falloff = 1.0f - smoothstep(0.0f, sphereRadius, dist);

    return disp * amount * falloff;
}

#endif // NOISE4D_H
