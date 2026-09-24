#ifndef SHARED_COLOR_SPACE_H
#define SHARED_COLOR_SPACE_H

static const float SRGB_MID_GRAY_LINEAR = 0.21404114;

float3 SrgbToLinear(float3 value)
{
    float3 low = value / 12.92;
    float3 high = pow((max(value, 0.0) + 0.055) / 1.055, 2.4);
    return lerp(high, low, step(value, 0.04045));
}

float3 LinearToSrgb(float3 value)
{
    value = max(value, 0.0);
    float3 low = value * 12.92;
    float3 high = 1.055 * pow(value, 1.0 / 2.4) - 0.055;
    return lerp(high, low, step(value, 0.0031308));
}

float LinearLuminance(float3 value)
{
    return dot(value, float3(0.2126, 0.7152, 0.0722));
}

float3 ApplyDetailModulation(float3 albedo, float3 detail)
{
    return albedo * detail * (1.0 / SRGB_MID_GRAY_LINEAR);
}

#endif
