#include "shared/common.h"

struct VS_INPUT
{
    float3 pos0   : POSITION0;
    float3 pos1   : POSITION1;
    float3 n0     : NORMAL0;
    float3 n1     : NORMAL1;
    float4 sun_af : COLOR0;
    float4 rgbh0  : COLOR1;
    float4 rgbh1  : COLOR2;
    float2 tc0    : TEXCOORD0;
    float2 tc1    : TEXCOORD1;
};

struct VS_OUTPUT
{
    float4 hpos     : SV_Position;
    float3 worldPos : TEXCOORD0;
    float2 tc0      : TEXCOORD1;
    float2 tc1      : TEXCOORD2;
    float4 af       : COLOR0;
    float3 normal   : TEXCOORD3;
};

#define L_SCALE (2.0 * 1.55)

VS_OUTPUT main(VS_INPUT I)
{
    VS_OUTPUT o;
    float factor = I.sun_af.w;
    float4 pos = float4(lerp(I.pos0, I.pos1, factor), 1.0);
    float h = lerp(I.rgbh0.w, I.rgbh1.w, factor) * L_SCALE;

    o.hpos = mul(m_VP, pos);
    o.worldPos = pos.xyz;
    o.tc0 = I.tc0;
    o.tc1 = I.tc1;
    o.af = float4(h, h, I.sun_af.z, factor);
    o.normal = normalize(lerp(I.n0, I.n1, factor));
    return o;
}
