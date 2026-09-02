#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_mdi_common.h"

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

VS_OUTPUT_MDI main(VS_INPUT v)
{
    VS_OUTPUT_MDI o;
    o.worldPos = v.position.xyz;
    o.position = mul(m_VP, float4(v.position.xyz, 1.0));
    o.normal = normalize(v.normal.xyz * 2.0 - 1.0);
    o.tangent = normalize(v.tangent.xyz * 2.0 - 1.0);
    o.bitangent = normalize(v.binormal.xyz * 2.0 - 1.0);
    o.materialID = skinned_mdi_material(v.drawIndex);
    o.drawIndex = v.drawIndex;
    o.texcoord = v.texcoord;
    return o;
}
