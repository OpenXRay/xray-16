#ifndef PARALLAX_H
#define PARALLAX_H

float SampleHeightIndex(uint index, float2 uv)
{
    if (index == INVALID_TEXTURE_INDEX)
        return 0.5;
    return GetBindlessTexture(index).SampleLevel(smp_linear, uv, 0).a;
}

bool HeightHasRelief(uint heightIndex, float2 uv)
{
    float h0 = SampleHeightIndex(heightIndex, uv);
    float h1 = SampleHeightIndex(heightIndex, uv + float2(0.012, 0.0));
    float h2 = SampleHeightIndex(heightIndex, uv + float2(0.0, 0.012));
    float contrast = max(abs(h0 - h1), abs(h0 - h2));
    return contrast > 0.045;
}

float2 ParallaxOffsetUV(float2 uv, uint heightIndex, float3 T, float3 B, float3 N, float3 worldPos, bool forceSteep)
{
    if (heightIndex == INVALID_TEXTURE_INDEX)
        return uv;
    float scale = parallax.x;
    if (scale <= 1e-5)
        return uv;
    if (!HeightHasRelief(heightIndex, uv))
        return uv;
    float3 Nw = normalize(N);
    float tlen = length(T);
    float blen = length(B);
    if (tlen < 1e-4 || blen < 1e-4)
        return uv;
    float3 Tw = T / tlen;
    float3 Bw = B / blen;
    float3 toEye = eye_position.xyz - worldPos;
    float dist = length(toEye);
    if (dist < 1e-4)
        return uv;
    float3 V = toEye / dist;
    float fade = 1.0 - saturate((dist - 8.0) / 4.0);
    if (fade <= 1e-3)
        return uv;
    float3 viewTS = float3(dot(V, Tw), dot(V, Bw), dot(V, Nw));
    float vlen = length(viewTS);
    if (vlen < 1e-4)
        return uv;
    viewTS /= vlen;
    if (viewTS.z < 0.15)
        return uv;
    bool steep = forceSteep;
    if (steep)
    {
        int steps = (int)lerp(16.0, 6.0, saturate(viewTS.z));
        steps = clamp(steps, 6, 16);
        float stepSize = 1.0 / float(steps);
        float2 delta = viewTS.xy * (-scale * 1.2);
        float2 stepUV = delta * stepSize;
        float2 cur = uv;
        float currH = 0.0;
        float bound = 1.0;
        [loop]
        for (int i = 0; i < steps; ++i)
        {
            if (currH < bound)
            {
                cur += stepUV;
                currH = SampleHeightIndex(heightIndex, cur);
                bound -= stepSize;
            }
        }
        cur -= stepUV;
        float prevH = SampleHeightIndex(heightIndex, cur);
        float d2 = (bound + stepSize) - prevH;
        float d1 = bound - currH;
        float amount = (bound * d2 - (bound + stepSize) * d1) / max(d2 - d1, 1e-5);
        return uv + delta * ((1.0 - amount) * fade);
    }
    float h = SampleHeightIndex(heightIndex, uv);
    h = h * parallax.x + parallax.y;
    return uv + h * viewTS.xy * fade;
}

float2 ApplyMaterialParallaxUV(MaterialData mat, float2 uv, float3 T, float3 B, float3 N, float3 worldPos)
{
    if ((mat.flags & MAT_FLAG_HAS_PBR) == 0)
        return uv;
    bool steep = ((mat.flags & MAT_FLAG_STEEP_PARALLAX) != 0) || (parallax.z > 0.5);
    return ParallaxOffsetUV(uv, mat.pbrIndex, T, B, N, worldPos, steep);
}

float TerrainBlendedHeight(TerrainMaterialData mat, float4 mask, float2 detailUV)
{
    float hR = SampleHeightIndex(mat.pbrR_Index, detailUV);
    float hG = SampleHeightIndex(mat.pbrG_Index, detailUV);
    float hB = SampleHeightIndex(mat.pbrB_Index, detailUV);
    float hA = SampleHeightIndex(mat.pbrA_Index, detailUV);
    return hR * mask.r + hG * mask.g + hB * mask.b + hA * mask.a;
}

bool TerrainHeightHasRelief(TerrainMaterialData mat, float4 mask, float2 detailUV)
{
    float h0 = TerrainBlendedHeight(mat, mask, detailUV);
    float h1 = TerrainBlendedHeight(mat, mask, detailUV + float2(0.012, 0.0));
    float h2 = TerrainBlendedHeight(mat, mask, detailUV + float2(0.0, 0.012));
    float contrast = max(abs(h0 - h1), abs(h0 - h2));
    return contrast > 0.045;
}

float2 ApplyTerrainParallaxUV(TerrainMaterialData mat, float4 mask, float2 detailUV, float3 T, float3 B, float3 N, float3 worldPos)
{
    if ((mat.flags & MAT_FLAG_HAS_PBR_LAYER) == 0)
        return detailUV;
    float scale = parallax.x;
    if (scale <= 1e-5)
        return detailUV;
    if (!TerrainHeightHasRelief(mat, mask, detailUV))
        return detailUV;
    float3 Nw = normalize(N);
    float tlen = length(T);
    float blen = length(B);
    if (tlen < 1e-4 || blen < 1e-4)
        return detailUV;
    float3 Tw = T / tlen;
    float3 Bw = B / blen;
    float3 toEye = eye_position.xyz - worldPos;
    float dist = length(toEye);
    if (dist < 1e-4)
        return detailUV;
    float3 V = toEye / dist;
    float fade = 1.0 - saturate((dist - 8.0) / 4.0);
    if (fade <= 1e-3)
        return detailUV;
    float3 viewTS = float3(dot(V, Tw), dot(V, Bw), dot(V, Nw));
    float vlen = length(viewTS);
    if (vlen < 1e-4)
        return detailUV;
    viewTS /= vlen;
    if (viewTS.z < 0.15)
        return detailUV;
    bool steep = ((mat.flags & MAT_FLAG_STEEP_PARALLAX) != 0) || (parallax.z > 0.5);
    if (steep)
    {
        int steps = (int)lerp(16.0, 6.0, saturate(viewTS.z));
        steps = clamp(steps, 6, 16);
        float stepSize = 1.0 / float(steps);
        float2 delta = viewTS.xy * (-scale * 1.2);
        float2 stepUV = delta * stepSize;
        float2 cur = detailUV;
        float currH = 0.0;
        float bound = 1.0;
        [loop]
        for (int i = 0; i < steps; ++i)
        {
            if (currH < bound)
            {
                cur += stepUV;
                currH = TerrainBlendedHeight(mat, mask, cur);
                bound -= stepSize;
            }
        }
        cur -= stepUV;
        float prevH = TerrainBlendedHeight(mat, mask, cur);
        float d2 = (bound + stepSize) - prevH;
        float d1 = bound - currH;
        float amount = (bound * d2 - (bound + stepSize) * d1) / max(d2 - d1, 1e-5);
        return detailUV + delta * ((1.0 - amount) * fade);
    }
    float h = TerrainBlendedHeight(mat, mask, detailUV);
    h = h * parallax.x + parallax.y;
    return detailUV + h * viewTS.xy * fade;
}

#endif
