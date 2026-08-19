cbuffer BloomParams : register(b5)
{
    float2 g_SrcSize;
    float2 g_DstSize;
    float g_Threshold;
    float g_Intensity;
    float2 g_Dir;
};

Texture2D<float4> t_In : register(t0);
RWTexture2D<float4> u_Out : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint2 pixel = id.xy;
    if (pixel.x >= (uint)g_DstSize.x || pixel.y >= (uint)g_DstSize.y)
        return;

    const float w[8] = {
        0.1346, 0.1273, 0.1078, 0.0816,
        0.0553, 0.0335, 0.0182, 0.0088
    };

    float step = max(g_Intensity, 0.5);
    int2 maxP = int2(g_DstSize) - 1;
    float4 c = t_In.Load(int3(pixel, 0)) * w[0];
    [unroll]
    for (int i = 1; i < 8; ++i) {
        int2 o = int2(g_Dir * (float(i) * step) + 0.5);
        o = max(o, int2(g_Dir));
        int2 p0 = clamp(int2(pixel) + o, int2(0, 0), maxP);
        int2 p1 = clamp(int2(pixel) - o, int2(0, 0), maxP);
        c += t_In.Load(int3(p0, 0)) * w[i];
        c += t_In.Load(int3(p1, 0)) * w[i];
    }
    u_Out[pixel] = c;
}
