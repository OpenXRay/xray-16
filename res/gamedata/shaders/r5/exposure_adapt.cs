#include "exposure_common.h"

RWByteAddressBuffer u_Histogram : register(u0);
RWStructuredBuffer<float4> u_Exposure : register(u1);

cbuffer ExposureAdaptParams : register(b0)
{
    float g_Compensation;
    float g_MinEv;
    float g_MaxEv;
    float g_RateUp;
    float g_RateDown;
    float g_DeltaTime;
    uint g_AutoExposure;
    uint g_Reset;
};

static const float EXPOSURE_LOW_PERCENTILE = 0.5;
static const float EXPOSURE_HIGH_PERCENTILE = 0.95;
static const float EXPOSURE_MIDDLE_GRAY_LOG2 = -2.4739312;

groupshared uint gs_Counts[EXPOSURE_HISTOGRAM_BINS];

float BinLog2Luminance(uint bin)
{
    if (bin == 0u)
        return EXPOSURE_MIN_LOG2_LUMINANCE;
    return EXPOSURE_MIN_LOG2_LUMINANCE + float(bin - 1u) / float(EXPOSURE_HISTOGRAM_BINS - 2) * EXPOSURE_LOG2_LUMINANCE_RANGE;
}

[numthreads(EXPOSURE_HISTOGRAM_BINS, 1, 1)]
void main(uint groupIndex : SV_GroupIndex)
{
    gs_Counts[groupIndex] = u_Histogram.Load(groupIndex * 4u);
    u_Histogram.Store(groupIndex * 4u, 0u);
    GroupMemoryBarrierWithGroupSync();
    if (groupIndex != 0u)
        return;

    float4 previous = u_Exposure[0];
    bool previousValid = all(isfinite(previous)) && previous.x > 0.0;
    float averageLog2 = previousValid ? previous.w : EXPOSURE_MIDDLE_GRAY_LOG2;
    float targetEv = g_Compensation;

    if (g_AutoExposure != 0u)
    {
        uint total = 0u;
        for (uint i = 0u; i < EXPOSURE_HISTOGRAM_BINS; ++i)
            total += gs_Counts[i];

        if (total != 0u)
        {
            float low = float(total) * EXPOSURE_LOW_PERCENTILE;
            float high = float(total) * EXPOSURE_HIGH_PERCENTILE;
            float accumulated = 0.0;
            float weightSum = 0.0;
            float log2Sum = 0.0;
            for (uint bin = 0u; bin < EXPOSURE_HISTOGRAM_BINS; ++bin)
            {
                float begin = accumulated;
                accumulated += float(gs_Counts[bin]);
                float weight = max(0.0, min(accumulated, high) - max(begin, low));
                weightSum += weight;
                log2Sum += weight * BinLog2Luminance(bin);
            }
            if (weightSum > 0.0)
                averageLog2 = log2Sum / weightSum;
        }
        targetEv = clamp(EXPOSURE_MIDDLE_GRAY_LOG2 - averageLog2, g_MinEv, g_MaxEv) + g_Compensation;
    }

    float currentEv = targetEv;
    if (g_Reset == 0u && previousValid)
    {
        float rate = targetEv > previous.y ? g_RateUp : g_RateDown;
        currentEv = previous.y + (targetEv - previous.y) * (1.0 - exp(-max(g_DeltaTime, 0.0) * rate));
    }

    u_Exposure[0] = float4(exp2(currentEv), currentEv, targetEv, averageLog2);
}
