#define SM_5_0
#include "common.h"
#include "bindless_common.h"

struct VS_INPUT
{
    float4 position  : POSITION;
    float4 normal    : NORMAL;
    float4 tangent   : TANGENT;
    float4 binormal  : BINORMAL;
    float2 texcoord  : TEXCOORD0;
    float2 texcoord1 : TEXCOORD1;
    float4 color     : COLOR0;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float3 worldPos : TEXCOORD0;
    float2 texcoord : TEXCOORD1;
    float3 normal   : TEXCOORD2;
    float3 tangent  : TEXCOORD3;
    float3 bitangent: TEXCOORD4;
    nointerpolation uint materialID : TEXCOORD5;
};

cbuffer PerDrawConstants : register(b5)
{
    float4x4 g_World;
    uint g_MaterialID;
    float3 g_Padding;
};

float3 UnpackNormal(float4 packed)
{
    return packed.rgb * 2.0 - 1.0;
}

VS_OUTPUT main(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;

    float3 normalUnpacked = UnpackNormal(input.normal);
    float3 tangentUnpacked = UnpackNormal(input.tangent);
    float3 binormalUnpacked = UnpackNormal(input.binormal);

    float4 worldPos = mul(g_World, float4(input.position.xyz, 1.0));
    output.worldPos = worldPos.xyz;
    output.position = mul(m_VP, worldPos);

    float3x3 worldMatrix3x3 = (float3x3)g_World;
    output.normal = normalize(mul(worldMatrix3x3, normalUnpacked));
    output.tangent = normalize(mul(worldMatrix3x3, tangentUnpacked));
    output.bitangent = normalize(mul(worldMatrix3x3, binormalUnpacked));

    output.texcoord = input.texcoord;

    output.materialID = g_MaterialID;

    return output;
}
