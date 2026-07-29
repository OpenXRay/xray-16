#include "shared/common.h"
#include "shared/cloudconfig.h"

struct VS_INPUT
{
    float3 position : POSITION;
    float4 dir      : COLOR0;
    float4 color    : COLOR1;
};

struct VS_OUTPUT
{
    float4 hpos  : SV_Position;
    float4 color : COLOR0;
    float2 tc0   : TEXCOORD0;
    float2 tc1   : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT v)
{
    VS_OUTPUT o;

    float4 p = float4(v.position, 1.0);
    o.hpos = mul(m_WVP, p);
    o.hpos.z = o.hpos.w * 0.0001;

    float2 d0 = v.dir.xy * 2.0 - 1.0;
    float2 d1 = v.dir.wz * 2.0 - 1.0;
    o.tc0 = v.position.xz * CLOUD_TILE0 + d0 * timers.z * CLOUD_SPEED0;
    o.tc1 = v.position.xz * CLOUD_TILE1 + d1 * timers.z * CLOUD_SPEED1;

    o.color = v.color;
    o.color.w *= pow(max(v.position.y, 0.0), 25.0);

    return o;
}
