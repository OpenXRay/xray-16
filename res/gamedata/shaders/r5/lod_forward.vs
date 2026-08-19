#include "shared/common.h"

struct VS_INPUT
{
    float3 pos   : POSITION;
    float4 color : COLOR0;
    float2 tc0   : TEXCOORD0;
    float2 tc1   : TEXCOORD1;
    float4 af    : TEXCOORD2;
};

struct VS_OUTPUT
{
    float4 hpos  : SV_Position;
    float4 color : COLOR0;
    float2 tc0   : TEXCOORD0;
    float2 tc1   : TEXCOORD1;
    float4 af    : TEXCOORD2;
};

VS_OUTPUT main(VS_INPUT v)
{
    VS_OUTPUT o;
    o.hpos = mul(m_WVP, float4(v.pos, 1.0));
    o.color = v.color;
    o.tc0 = v.tc0;
    o.tc1 = v.tc1;
    o.af = v.af;
    return o;
}
