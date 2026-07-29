#ifndef OBJECT_PARALLAX_H
#define OBJECT_PARALLAX_H

float2 ObjectParallaxUV(MaterialData mat, float2 uv, float3 worldPos,
                        float3 T, float3 B, float3 N)
{
    if (parallax.x <= 0.0)
        return uv;
    if ((mat.flags & MAT_FLAG_HAS_NORMAL_X) == 0 || mat.normalXIndex == INVALID_TEXTURE_INDEX)
        return uv;
    if (mat.flags & (MAT_FLAG_ALPHA_TEST | MAT_FLAG_FOLIAGE))
        return uv;

    float3 V = normalize(eye_position - worldPos);
    float3 Vts = normalize(float3(dot(V, T), dot(V, B), dot(V, N)));
    if (Vts.z < 0.02)
        return uv;

    float viewZ = abs(mul(m_V, float4(worldPos, 1.0)).z);
    const float startFade = 8.0;
    const float stopFade = 20.0;
    if (viewZ >= stopFade)
        return uv;

    const float maxSamples = 16.0;
    const float minSamples = 6.0;
    float nSteps = lerp(maxSamples, minSamples, saturate(Vts.z));
    float stepSize = 1.0 / nSteps;
    float2 vDelta = Vts.xy * (-parallax.x) * 1.2;
    float2 offPerStep = stepSize * vDelta;

    Texture2D hx = GetBindlessTexture(mat.normalXIndex);
    float2 curOff = uv;
    float curH = 0.0;
    float curBound = 1.0;

    [loop]
    for (int i = 0; i < (int)nSteps; ++i)
    {
        if (curH < curBound)
        {
            curOff += offPerStep;
            curH = hx.Sample(smp_linear, curOff).a;
            curBound -= stepSize;
        }
    }

    float2 prevOff = curOff - offPerStep;
    float prevH = hx.Sample(smp_linear, prevOff).a;
    float d2 = (curBound + stepSize) - prevH;
    float d1 = curBound - curH;
    float denom = d2 - d1;
    float amt = (abs(denom) > 1e-5)
        ? (curBound * d2 - (curBound + stepSize) * d1) / denom
        : curBound;
    float fade = smoothstep(stopFade, startFade, viewZ);
    return uv + vDelta * ((1.0 - amt) * fade);
}

#endif
