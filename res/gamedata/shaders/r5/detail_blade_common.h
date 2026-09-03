#ifndef DETAIL_BLADE_COMMON_H
#define DETAIL_BLADE_COMMON_H

struct DetailInstance
{
    float3 pos;
    uint packed;
};

struct GrassObjectTint
{
    float r, g, b, pad;
};

static const float BLADE_PACK_MAX_SCALE = 4.0;
static const float BLADE_TWO_PI = 6.28318530718;
static const float BLADE_PI = 3.1415926;
static const float BLADE_HEIGHT_VARIATION_MIN = 0.7;
static const float BLADE_HEIGHT_VARIATION_MAX = 1.15;
static const float BLADE_HEIGHT_NOISE_SCALE = 0.05;

uint BladeTriangleVertex(uint tri, uint corner, uint segments)
{
    uint quads = (segments - 1u) * 2u;
    if (tri < quads)
    {
        uint b = (tri >> 1) * 2u;
        if ((tri & 1u) == 0u)
            return corner == 0u ? b : (corner == 1u ? b + 2u : b + 1u);
        return corner == 0u ? b + 1u : (corner == 1u ? b + 2u : b + 3u);
    }
    uint b = (segments - 1u) * 2u;
    return corner == 0u ? b : (corner == 1u ? segments * 2u : b + 1u);
}

struct BladeInstance
{
    float3 pos;
    float scale;
    float rotation;
    uint objectId;
    float bladeHeight;
    float3 facing;
    float3 right;
    float bladeHash;
};

BladeInstance DecodeBlade(DetailInstance raw, Texture3D perlin, SamplerState smp, float heightMul)
{
    BladeInstance b;
    b.pos = raw.pos;
    b.objectId = raw.packed & 0x3Fu;
    b.rotation = float((raw.packed >> 8) & 0x3FFu) / 1023.0 * BLADE_TWO_PI;
    b.scale = float((raw.packed >> 18) & 0x3FFu) / 1023.0 * BLADE_PACK_MAX_SCALE;
    float2 heightUv = b.pos.xz * BLADE_HEIGHT_NOISE_SCALE + float2(0.37, 0.73);
    float heightNoise = perlin.SampleLevel(smp, float3(heightUv, 0.0), 0).r;
    b.bladeHeight = b.scale * lerp(BLADE_HEIGHT_VARIATION_MIN, BLADE_HEIGHT_VARIATION_MAX, heightNoise) * heightMul;
    float sr = sin(b.rotation);
    float cr = cos(b.rotation);
    b.facing = normalize(float3(sr, 0.0, cr));
    b.right = float3(cr, 0.0, -sr);
    int2 bc = int2(floor(b.pos.xz));
    uint bh = asuint(bc.x * 73856093 + bc.y * 19349663);
    bh ^= bh >> 16;
    b.bladeHash = float(bh & 0xFFFFu) / 65535.0;
    return b;
}

struct BladeWind
{
    float3 bendDir;
    float bendStrength;
    float windSpeed;
};

BladeWind EvalBladeWind(BladeInstance b, float time, float2 windAngleSpeed, float windDisplacement, Texture3D perlin, SamplerState smp)
{
    BladeWind w;
    w.windSpeed = max(windAngleSpeed.y, 0.1);
    float2 dirUv = b.pos.zx * (0.005 / w.windSpeed) + time * (0.005 * w.windSpeed);
    float windDirNoise = perlin.SampleLevel(smp, float3(dirUv, 0.0), 0).r;
    float2 strUv = b.pos.xz * (0.025 / w.windSpeed) + time * 0.05;
    float windStrNoise = perlin.SampleLevel(smp, float3(strUv, 0.0), 0).r;
    float fbmStrength = lerp(0.25, 1.0, windStrNoise);
    fbmStrength *= fbmStrength;
    fbmStrength *= w.windSpeed;
    float fbmTurb = (windDirNoise * 2.0 - 1.0) * 0.3;
    float angle = windAngleSpeed.x * (BLADE_PI / 180.0);
    float2 globalDir = float2(sin(angle), cos(angle));
    float2 perp = float2(-globalDir.y, globalDir.x);
    float2 windDirXZ = normalize(normalize(globalDir + perp * fbmTurb));
    float alignment = dot(windDirXZ, b.facing.xz);
    float resistance = lerp(0.3, 1.0, abs(alignment));
    w.bendDir = float3(windDirXZ.x, 0.0, windDirXZ.y);
    w.bendStrength = fbmStrength * windDisplacement * resistance;
    return w;
}

struct BladeVertex
{
    float3 pos;
    float3 rotatedNormal1;
    float3 rotatedNormal2;
    float t;
    float2 uv;
};

BladeVertex EvalBladeVertex(BladeInstance b, BladeWind w, uint localVert, uint segments, float time, float bladeWidth, Texture3D perlin, SamplerState smp)
{
    BladeVertex v;
    float localX;
    if (localVert < segments * 2u)
    {
        uint seg = localVert >> 1;
        uint side = localVert & 1u;
        v.t = float(seg) / float(segments);
        float width = bladeWidth * (1.0 - pow(v.t, 0.7));
        localX = (side == 0u) ? -width * 0.5 : width * 0.5;
        v.uv = float2(float(side), 1.0 - v.t);
    }
    else
    {
        v.t = 1.0;
        localX = 0.0;
        v.uv = float2(0.5, 0.0);
    }

    float2 turbUv = b.pos.xz * (0.025 / w.windSpeed) + (time + v.t * v.t * 0.25) * 0.05;
    float turbNoise = perlin.SampleLevel(smp, float3(turbUv, 0.0), 0).r;
    float turbulence = lerp(0.25, 1.0, turbNoise);
    turbulence *= turbulence;
    turbulence *= min(w.windSpeed, 1.0) * 0.3;

    float h = b.bladeHeight;
    float3 P0 = b.pos;
    float3 P1 = b.pos + float3(0.0, h * 0.33, 0.0);
    float3 P2 = b.pos + float3(0.0, h * 0.67, 0.0);
    float3 P3 = b.pos + float3(0.0, h, 0.0);

    float bendAngle = saturate(w.bendStrength / (h * 0.8)) * 1.2 + turbulence * v.t;
    if (w.bendStrength > 0.001)
    {
        P3 += w.bendDir * (h * sin(bendAngle));
        P3.y -= h * (1.0 - cos(bendAngle));
        float a1 = bendAngle * 0.33;
        P1 += w.bendDir * (h * 0.33 * sin(a1));
        P1.y -= h * 0.33 * (1.0 - cos(a1));
        float a2 = bendAngle * 0.67;
        P2 += w.bendDir * (h * 0.67 * sin(a2));
        P2.y -= h * 0.67 * (1.0 - cos(a2));
    }

    float t = v.t;
    float mt = 1.0 - t;
    float mt2 = mt * mt;
    float mt3 = mt2 * mt;
    float t2 = t * t;
    float t3 = t2 * t;
    float3 bezier = mt3 * P0 + 3.0 * mt2 * t * P1 + 3.0 * mt * t2 * P2 + t3 * P3;
    float3 tangentRaw = 3.0 * mt2 * (P1 - P0) + 6.0 * mt * t * (P2 - P1) + 3.0 * t2 * (P3 - P2);
    float tangentLen = length(tangentRaw);
    float3 tangent = (tangentLen > 0.001) ? (tangentRaw / tangentLen) : float3(0.0, 1.0, 0.0);

    v.pos = bezier + b.right * (localX * b.scale);

    float3 normalRaw = cross(tangent, b.right);
    float normalLen = length(normalRaw);
    float3 normal = (normalLen > 0.001) ? (normalRaw / normalLen) : b.facing;

    float rotationAngle = 3.14159 * 0.3;
    float cosTheta = cos(rotationAngle);
    float sinTheta = sin(rotationAngle);
    v.rotatedNormal1 = normal * cosTheta + cross(tangent, normal) * sinTheta + tangent * dot(tangent, normal) * (1.0 - cosTheta);
    v.rotatedNormal2 = normal * cosTheta + cross(tangent, normal) * (-sinTheta) + tangent * dot(tangent, normal) * (1.0 - cosTheta);
    return v;
}

#endif
