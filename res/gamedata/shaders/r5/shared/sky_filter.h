#ifndef SHARED_SKY_FILTER_H
#define SHARED_SKY_FILTER_H

static const float SKY_FILTER_PI = 3.14159265359;

float2 SkyHammersley(uint index, uint count)
{
    return float2(float(index) / float(count), float(reversebits(index)) * 2.3283064365386963e-10);
}

float3 SkyImportanceSampleGGX(float2 xi, float alpha)
{
    float phi = 2.0 * SKY_FILTER_PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (alpha * alpha - 1.0) * xi.y));
    float sinTheta = sqrt(max(1.0 - cosTheta * cosTheta, 0.0));
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float SkyDistributionGGX(float NdotH, float alpha)
{
    float a2 = alpha * alpha;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(SKY_FILTER_PI * denom * denom, 1e-12);
}

#endif
