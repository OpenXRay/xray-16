// rain_billboard.cs — build rain streak quads from GPU-visible particles
#define THREAD_GROUP_SIZE 64

struct RainParticle {
    float4 headSpeed;
    float4 dirPad;
    uint2  uvFlags;
    uint2  pad;
};

// float4-only cbuffer (32 bytes) — rain color u32 passed via g_Params.w bitcast.
cbuffer RainBillboardParams : register(b0) {
    float4 g_CameraPos;
    float4 g_Params; // x=factorVisual, y=dropLength, z=dropWidth, w=rainColorU32 bits
};

StructuredBuffer<RainParticle> g_Particles : register(t0);
StructuredBuffer<uint> g_VisibleIndices : register(t1);
ByteAddressBuffer g_VisibleCountBuf : register(t2);

RWByteAddressBuffer g_Vertices : register(u0);
RWByteAddressBuffer g_DrawArgs : register(u1);

static float2 UV0[4] = { float2(0,1), float2(0,0), float2(1,1), float2(1,0) };
static float2 UV1[4] = { float2(1,0), float2(1,1), float2(0,0), float2(0,1) };

void WriteRainVertex(uint vIdx, float3 pos, uint col, float2 uv)
{
    uint b = vIdx * 24u;
    g_Vertices.Store3(b, asuint(pos));
    g_Vertices.Store(b + 12u, col);
    g_Vertices.Store2(b + 16u, asuint(uv));
}

void WriteDrawArgs(uint actualCount)
{
    uint vertexCount = actualCount * 6;
    g_DrawArgs.Store(0, vertexCount);
    g_DrawArgs.Store(4, 1u);
    g_DrawArgs.Store(8, 0u);
    g_DrawArgs.Store(12, 0u);
}

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint visIdx = dtID.x;
    uint actualCount = g_VisibleCountBuf.Load(0);

    if (visIdx == 0)
        WriteDrawArgs(actualCount);

    if (visIdx >= actualCount)
        return;

    uint pIdx = g_VisibleIndices[visIdx];
    RainParticle p = g_Particles[pIdx];

    const float factorVisual = g_Params.x;
    const float dropLen = g_Params.y * factorVisual;
    const float dropW = g_Params.z;
    const uint col = asuint(g_Params.w);

    float3 posHead = p.headSpeed.xyz;
    float3 dir = normalize(p.dirPad.xyz);
    float3 posTrail = posHead - dir * dropLen;

    float3 sC = (posHead + posTrail) * 0.5;
    float3 lineD = normalize(posHead - posTrail);

    float3 camDir = normalize(sC - g_CameraPos.xyz);
    float3 lineTop = cross(camDir, lineD);
    float ltLen = length(lineTop);
    if (ltLen < 1e-4)
        lineTop = float3(0, 0, 1);
    else
        lineTop /= ltLen;

    float2 uv[4];
    if (p.uvFlags.x == 0u) { [unroll] for (int i = 0; i < 4; ++i) uv[i] = UV0[i]; }
    else                   { [unroll] for (int j = 0; j < 4; ++j) uv[j] = UV1[j]; }

    float3 corners[4];
    corners[0] = posTrail - lineTop * dropW;
    corners[1] = posTrail + lineTop * dropW;
    corners[2] = posHead  - lineTop * dropW;
    corners[3] = posHead  + lineTop * dropW;

    uint baseV = visIdx * 6;
    WriteRainVertex(baseV + 0, corners[0], col, uv[0]);
    WriteRainVertex(baseV + 1, corners[1], col, uv[1]);
    WriteRainVertex(baseV + 2, corners[2], col, uv[2]);
    WriteRainVertex(baseV + 3, corners[1], col, uv[1]);
    WriteRainVertex(baseV + 4, corners[3], col, uv[3]);
    WriteRainVertex(baseV + 5, corners[2], col, uv[2]);
}
