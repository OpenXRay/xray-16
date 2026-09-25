#include "shared/color_space.h"
#include "bloom_common.h"

StructuredBuffer<float4> t_Exposure : register(t1);

static const float BLOOM_MAX_RADIANCE = 60000.0;

float3 BloomFetch(float2 uv)
{
    float3 value = t_Source.SampleLevel(smp_rtlinear, uv, 0).rgb;
    return all(isfinite(value)) ? clamp(value, 0.0, BLOOM_MAX_RADIANCE) : 0.0;
}

float BloomKarisWeight(float3 color, float exposure)
{
    return 1.0 / (1.0 + LinearLuminance(color) * exposure);
}

[numthreads(BLOOM_GROUP_SIZE, BLOOM_GROUP_SIZE, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 pixel = dispatchThreadId.xy;
    if (any(pixel >= g_TargetSize))
        return;

    float2 uv = BloomTargetUV(pixel);
    float2 texel = g_SourceTexelSize;
    float3 a = BloomFetch(uv + texel * float2(-2.0, -2.0));
    float3 b = BloomFetch(uv + texel * float2(0.0, -2.0));
    float3 c = BloomFetch(uv + texel * float2(2.0, -2.0));
    float3 d = BloomFetch(uv + texel * float2(-2.0, 0.0));
    float3 e = BloomFetch(uv);
    float3 f = BloomFetch(uv + texel * float2(2.0, 0.0));
    float3 g = BloomFetch(uv + texel * float2(-2.0, 2.0));
    float3 h = BloomFetch(uv + texel * float2(0.0, 2.0));
    float3 i = BloomFetch(uv + texel * float2(2.0, 2.0));
    float3 j = BloomFetch(uv + texel * float2(-1.0, -1.0));
    float3 k = BloomFetch(uv + texel * float2(1.0, -1.0));
    float3 l = BloomFetch(uv + texel * float2(-1.0, 1.0));
    float3 m = BloomFetch(uv + texel * float2(1.0, 1.0));

    float3 box0 = (a + b + d + e) * 0.25;
    float3 box1 = (b + c + e + f) * 0.25;
    float3 box2 = (d + e + g + h) * 0.25;
    float3 box3 = (e + f + h + i) * 0.25;
    float3 center = (j + k + l + m) * 0.25;

    float4 boxWeights = 0.125;
    float centerWeight = 0.5;
    if (g_FirstLevel != 0u)
    {
        float exposure = t_Exposure[0].x;
        exposure = isfinite(exposure) ? max(exposure, 0.0) : 1.0;
        boxWeights *= float4(
            BloomKarisWeight(box0, exposure),
            BloomKarisWeight(box1, exposure),
            BloomKarisWeight(box2, exposure),
            BloomKarisWeight(box3, exposure));
        centerWeight *= BloomKarisWeight(center, exposure);
    }

    float3 result = box0 * boxWeights.x + box1 * boxWeights.y + box2 * boxWeights.z + box3 * boxWeights.w
        + center * centerWeight;
    u_Target[pixel] = float4(result / (dot(boxWeights, (1.0).xxxx) + centerWeight), 1.0);
}
