#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "shared\waterconfig.h"
#include "shared\watermove.h"

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

struct VS_OUTPUT
{
    float4 hpos     : SV_Position;
    float2 tbase    : TEXCOORD0;
    float2 tdist0   : TEXCOORD1;
    float2 tdist1   : TEXCOORD2;
    float3 worldPos : TEXCOORD3;
    nointerpolation uint materialID : TEXCOORD4;
    float4 tctexgen : TEXCOORD5;
};

struct InstanceData
{
    float4x4 world;
    uint materialID;
    uint flags;
    float pad0, pad1;
};

StructuredBuffer<InstanceData> g_InstanceData : register(t14);
StructuredBuffer<uint> g_CompactBatchIndices : register(t15);
StructuredBuffer<uint> g_CompactMaterialIDs : register(t16);

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT o;

    uint drawID = input.drawIndex;
    uint batchIndex = g_CompactBatchIndices[drawID];
    InstanceData instanceData = g_InstanceData[batchIndex];
    uint materialID = g_CompactMaterialIDs[drawID];

    float2 tbase = input.texcoord;
    float4 P = mul(instanceData.world, float4(input.position.xyz, 1.0));
    P = watermove(P);

    o.tbase = tbase;
    o.tdist0 = watermove_tc(tbase * W_DISTORT_BASE_TILE_0, P.xz, W_DISTORT_AMP_0);
    o.tdist1 = watermove_tc(tbase * W_DISTORT_BASE_TILE_1, P.xz, W_DISTORT_AMP_1);
    o.worldPos = P.xyz;
    o.hpos = mul(m_VP, P);
    o.materialID = materialID;
    o.tctexgen = o.hpos;
    float3 Pe = mul(m_V, P).xyz;
    o.tctexgen.z = Pe.z;
    return o;
}
