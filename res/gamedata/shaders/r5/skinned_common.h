#ifndef SKINNED_COMMON_H
#define SKINNED_COMMON_H

StructuredBuffer<float4x4> g_BoneMatrices : register(t3);

cbuffer SkinnedMaterialCB : register(b4)
{
    uint g_SkinnedMaterialID;
    uint g_SkeletonBoneOffset;
    uint g_SplatOffset;
    uint g_SplatCount;
};

struct PaintSplat
{
    float4 posRadius;       // xyz = rest-pose position, w = world-space radius
    float4 color;           // rgb + alpha
    uint4  boneIdx;         // up to 4 bone indices (local to skeleton)
    float4 boneWeights;     // corresponding weights (sum = 1)
    float2 hitUV;           // hit UV in target diffuse UV space
    float uvRadius;         // radius in UV space for stamp sampling
    uint wallmarkMaterialID; // bindless material ID for wallmark texture
    float4 evolution;       // x=spawnTime, y=invLifetime, z=seed, w=mode
};

StructuredBuffer<PaintSplat> g_PaintSplats : register(t11);

float4x4 get_bone(int legacy_index)
{
    return g_BoneMatrices[g_SkeletonBoneOffset + (legacy_index / 3)];
}

float4 skinning_pos(float4 pos, float4x4 bone)
{
    return mul(bone, pos);
}

float3 skinning_dir(float3 dir, float4x4 bone)
{
    return mul((float3x3)bone, dir);
}

float3 unpack_d3dcolor_normal(float3 packed)
{
    return packed * 2.0 - 1.0;
}

float3 skin_splat_pos(PaintSplat splat)
{
    float3 result = 0;
    for (uint i = 0; i < 4; i++)
    {
        float4x4 bone = g_BoneMatrices[g_SkeletonBoneOffset + splat.boneIdx[i]];
        result += mul(bone, float4(splat.posRadius.xyz, 1)).xyz * splat.boneWeights[i];
    }
    return result;
}

#define SPLAT_MODE_DECAL              0u
#define SPLAT_MODE_PROCEDURAL_BLOOD   1u

uint get_splat_mode(PaintSplat splat)
{
    return (uint)(splat.evolution.w + 0.5);
}

float get_splat_age(PaintSplat splat)
{
    float invLifetime = max(splat.evolution.y, 0.0);
    if (invLifetime <= 1e-5)
        return 0.0;
    return saturate((timers.x - splat.evolution.x) * invLifetime);
}

float2 get_splat_local_uv(PaintSplat splat, float2 meshUV)
{
    return (meshUV - splat.hitUV) / max(splat.uvRadius, 1e-5);
}

float hash21(float2 p)
{
    p = frac(p * float2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return frac(p.x * p.y);
}

float noise2(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    float a = hash21(i);
    float b = hash21(i + float2(1, 0));
    float c = hash21(i + float2(0, 1));
    float d = hash21(i + float2(1, 1));

    float2 u = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

float fbm2(float2 p)
{
    float v = 0.0;
    float a = 0.5;
    v += a * noise2(p); p *= 2.0; a *= 0.5;
    v += a * noise2(p); p *= 2.0; a *= 0.5;
    v += a * noise2(p);
    return v;
}

float procedural_splat_mask(float2 localUV, float seed, float age)
{
    float radial = length(localUV);
    if (radial > 1.5)
        return 0.0;

    float angle = atan2(localUV.y, localUV.x);
    float edgeNoise = fbm2(localUV * 3.5 + float2(seed * 0.37, seed * 0.73));
    float lobe = sin(angle * (6.0 + frac(seed) * 3.0) + seed * 6.28318) * 0.11;
    float edge = 1.0 + lobe + (edgeNoise - 0.5) * 0.35;
    float softness = lerp(0.10, 0.28, age);

    float mask = 1.0 - smoothstep(edge - softness, edge + 0.05, radial);

    float pits = fbm2(localUV * 10.0 + float2(seed * 1.31, seed * 2.17));
    float pitMask = lerp(1.0, saturate(0.55 + pits * 0.7), 0.25);
    return saturate(mask * pitMask);
}

float3 apply_splat_deform(float3 worldPos, float3 worldNormal)
{
    float3 offset = 0;
    for (uint i = 0; i < g_SplatCount; i++)
    {
        PaintSplat splat = g_PaintSplats[g_SplatOffset + i];
        float3 splatWorldPos = mul(m_W, float4(skin_splat_pos(splat), 1)).xyz;
        float dist = distance(worldPos, splatWorldPos);
        float r = splat.posRadius.w;
        if (dist < r)
        {
            float fade = 1.0 - smoothstep(r * 0.5, r, dist);
            offset -= worldNormal * (splat.color.a * fade * dev_param_1.x);
        }
    }
    return offset;
}

float4 sample_decal_stamp(PaintSplat splat, float2 meshUV)
{
    if (splat.wallmarkMaterialID == INVALID_TEXTURE_INDEX || splat.uvRadius <= 1e-5)
        return float4(1, 1, 1, 1);

    float2 uvMin = splat.hitUV - splat.uvRadius;
    float2 uvMax = splat.hitUV + splat.uvRadius;
    if (any(meshUV < uvMin) || any(meshUV > uvMax))
        return float4(0, 0, 0, 0);

    float2 stampUV = (meshUV - uvMin) / max(2.0 * splat.uvRadius, 1e-5);
    MaterialData wallmarkMat = g_Materials[splat.wallmarkMaterialID];
    return SampleDiffuse(wallmarkMat, stampUV);
}

float4 sample_procedural_blood(PaintSplat splat, float2 meshUV)
{
    if (splat.uvRadius <= 1e-5)
        return float4(0, 0, 0, 0);

    float age = get_splat_age(splat);
    float seed = splat.evolution.z;
    float2 localUV = get_splat_local_uv(splat, meshUV);
    float radial = length(localUV);
    float baseMask = procedural_splat_mask(localUV, seed, age);
    float mask = baseMask;
    if (mask <= 1e-5)
        return float4(0, 0, 0, 0);

    float dry = smoothstep(0.2, 1.0, age);
    float3 freshColor = splat.color.rgb;
    float3 dryColor = freshColor * float3(0.34, 0.15, 0.13);
    float3 finalColor = lerp(freshColor, dryColor, dry);

    // Add a separate impact-hole core so procedural mode reads as blood + entry wound.
    float holeRadius = lerp(0.10, 0.14, hash21(float2(seed, seed * 1.73)));
    float holeNoise = noise2(localUV * 22.0 + float2(seed * 0.41, seed * 0.97));
    float holeMask = (1.0 - smoothstep(holeRadius * 0.7, holeRadius * 1.6, radial));
    holeMask *= saturate(0.65 + holeNoise * 0.55);
    holeMask *= saturate(mask * 1.35);
    float holeAgeFade = 1.0 - smoothstep(0.55, 1.0, age);
    holeMask *= lerp(1.0, 0.7, dry) * holeAgeFade;
    finalColor = lerp(finalColor, float3(0.02, 0.005, 0.005), saturate(holeMask * 0.95));

    float finalAlpha = mask * lerp(1.0, 0.75, dry);
    return float4(finalColor, finalAlpha);
}

float3 apply_splat_color(float3 albedo, float3 worldPos, float2 meshUV)
{
    for (uint i = 0; i < g_SplatCount; i++)
    {
        PaintSplat splat = g_PaintSplats[g_SplatOffset + i];
        float3 splatWorldPos = mul(m_W, float4(skin_splat_pos(splat), 1)).xyz;
        float dist = distance(worldPos, splatWorldPos);
        float r = splat.posRadius.w;
        uint mode = get_splat_mode(splat);
        float reach = (mode == SPLAT_MODE_PROCEDURAL_BLOOD) ? (r * 1.8) : r;
        if (dist < reach)
        {
            float fade = 1.0 - smoothstep(r * 0.5, reach, dist);

            if (mode == SPLAT_MODE_PROCEDURAL_BLOOD)
            {
                float4 proc = sample_procedural_blood(splat, meshUV);
                if (proc.a <= 1e-5)
                    continue;
                float procAlpha = saturate(fade * proc.a);
                albedo = lerp(albedo, proc.rgb, procAlpha);
                continue;
            }

            float4 stamp = sample_decal_stamp(splat, meshUV);
            if (stamp.a <= 1e-5)
                continue;

            float3 splatColor = splat.color.rgb * stamp.rgb;
            float splatAlpha = splat.color.a * fade * stamp.a;
            albedo = lerp(albedo, splatColor, saturate(splatAlpha));
        }
    }
    return albedo;
}

#endif // SKINNED_COMMON_H
