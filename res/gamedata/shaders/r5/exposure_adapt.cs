#define SM_5_0

cbuffer ExposureAdaptParams : register(b5)
{
    float4 g_MiddleGray;
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

float AverageLuminance(uint totalPixels)
{
    if (totalPixels == 0)
        return 1.0;

    float weightedSum = 0.0;
    for (uint bin = 0; bin < 64; bin++)
    {
        uint c = g_histogram[bin];
        if (c > 0)
            weightedSum += LuminanceFromBin(bin) * float(c);
    }
    return max(weightedSum / float(totalPixels), 1e-6);
}

[numthreads(1, 1, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    uint totalPixels = 0;
    for (uint i = 0; i < 64; i++)
        totalPixels += g_histogram[i];

    float result = AverageLuminance(totalPixels);

    float scale = g_MiddleGray.x / max(result * g_MiddleGray.y + g_MiddleGray.z, 1e-6);

    float scale_prev = g_exposure[uint2(0, 0)];
    if (scale_prev <= 1e-6)
        scale_prev = 1.0;

    float rvalue = lerp(scale_prev, scale, saturate(g_MiddleGray.w));
    rvalue = clamp(rvalue, 1.0 / 128.0, 20.0);

    g_exposure[uint2(0, 0)] = rvalue;
}
