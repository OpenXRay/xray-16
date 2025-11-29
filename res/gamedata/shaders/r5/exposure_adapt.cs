#define SM_5_0
#include "common.h"

cbuffer ExposureAdaptParams : register(b5)
{
    float g_min_log_luminance;
    float g_log_luminance_range;
    float g_low_percentile;
    float g_high_percentile;

    float g_adapt_speed_up;
    float g_adapt_speed_down;
    float g_delta_time;
    float g_exposure_compensation;

    float g_min_exposure;
    float g_max_exposure;
    float g_calibration_constant;
    float g_padding;
};

StructuredBuffer<uint> g_histogram : register(t0);
RWTexture2D<float> g_exposure : register(u0);

float ComputeAverageLuminance()
{
    uint totalPixels = 0;
    for (uint i = 0; i < 64; i++)
    {
        totalPixels += g_histogram[i];
    }

    if (totalPixels == 0)
        return 0.5;

    uint lowThreshold = (uint)(totalPixels * g_low_percentile);
    uint highThreshold = (uint)(totalPixels * g_high_percentile);

    float weightedSum = 0.0;
    uint validPixels = 0;
    uint runningCount = 0;

    for (uint bin = 0; bin < 64; bin++)
    {
        uint binCount = g_histogram[bin];
        uint prevCount = runningCount;
        runningCount += binCount;

        if (runningCount <= lowThreshold)
            continue;

        if (prevCount >= highThreshold)
            break;

        uint startInBin = max(prevCount, lowThreshold) - prevCount;
        uint endInBin = min(runningCount, highThreshold) - prevCount;
        uint countInRange = endInBin - startInBin;

        if (countInRange > 0)
        {
            float t = (float(bin) + 0.5) / 64.0;
            float logLum = g_min_log_luminance + t * g_log_luminance_range;
            float luminance = exp2(logLum);

            weightedSum += luminance * float(countInRange);
            validPixels += countInRange;
        }
    }

    if (validPixels == 0)
        return 0.5;

    return weightedSum / float(validPixels);
}

float ComputeTargetExposure(float avgLuminance)
{

    float lum = max(avgLuminance, 0.0001);

    float exposure = g_calibration_constant / (lum * 100.0);

    exposure *= exp2(g_exposure_compensation);

    return clamp(exposure, g_min_exposure, g_max_exposure);
}

float AdaptExposure(float currentExposure, float targetExposure)
{
    float adaptSpeed = (targetExposure > currentExposure)
        ? g_adapt_speed_up
        : g_adapt_speed_down;

    float adaptFactor = 1.0 - exp(-g_delta_time * adaptSpeed);

    return lerp(currentExposure, targetExposure, adaptFactor);
}

[numthreads(1, 1, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    float currentExposure = g_exposure[uint2(0, 0)];

    float avgLuminance = ComputeAverageLuminance();

    float targetExposure = ComputeTargetExposure(avgLuminance);

    float newExposure = AdaptExposure(currentExposure, targetExposure);

    g_exposure[uint2(0, 0)] = newExposure;
}
