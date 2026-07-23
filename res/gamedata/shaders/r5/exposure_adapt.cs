// exposure_adapt.cs — classic CoP MiddleGray auto-exposure (bloom_luminance_3)
// scale = middlegray.x / (Lw * middlegray.y + middlegray.z)
// temporally lerped, clamped to [1/128, 20]
#define SM_5_0

cbuffer ExposureAdaptParams : register(b5)
{
    float g_middle_gray_x; // key (middlegray), or 1 when tonemap off
    float g_middle_gray_y; // amount (0 when tonemap off)
    float g_middle_gray_z; // lowlum blend floor
    float g_middle_gray_w; // adaptation blend factor [0..1]
};

StructuredBuffer<uint> g_histogram : register(t0);
RWTexture2D<float> g_exposure : register(u0);

// Recover approximate linear average luminance from 64 log2 bins.
// Classic CoP used a linear downsample of the bloom/high buffer; this is the
// FG stand-in (same MiddleGray formula on top).
float ComputeAverageLuminance()
{
    uint totalPixels = 0;
    for (uint i = 0; i < 64; i++)
        totalPixels += g_histogram[i];

    if (totalPixels == 0)
        return 1.0;

    // Match histogram binning in luminance_histogram.cs: log2 in [-10, +4]
    const float minLog = -10.0;
    const float logRange = 14.0;

    float weightedSum = 0.0;
    for (uint bin = 0; bin < 64; bin++)
    {
        uint c = g_histogram[bin];
        if (c == 0)
            continue;
        float t = (float(bin) + 0.5) / 64.0;
        float lum = exp2(minLog + t * logRange);
        weightedSum += lum * float(c);
    }
    return weightedSum / float(totalPixels);
}

[numthreads(1, 1, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    float Lw = ComputeAverageLuminance();
    float scale = g_middle_gray_x / max(Lw * g_middle_gray_y + g_middle_gray_z, 1e-6);

    float scalePrev = g_exposure[uint2(0, 0)];
    // First frame / seed: treat 0 as "no history"
    if (scalePrev <= 1e-6)
        scalePrev = 1.0;

    float rvalue = lerp(scalePrev, scale, saturate(g_middle_gray_w));
    rvalue = clamp(rvalue, 1.0 / 128.0, 20.0);

    g_exposure[uint2(0, 0)] = rvalue;
}
