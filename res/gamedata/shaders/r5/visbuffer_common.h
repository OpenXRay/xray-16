#ifndef VISBUFFER_COMMON_H
#define VISBUFFER_COMMON_H

struct ClusterEntry
{
    float4 sphere;
    float4 lodSelf;
    float4 lodParent;
    uint indexCount;
    uint ibFirst;
    uint firstVertex;
    uint batchIndex;
    uint materialID;
    uint flags;
    float selfError;
    float parentError;
    float3 extent;
    float extentPad;
};

struct InstanceData
{
    float4x4 world;
    uint materialID;
    uint flags;
    float pad0, pad1;
};

#define CLUSTER_ENTRY_FLAG_AT 1u
#define CLUSTER_ENTRY_FLAG_PLAIN 2u
#define CLUSTER_ENTRY_FLAG_TERRAIN 4u
#define CLUSTER_ENTRY_FLAG_SHADOW_ONLY 8u
#define CLUSTER_ENTRY_FLAG_SKINNED 16u
#define CLUSTER_ENTRY_FLAG_HUD 32u
#define CLUSTER_ENTRY_FLAG_DYNAMIC 64u

#define VIS_ID_TRI_BITS 7u
#define VIS_ID_TRI_MASK 127u

uint PackVisID(uint entryIdx, uint tri)
{
    return (entryIdx << VIS_ID_TRI_BITS) | (tri & VIS_ID_TRI_MASK);
}

struct MegaVertex
{
    float3 position;
    float3 normal;
    float3 tangent;
    float3 binormal;
    float2 uv;
};

float3 UnpackD3DColorDir(uint v)
{
    return float3(float((v >> 16) & 0xFFu), float((v >> 8) & 0xFFu), float(v & 0xFFu)) / 255.0 * 2.0 - 1.0;
}

MegaVertex LoadMegaVertex(ByteAddressBuffer vb, uint vertexIndex)
{
    uint vertexByte = vertexIndex * 48u;
    uint4 w0 = vb.Load4(vertexByte);
    uint4 w1 = vb.Load4(vertexByte + 16u);
    MegaVertex v;
    v.position = float3(asfloat(w0.x), asfloat(w0.y), asfloat(w0.z));
    v.normal = UnpackD3DColorDir(w0.w);
    v.tangent = UnpackD3DColorDir(w1.x);
    v.binormal = UnpackD3DColorDir(w1.y);
    v.uv = float2(asfloat(w1.z), asfloat(w1.w));
    return v;
}

float3 LoadMegaPosition(ByteAddressBuffer vb, uint vertexIndex)
{
    uint3 w = vb.Load3(vertexIndex * 48u);
    return float3(asfloat(w.x), asfloat(w.y), asfloat(w.z));
}

struct BarycentricDeriv
{
    float3 m_lambda;
    float3 m_ddx;
    float3 m_ddy;
};

BarycentricDeriv CalcFullBary(float4 pt0, float4 pt1, float4 pt2, float2 pixelNdc, float2 winSize)
{
    BarycentricDeriv ret;
    float3 invW = rcp(float3(pt0.w, pt1.w, pt2.w));
    float2 ndc0 = pt0.xy * invW.x;
    float2 ndc1 = pt1.xy * invW.y;
    float2 ndc2 = pt2.xy * invW.z;
    float invDet = rcp(determinant(float2x2(ndc2 - ndc1, ndc0 - ndc1)));
    ret.m_ddx = float3(ndc1.y - ndc2.y, ndc2.y - ndc0.y, ndc0.y - ndc1.y) * invDet * invW;
    ret.m_ddy = float3(ndc2.x - ndc1.x, ndc0.x - ndc2.x, ndc1.x - ndc0.x) * invDet * invW;
    float ddxSum = dot(ret.m_ddx, float3(1, 1, 1));
    float ddySum = dot(ret.m_ddy, float3(1, 1, 1));
    float2 deltaVec = pixelNdc - ndc0;
    float interpInvW = invW.x + deltaVec.x * ddxSum + deltaVec.y * ddySum;
    float interpW = rcp(interpInvW);
    ret.m_lambda.x = interpW * (invW.x + deltaVec.x * ret.m_ddx.x + deltaVec.y * ret.m_ddy.x);
    ret.m_lambda.y = interpW * (deltaVec.x * ret.m_ddx.y + deltaVec.y * ret.m_ddy.y);
    ret.m_lambda.z = interpW * (deltaVec.x * ret.m_ddx.z + deltaVec.y * ret.m_ddy.z);
    ret.m_ddx *= (2.0 / winSize.x);
    ret.m_ddy *= (2.0 / winSize.y);
    ddxSum *= (2.0 / winSize.x);
    ddySum *= (2.0 / winSize.y);
    ret.m_ddy *= -1.0;
    ddySum *= -1.0;
    float interpW_ddx = 1.0 / (interpInvW + ddxSum);
    float interpW_ddy = 1.0 / (interpInvW + ddySum);
    ret.m_ddx = interpW_ddx * (ret.m_lambda * interpInvW + ret.m_ddx) - ret.m_lambda;
    ret.m_ddy = interpW_ddy * (ret.m_lambda * interpInvW + ret.m_ddy) - ret.m_lambda;
    return ret;
}

float2 InterpolateBary2(BarycentricDeriv bd, float2 v0, float2 v1, float2 v2)
{
    return bd.m_lambda.x * v0 + bd.m_lambda.y * v1 + bd.m_lambda.z * v2;
}

float2 InterpolateBaryDdx2(BarycentricDeriv bd, float2 v0, float2 v1, float2 v2)
{
    return bd.m_ddx.x * v0 + bd.m_ddx.y * v1 + bd.m_ddx.z * v2;
}

float2 InterpolateBaryDdy2(BarycentricDeriv bd, float2 v0, float2 v1, float2 v2)
{
    return bd.m_ddy.x * v0 + bd.m_ddy.y * v1 + bd.m_ddy.z * v2;
}

float3 InterpolateBary3(BarycentricDeriv bd, float3 v0, float3 v1, float3 v2)
{
    return bd.m_lambda.x * v0 + bd.m_lambda.y * v1 + bd.m_lambda.z * v2;
}

#endif
