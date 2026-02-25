#define SM_6_0
#include "common.h"
#include "bindless_common.h"

struct GPUDecalData
{
    float4x4 worldToDecal;
    float4x4 decalToWorld;
    uint materialID;
    float opacity;
    float normalThreshold;
    uint pad;
};

StructuredBuffer<GPUDecalData> g_Decals : register(t3);

struct VS_INPUT
{
    float3 position : POSITION;
    uint instanceID : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float4 screenPos : TEXCOORD0;
    nointerpolation uint instanceID : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT input)
{
    GPUDecalData decal = g_Decals[input.instanceID];
    float4 worldPos = mul(decal.decalToWorld, float4(input.position, 1.0));

    VS_OUTPUT o;
    o.position = mul(m_VP, worldPos);
    o.screenPos = o.position;
    o.instanceID = input.instanceID;
    return o;
}
