Texture2D<float4> t_source : register(t0);
RWTexture2D<float4> u_output : register(u0);

cbuffer DebugPreviewParams : register(b5)
{
    uint2 g_outputSize;
    uint2 g_sourceSize;
    uint g_mode;
    uint g_mipLevel;
    float g_projA;
    float g_projB;
};

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (any(dtid.xy >= g_outputSize))
        return;

    float2 uv = (float2(dtid.xy) + 0.5) / float2(g_outputSize);
    uint2 srcCoord = uint2(uv * float2(g_sourceSize));
    srcCoord = min(srcCoord, g_sourceSize - 1);

    float4 color = t_source.Load(int3(srcCoord, g_mipLevel));

    if (g_mode == 5)
    {
        float d = color.r;
        float z = g_projB / max(d - g_projA, 1e-7);
        float zn = g_projB / max(1.0 - g_projA, 1e-7);
        float zf = min(g_projB / max(-g_projA, 1e-7), 4096.0);
        float t = saturate(log2(max(z / zn, 1.0)) / log2(max(zf / zn, 2.0)));
        float v = d > 0.0 ? 1.0 - t : 0.0;
        color = float4(v, v, v, 1.0);
    }
    else if (g_mode == 1)
        color = float4(color.r, color.r, color.r, 1.0);
    else if (g_mode == 2)
        color = float4(color.g, color.g, color.g, 1.0);
    else if (g_mode == 3)
        color = float4(color.b, color.b, color.b, 1.0);
    else if (g_mode == 4)
        color = float4(color.a, color.a, color.a, 1.0);
    else
        color.a = 1.0;

    u_output[dtid.xy] = color;
}
