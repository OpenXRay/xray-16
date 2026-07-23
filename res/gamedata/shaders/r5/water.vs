// water.vs — MDI port of r3/water.vs (watermove + dual nmap TC + TBN + vertex lighting)
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
    float2 tnorm0   : TEXCOORD1;
    float2 tnorm1   : TEXCOORD2;
    float3 M1       : TEXCOORD3;
    float3 M2       : TEXCOORD4;
    float3 M3       : TEXCOORD5;
    float3 v2point  : TEXCOORD6;
    float4 c0       : COLOR0;
    float  fog      : TEXCOORD7;
    nointerpolation uint materialID : TEXCOORD8;
    float4 tctexgen : TEXCOORD9;
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

float3 UnpackNormal(float4 packed)
{
    return packed.rgb * 2.0 - 1.0;
}

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT o;

    uint drawID = input.drawIndex;
    uint batchIndex = g_CompactBatchIndices[drawID];
    InstanceData instanceData = g_InstanceData[batchIndex];
    float4x4 worldMatrix = instanceData.world;
    uint materialID = g_CompactMaterialIDs[drawID];

    // Match r3 unpack: normals/color arrive as UNORM [0,1] (same as unpack_D3DCOLOR path)
    float3 N_unpacked = UnpackNormal(input.normal);
    float3 T_unpacked = UnpackNormal(input.tangent);
    float3 B_unpacked = UnpackNormal(input.binormal);

    // UnifiedVertex UVs are already unpacked (float); r3 used unpack_tc_base on int2
    float2 tbase = input.texcoord;

    float4 P = mul(worldMatrix, float4(input.position.xyz, 1.0));
    P = watermove(P);

    o.v2point = P.xyz - eye_position;
    o.tbase = tbase;
    o.tnorm0 = watermove_tc(tbase * W_DISTORT_BASE_TILE_0, P.xz, W_DISTORT_AMP_0);
    o.tnorm1 = watermove_tc(tbase * W_DISTORT_BASE_TILE_1, P.xz, W_DISTORT_AMP_1);

    // r3: xform = m_W * float3x3(columns T,B,N)
    float3 N = N_unpacked;
    float3 T = T_unpacked;
    float3 B = B_unpacked;
    float3x3 world3 = (float3x3)worldMatrix;
    float3x3 tbn = float3x3(
        T.x, B.x, N.x,
        T.y, B.y, N.y,
        T.z, B.z, N.z);
    float3x3 xform = mul(world3, tbn);
    o.M1 = xform[0];
    o.M2 = xform[1];
    o.M3 = xform[2];

    // Vertex lighting (r3/water.vs)
    float hemi = input.normal.a;          // packed hemi in alpha
    float sunOcclusion = input.color.a;
    float3 L_rgb = input.color.rgb;
    float3 L_hemi = v_hemi(N) * hemi;
    float3 L_sun = v_sun(N) * sunOcclusion;
    float3 L_final = L_rgb + L_hemi + L_sun + L_ambient.rgb;
    o.c0 = float4(L_final, 1.0);

    o.hpos = mul(m_VP, P);
    o.fog = saturate(calc_fogging(P));
    o.materialID = materialID;
    o.tctexgen = o.hpos;
    return o;
}
