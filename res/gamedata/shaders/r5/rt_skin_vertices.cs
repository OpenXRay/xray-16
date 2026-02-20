ByteAddressBuffer g_SrcVB : register(t0);
StructuredBuffer<float4x4> g_BoneMatrices : register(t1);
RWByteAddressBuffer g_Output : register(u0);

cbuffer RTSkinningCB : register(b5) {
    column_major float4x4 g_WorldMatrix;
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
    float2 uv;

    switch (g_FormatID)
    {
    case 0: // nonHQ/1W — 24B
    {
        uint4 posData = g_SrcVB.Load4(srcAddr);
        localPos = float4(unpack_snorm16_xyz(posData.xy), 1.0);
        float4 normalPacked = unpack_d3dcolor(posData.z);
        localNormal = normalPacked.rgb * 2.0 - 1.0;
        uint boneIdx = int(normalPacked.a * 255.0 + 0.3);
        float4x4 bone = get_bone(boneIdx);
        float4 skinnedPos = mul(bone, localPos);
        float3 skinnedN = mul((float3x3)bone, localNormal);

        uint uvPacked = g_SrcVB.Load(srcAddr + 20);
        uv = unpack_snorm16_xy(uvPacked);

        float4 worldPos = mul(g_WorldMatrix, skinnedPos);
        float3 worldN = normalize(mul((float3x3)g_WorldMatrix, skinnedN));

        uint outAddr = (g_OutputOffset + vid) * 24;
        g_Output.Store3(outAddr, asuint(worldPos.xyz));
        g_Output.Store(outAddr + 12, pack_normal(worldN));
        g_Output.Store2(outAddr + 16, asuint(uv));
        return;
    }

    case 1: // hq1w/1W_HQ — 36B
    {
        float4 pos = asfloat(g_SrcVB.Load4(srcAddr));
        localPos = float4(pos.xyz, 1.0);
        float4 normalPacked = unpack_d3dcolor(g_SrcVB.Load(srcAddr + 16));
        localNormal = normalPacked.rgb * 2.0 - 1.0;
        uint boneIdx = int(normalPacked.a * 255.0 + 0.3);
        float4x4 bone = get_bone(boneIdx);
        float4 skinnedPos = mul(bone, localPos);
        float3 skinnedN = mul((float3x3)bone, localNormal);
        uv = asfloat(g_SrcVB.Load2(srcAddr + 28));

        float4 worldPos = mul(g_WorldMatrix, skinnedPos);
        float3 worldN = normalize(mul((float3x3)g_WorldMatrix, skinnedN));

        uint outAddr = (g_OutputOffset + vid) * 24;
        g_Output.Store3(outAddr, asuint(worldPos.xyz));
        g_Output.Store(outAddr + 12, pack_normal(worldN));
        g_Output.Store2(outAddr + 16, asuint(uv));
        return;
    }

    case 2: // hq2w/2W — 44B
    {
        float4 pos = asfloat(g_SrcVB.Load4(srcAddr));
        localPos = float4(pos.xyz, 1.0);
        float4 normalPacked = unpack_d3dcolor(g_SrcVB.Load(srcAddr + 16));
        localNormal = normalPacked.rgb * 2.0 - 1.0;
        float w0 = normalPacked.a;

        float4 tcData = asfloat(g_SrcVB.Load4(srcAddr + 28));
        uv = tcData.xy;
        uint boneIdx0 = int(tcData.z);
        uint boneIdx1 = int(tcData.w);

        float4x4 bone0 = get_bone(boneIdx0);
        float4x4 bone1 = get_bone(boneIdx1);
        float4 skinnedPos = mul(bone0, localPos) * w0 + mul(bone1, localPos) * (1.0 - w0);
        float3 skinnedN = mul((float3x3)bone0, localNormal) * w0 + mul((float3x3)bone1, localNormal) * (1.0 - w0);

        float4 worldPos = mul(g_WorldMatrix, skinnedPos);
        float3 worldN = normalize(mul((float3x3)g_WorldMatrix, skinnedN));

        uint outAddr = (g_OutputOffset + vid) * 24;
        g_Output.Store3(outAddr, asuint(worldPos.xyz));
        g_Output.Store(outAddr + 12, pack_normal(worldN));
        g_Output.Store2(outAddr + 16, asuint(uv));
        return;
    }

    case 3: // hq3w/3W — 44B
    {
        float4 pos = asfloat(g_SrcVB.Load4(srcAddr));
        localPos = float4(pos.xyz, 1.0);
        float4 normalPacked = unpack_d3dcolor(g_SrcVB.Load(srcAddr + 16));
        localNormal = normalPacked.rgb * 2.0 - 1.0;
        float w0 = normalPacked.a;
        float4 tangentPacked = unpack_d3dcolor(g_SrcVB.Load(srcAddr + 20));
        float w1 = tangentPacked.a;
        float w2 = 1.0 - w0 - w1;

        float4 tcData = asfloat(g_SrcVB.Load4(srcAddr + 28));
        uv = tcData.xy;
        uint boneIdx0 = int(tcData.z);
        uint boneIdx1 = int(tcData.w);
        float4 binormalPacked = unpack_d3dcolor(g_SrcVB.Load(srcAddr + 24));
        uint boneIdx2 = int(binormalPacked.a * 255.0 + 0.3);

        float4x4 bone0 = get_bone(boneIdx0);
        float4x4 bone1 = get_bone(boneIdx1);
        float4x4 bone2 = get_bone(boneIdx2);
        float4 skinnedPos = mul(bone0, localPos) * w0 + mul(bone1, localPos) * w1 + mul(bone2, localPos) * w2;
        float3 skinnedN = mul((float3x3)bone0, localNormal) * w0 + mul((float3x3)bone1, localNormal) * w1 + mul((float3x3)bone2, localNormal) * w2;

        float4 worldPos = mul(g_WorldMatrix, skinnedPos);
        float3 worldN = normalize(mul((float3x3)g_WorldMatrix, skinnedN));

        uint outAddr = (g_OutputOffset + vid) * 24;
        g_Output.Store3(outAddr, asuint(worldPos.xyz));
        g_Output.Store(outAddr + 12, pack_normal(worldN));
        g_Output.Store2(outAddr + 16, asuint(uv));
        return;
    }

    case 4: // hq4w/4W — 40B
    {
        float4 pos = asfloat(g_SrcVB.Load4(srcAddr));
        localPos = float4(pos.xyz, 1.0);
        float4 normalPacked = unpack_d3dcolor(g_SrcVB.Load(srcAddr + 16));
        localNormal = normalPacked.rgb * 2.0 - 1.0;
        float w0 = normalPacked.a;
        float4 tangentPacked = unpack_d3dcolor(g_SrcVB.Load(srcAddr + 20));
        float w1 = tangentPacked.a;
        float4 binormalPacked = unpack_d3dcolor(g_SrcVB.Load(srcAddr + 24));
        float w2 = binormalPacked.a;
        float w3 = 1.0 - w0 - w1 - w2;

        uv = asfloat(g_SrcVB.Load2(srcAddr + 28));
        float4 blendIndices = unpack_d3dcolor(g_SrcVB.Load(srcAddr + 36));
        uint boneIdx0 = int(blendIndices.r * 255.0 + 0.3);
        uint boneIdx1 = int(blendIndices.g * 255.0 + 0.3);
        uint boneIdx2 = int(blendIndices.b * 255.0 + 0.3);
        uint boneIdx3 = int(blendIndices.a * 255.0 + 0.3);

        float4x4 bone0 = get_bone(boneIdx0);
        float4x4 bone1 = get_bone(boneIdx1);
        float4x4 bone2 = get_bone(boneIdx2);
        float4x4 bone3 = get_bone(boneIdx3);
        float4 skinnedPos = mul(bone0, localPos) * w0 + mul(bone1, localPos) * w1 + mul(bone2, localPos) * w2 + mul(bone3, localPos) * w3;
        float3 skinnedN = mul((float3x3)bone0, localNormal) * w0 + mul((float3x3)bone1, localNormal) * w1 + mul((float3x3)bone2, localNormal) * w2 + mul((float3x3)bone3, localNormal) * w3;

        float4 worldPos = mul(g_WorldMatrix, skinnedPos);
        float3 worldN = normalize(mul((float3x3)g_WorldMatrix, skinnedN));

        uint outAddr = (g_OutputOffset + vid) * 24;
        g_Output.Store3(outAddr, asuint(worldPos.xyz));
        g_Output.Store(outAddr + 12, pack_normal(worldN));
        g_Output.Store2(outAddr + 16, asuint(uv));
        return;
    }

    default:
        break;
    }
}
