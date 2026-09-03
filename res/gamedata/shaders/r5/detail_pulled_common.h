#ifndef DETAIL_PULLED_COMMON_H
#define DETAIL_PULLED_COMMON_H

#include "detail_blade_common.h"

static const uint DETAIL_KIND_MESH = 3u;
static const uint DETAIL_KIND_DECAL = 4u;

struct DetailModelGPU
{
    float minScale;
    float maxScale;
    float flags;
    float geomExtentX;
    float geomExtentZ;
    float uv_min_x;
    float uv_min_y;
    float uv_max_x;
    float uv_max_y;
    uint pulledVertexBase;
    uint pulledIndexCount;
    float geomExtentY;
};

struct PulledVertex
{
    float px, py, pz;
    float u, v;
};

struct PulledInstance
{
    float3 pos;
    float scale;
    float c;
    float s;
    uint objectId;
};

PulledInstance DecodePulled(DetailInstance raw)
{
    PulledInstance inst;
    inst.pos = raw.pos;
    inst.objectId = raw.packed & 0x3Fu;
    float rotation = float((raw.packed >> 8) & 0x3FFu) / 1023.0 * BLADE_TWO_PI;
    inst.scale = float((raw.packed >> 18) & 0x3FFu) / 1023.0 * BLADE_PACK_MAX_SCALE;
    inst.c = cos(rotation);
    inst.s = sin(rotation);
    return inst;
}

float3 PulledRotate(PulledInstance inst, float3 v)
{
    return float3(v.x * inst.c - v.z * inst.s, v.y, v.x * inst.s + v.z * inst.c);
}

float3 PulledWorldPos(PulledInstance inst, PulledVertex v)
{
    return PulledRotate(inst, float3(v.px, v.py, v.pz) * inst.scale) + inst.pos;
}

float PulledHeightFactor(PulledVertex v, DetailModelGPU mdl)
{
    return saturate(v.py / max(mdl.geomExtentY, 0.01));
}

float3 PulledSway(float3 worldPos, float heightFactor, float time, float2 windAngleSpeed, float windDisplacement, Texture3D perlin, SamplerState smp)
{
    float windSpeed = max(windAngleSpeed.y, 0.1);
    float windAngle = windAngleSpeed.x * (BLADE_PI / 180.0);
    float2 globalDir = float2(sin(windAngle), cos(windAngle));
    float2 dirUv = worldPos.zx * (0.005 / windSpeed) + time * (0.005 * windSpeed);
    float windDirNoise = perlin.SampleLevel(smp, float3(dirUv, 0.0), 0).r;
    float2 strUv = worldPos.xz * (0.025 / windSpeed) + time * 0.05;
    float windStrNoise = perlin.SampleLevel(smp, float3(strUv, 0.0), 0).r;
    float strength = lerp(0.25, 1.0, windStrNoise);
    strength *= strength;
    strength *= windSpeed;
    float turbulence = (windDirNoise * 2.0 - 1.0) * 0.3;
    float2 perp = float2(-globalDir.y, globalDir.x);
    float2 windDir = normalize(globalDir + perp * turbulence);
    float displacement = strength * windDisplacement * heightFactor;
    worldPos.xz += displacement * windDir;
    return worldPos;
}

float3 PulledFaceNormal(PulledInstance inst, PulledVertex v0, PulledVertex v1, PulledVertex v2)
{
    float3 e1 = float3(v1.px - v0.px, v1.py - v0.py, v1.pz - v0.pz);
    float3 e2 = float3(v2.px - v0.px, v2.py - v0.py, v2.pz - v0.pz);
    float3 n = cross(e1, e2);
    float len = length(n);
    n = (len > 0.001) ? (n / len) : float3(0.0, 1.0, 0.0);
    return PulledRotate(inst, n);
}

#endif
