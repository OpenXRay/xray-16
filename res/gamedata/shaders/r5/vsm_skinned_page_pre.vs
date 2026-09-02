#define SM_6_0
#include "common.h"
#include "vsm_common.h"
#include "vsm_params.h"
#include "vsm_skinned_route.h"

struct VS_INPUT
{
    float4 position  : POSITION;
    float4 normal    : NORMAL;
    float4 tangent   : TANGENT;
    float4 binormal  : BINORMAL;
    float2 texcoord  : TEXCOORD0;
    float2 texcoord1 : TEXCOORD1;
    float4 color     : COLOR0;
    uint drawIndex   : DRAWINDEX;
};

VS_OUTPUT main(VS_INPUT v)
{
    return VsmSkinnedOut(v.drawIndex, v.position.xyz, v.texcoord);
}
