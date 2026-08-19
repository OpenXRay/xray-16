#include "shared/common.h"

struct VS_INPUT
{
    float3 pos   : POSITION;
    float4 color : COLOR0;
};

struct VS_OUTPUT
{
    float4 hpos  : SV_Position;
    float4 color : COLOR0;
};

VS_OUTPUT main(VS_INPUT v)
{
    VS_OUTPUT o;
    o.hpos = mul(m_VP, float4(v.pos, 1.0));
    float fog = saturate(dot(float4(v.pos, 1.0), fog_plane));
    o.color.rgb = lerp(fog_color.rgb, v.color.rgb, fog);
    o.color.a = fog * v.color.a;
    return o;
}
