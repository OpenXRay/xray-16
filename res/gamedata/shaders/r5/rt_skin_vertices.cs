ByteAddressBuffer g_SrcVB : register(t0);
StructuredBuffer<float4x4> g_BoneMatrices : register(t1);
RWByteAddressBuffer g_Output : register(u0);

cbuffer RTSkinningCB : register(b5) {
    column_major float4x4 g_WorldMatrix;
    column_major float4x4 g_NormalMatrix;
    uint g_VertexCount;
    uint g_VertexStride;
    uint g_FormatID;
    uint g_BoneOffset;
    uint g_OutputOffset;
    uint g_InputBaseVertex;
    uint2 g_Pad;
};

float4x4 get_bone(uint legacy_index)
{
    return g_BoneMatrices[g_BoneOffset + (legacy_index / 3)];
}

float3 unpack_snorm16_xyz(uint2 packed)
{
    int4 s;
    s.x = int(packed.x << 16) >> 16;
    s.y = int(packed.x) >> 16;
    s.z = int(packed.y << 16) >> 16;
    return float3(s.xyz) / 32767.0;
}

float2 unpack_snorm16_xy(uint packed)
{
    int2 s;
    s.x = int(packed << 16) >> 16;
    s.y = int(packed) >> 16;
    return float2(s) / 32767.0;
}

float4 unpack_d3dcolor(uint packed)
{
    return float4(
        (packed >> 16) & 0xFF,
        (packed >>  8) & 0xFF,
        (packed >>  0) & 0xFF,
        (packed >> 24) & 0xFF
    ) / 255.0;
}

float3 unpack_basis(uint packed)
{
    return unpack_d3dcolor(packed).rgb * 2.0 - 1.0;
}

uint pack_normal(float3 n)
{
    uint3 u = uint3(clamp(n * 127.5 + 127.5, 0, 255));
    return (u.x << 16) | (u.y << 8) | u.z;
}

[numthreads(256, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint vid = dtid.x;
    if (vid >= g_VertexCount)
        return;

    uint srcAddr = (g_InputBaseVertex + vid) * g_VertexStride;

    float4 localPos;
    float3 localNormal;
    float3 localTangent;
    float3 localBinormal;
    float2 uv;
    float4x4 bone0 = (float4x4)0;
    float4x4 bone1 = (float4x4)0;
    float4x4 bone2 = (float4x4)0;
    float4x4 bone3 = (float4x4)0;
    float w0 = 0.0;
    float w1 = 0.0;
    float w2 = 0.0;
    float w3 = 0.0;

    switch (g_FormatID)
    {
    case 0: // nonHQ/1W — 24B
    {
        uint4 posData = g_SrcVB.Load4(srcAddr);
        localPos = float4(unpack_snorm16_xyz(posData.xy) * 12.0, 1.0);
        float4 normalPacked = unpack_d3dcolor(posData.z);
        localNormal = normalPacked.rgb * 2.0 - 1.0;
        localTangent = unpack_basis(posData.w);
        uint2 basisData = g_SrcVB.Load2(srcAddr + 16);
        localBinormal = unpack_basis(basisData.x);
        uv = unpack_snorm16_xy(basisData.y);

        bone0 = get_bone(int(normalPacked.a * 255.0 + 0.3));
        w0 = 1.0;
        break;
    }

    case 1: // hq1w/1W_HQ — 36B
    {
        localPos = float4(asfloat(g_SrcVB.Load4(srcAddr)).xyz, 1.0);
        uint3 basis = g_SrcVB.Load3(srcAddr + 16);
        float4 normalPacked = unpack_d3dcolor(basis.x);
        localNormal = normalPacked.rgb * 2.0 - 1.0;
        localTangent = unpack_basis(basis.y);
        localBinormal = unpack_basis(basis.z);
        uv = asfloat(g_SrcVB.Load2(srcAddr + 28));

        bone0 = get_bone(int(normalPacked.a * 255.0 + 0.3));
        w0 = 1.0;
        break;
    }

    case 2: // hq2w/2W — 44B
    {
        localPos = float4(asfloat(g_SrcVB.Load4(srcAddr)).xyz, 1.0);
        uint3 basis = g_SrcVB.Load3(srcAddr + 16);
        float4 normalPacked = unpack_d3dcolor(basis.x);
        localNormal = normalPacked.rgb * 2.0 - 1.0;
        localTangent = unpack_basis(basis.y);
        localBinormal = unpack_basis(basis.z);

        float4 tcData = asfloat(g_SrcVB.Load4(srcAddr + 28));
        uv = tcData.xy;

        bone0 = get_bone(int(tcData.z));
        bone1 = get_bone(int(tcData.w));
        w0 = normalPacked.a;
        w1 = 1.0 - w0;
        break;
    }

    case 3: // hq3w/3W — 44B
    {
        localPos = float4(asfloat(g_SrcVB.Load4(srcAddr)).xyz, 1.0);
        uint3 basis = g_SrcVB.Load3(srcAddr + 16);
        float4 normalPacked = unpack_d3dcolor(basis.x);
        float4 tangentPacked = unpack_d3dcolor(basis.y);
        float4 binormalPacked = unpack_d3dcolor(basis.z);
        localNormal = normalPacked.rgb * 2.0 - 1.0;
        localTangent = tangentPacked.rgb * 2.0 - 1.0;
        localBinormal = binormalPacked.rgb * 2.0 - 1.0;

        float4 tcData = asfloat(g_SrcVB.Load4(srcAddr + 28));
        uv = tcData.xy;

        bone0 = get_bone(int(tcData.z));
        bone1 = get_bone(int(tcData.w));
        bone2 = get_bone(int(binormalPacked.a * 255.0 + 0.3));
        w0 = normalPacked.a;
        w1 = tangentPacked.a;
        w2 = 1.0 - w0 - w1;
        break;
    }

    case 4: // hq4w/4W — 40B
    {
        localPos = float4(asfloat(g_SrcVB.Load4(srcAddr)).xyz, 1.0);
        uint3 basis = g_SrcVB.Load3(srcAddr + 16);
        float4 normalPacked = unpack_d3dcolor(basis.x);
        float4 tangentPacked = unpack_d3dcolor(basis.y);
        float4 binormalPacked = unpack_d3dcolor(basis.z);
        localNormal = normalPacked.rgb * 2.0 - 1.0;
        localTangent = tangentPacked.rgb * 2.0 - 1.0;
        localBinormal = binormalPacked.rgb * 2.0 - 1.0;

        uv = asfloat(g_SrcVB.Load2(srcAddr + 28));
        float4 blendIndices = unpack_d3dcolor(g_SrcVB.Load(srcAddr + 36));

        bone0 = get_bone(int(blendIndices.r * 255.0 + 0.3));
        bone1 = get_bone(int(blendIndices.g * 255.0 + 0.3));
        bone2 = get_bone(int(blendIndices.b * 255.0 + 0.3));
        bone3 = get_bone(int(blendIndices.a * 255.0 + 0.3));
        w0 = normalPacked.a;
        w1 = tangentPacked.a;
        w2 = binormalPacked.a;
        w3 = 1.0 - w0 - w1 - w2;
        break;
    }

    default:
        return;
    }

    float4 skinnedPos = mul(bone0, localPos) * w0 + mul(bone1, localPos) * w1
        + mul(bone2, localPos) * w2 + mul(bone3, localPos) * w3;
    float3 skinnedN = mul((float3x3)bone0, localNormal) * w0 + mul((float3x3)bone1, localNormal) * w1
        + mul((float3x3)bone2, localNormal) * w2 + mul((float3x3)bone3, localNormal) * w3;
    float3 skinnedT = mul((float3x3)bone0, localTangent) * w0 + mul((float3x3)bone1, localTangent) * w1
        + mul((float3x3)bone2, localTangent) * w2 + mul((float3x3)bone3, localTangent) * w3;
    float3 skinnedB = mul((float3x3)bone0, localBinormal) * w0 + mul((float3x3)bone1, localBinormal) * w1
        + mul((float3x3)bone2, localBinormal) * w2 + mul((float3x3)bone3, localBinormal) * w3;

    float4 worldPos = mul(g_WorldMatrix, skinnedPos);
    float3 worldN = normalize(mul((float3x3)g_NormalMatrix, skinnedN));
    float3x3 worldRot = (float3x3)g_WorldMatrix;
    float3 worldT = mul(worldRot, skinnedT);
    float3 worldB = mul(worldRot, skinnedB);
    worldT = dot(worldT, worldT) > 1e-8 ? normalize(worldT) : float3(0, 0, 0);
    worldB = dot(worldB, worldB) > 1e-8 ? normalize(worldB) : float3(0, 0, 0);

    uint outAddr = (g_OutputOffset + vid) * 32;
    g_Output.Store3(outAddr, asuint(worldPos.xyz));
    g_Output.Store(outAddr + 12, pack_normal(worldN));
    g_Output.Store2(outAddr + 16, asuint(uv));
    g_Output.Store(outAddr + 24, pack_normal(worldT));
    g_Output.Store(outAddr + 28, pack_normal(worldB));
}
