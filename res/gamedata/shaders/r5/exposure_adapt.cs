#define SM_5_0

cbuffer ExposureAdaptParams : register(b5)
{
    float g_middle_gray_x;
    float g_middle_gray_y;
    float g_middle_gray_z;
    float g_pad0;
    float g_low_percentile;
    float g_high_percentile;
    float g_adapt_up;
    float g_adapt_down;
    float g_ev_bias;
    float g_delta_time;
    float g_pad1;
    float g_pad2;
};

StructuredBuffer<uint> g_histogram : register(t0);
RWTexture2D<float> g_exposure : register(u0);

float LuminanceFromBin(uint bin)
{
    const float minLog = -10.0;
    const float logRange = 14.0;
    float t = (float(bin) + 0.5) / 64.0;
    return exp2(minLog + t * logRange);
}

float PercentileLuminance(uint totalPixels, float percentile)
{
    float target = saturate(percentile) * float(totalPixels);
    uint accum = 0;
    for (uint bin = 0; bin < 64; bin++)
    {
        accum += g_histogram[bin];
        if (float(accum) >= target)
            return LuminanceFromBin(bin);
    }
    return LuminanceFromBin(63);
}

[numthreads(1, 1, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    uint totalPixels = 0;
    for (uint i = 0; i < 64; i++)
        totalPixels += g_histogram[i];

    float Lw = 1.0;
    if (totalPixels > 0)
    {
        float L50 = PercentileLuminance(totalPixels, g_low_percentile);
        float L90 = PercentileLuminance(totalPixels, g_high_percentile);
        Lw = max(lerp(L50, L90, 0.35), 1e-6);
    }

    float scale = g_middle_gray_x / max(Lw * g_middle_gray_y + g_middle_gray_z, 1e-6);
    scale *= exp2(g_ev_bias);
    scale = clamp(scale, 1.0 / 128.0, 20.0);

    float scalePrev = g_exposure[uint2(0, 0)];
    if (scalePrev <= 1e-6)
        scalePrev = 1.0;

    float rate = (scale > scalePrev) ? g_adapt_up : g_adapt_down;
    float alpha = saturate(1.0 - exp(-max(rate, 0.01) * max(g_delta_time, 1e-4)));
    float rvalue = lerp(scalePrev, scale, alpha);
    rvalue = clamp(rvalue, 1.0 / 128.0, 20.0);

    g_exposure[uint2(0, 0)] = rvalue;
}
