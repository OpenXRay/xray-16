// particle_billboard.cs
// GPU billboard generation for visible particles
// Reads visible indices, outputs vertex buffer + draw args

#define THREAD_GROUP_SIZE 64

// Must match GPUParticleData in ParticlePassSetup.h (48 bytes)
struct ParticleData {
    float3 position;    // 12 bytes
    float rotation;     //  4 bytes
    float2 size;        //  8 bytes
    uint color;         //  4 bytes
    uint materialID;    //  4 bytes
    float2 uvMin;       //  8 bytes
    float2 uvMax;       //  8 bytes
};

// Must match ParticleVertex in ParticlePassSetup.h (32 bytes)
// Padded to 32 bytes: std430 vec3 alignment=16 → struct alignment=16 → stride=ceil(28/16)*16=32
struct ParticleVertex {
    float3 position;    // 12 bytes
    uint color;         //  4 bytes
    float2 texcoord;    //  8 bytes
    uint materialID;    //  4 bytes
    uint _pad;          //  4 bytes
};

cbuffer BillboardParams : register(b0) {
    float4 g_CameraTop;     // 16 bytes (xyz + pad)
    float4 g_CameraRight;   // 16 bytes (xyz + pad)
    uint g_VisibleCount;    //  4 bytes
    uint3 g_Padding;        // 12 bytes
};

StructuredBuffer<ParticleData> g_ParticleData : register(t0);
StructuredBuffer<uint> g_VisibleIndices : register(t1);
ByteAddressBuffer g_VisibleCountBuf : register(t2);

RWStructuredBuffer<ParticleVertex> g_Vertices : register(u0);
RWByteAddressBuffer g_DrawArgs : register(u1);

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint visibleIdx = dispatchThreadID.x;

    uint actualVisibleCount = g_VisibleCountBuf.Load(0);
    if (visibleIdx >= actualVisibleCount)
        return;

    // Get original particle index
    uint particleIdx = g_VisibleIndices[visibleIdx];
    ParticleData p = g_ParticleData[particleIdx];

    // Calculate rotation vectors
    float sina, cosa;
    sincos(p.rotation, sina, cosa);

    float3 T = g_CameraTop.xyz;
    float3 R = g_CameraRight.xyz;

    float r1 = p.size.x;
    float r2 = p.size.y;

    // Rotated basis vectors
    float3 Vr = T * r1 * sina + R * r1 * cosa;
    float3 Vt = T * r2 * cosa - R * r2 * sina;

    // Corner offsets
    float3 a = Vt - Vr;   // Top-left
    float3 b = Vt + Vr;   // Top-right
    float3 c = -a;        // Bottom-right
    float3 d = -b;        // Bottom-left

    // Output vertex index (4 vertices per particle)
    uint baseVertex = visibleIdx * 4;

    // Vertex 0: Bottom-left
    g_Vertices[baseVertex + 0].position = p.position + d;
    g_Vertices[baseVertex + 0].color = p.color;
    g_Vertices[baseVertex + 0].texcoord = float2(p.uvMin.x, p.uvMax.y);
    g_Vertices[baseVertex + 0].materialID = p.materialID;

    // Vertex 1: Top-left
    g_Vertices[baseVertex + 1].position = p.position + a;
    g_Vertices[baseVertex + 1].color = p.color;
    g_Vertices[baseVertex + 1].texcoord = p.uvMin;
    g_Vertices[baseVertex + 1].materialID = p.materialID;

    // Vertex 2: Bottom-right
    g_Vertices[baseVertex + 2].position = p.position + c;
    g_Vertices[baseVertex + 2].color = p.color;
    g_Vertices[baseVertex + 2].texcoord = p.uvMax;
    g_Vertices[baseVertex + 2].materialID = p.materialID;

    // Vertex 3: Top-right
    g_Vertices[baseVertex + 3].position = p.position + b;
    g_Vertices[baseVertex + 3].color = p.color;
    g_Vertices[baseVertex + 3].texcoord = float2(p.uvMax.x, p.uvMin.y);
    g_Vertices[baseVertex + 3].materialID = p.materialID;

    // First thread updates draw args with actual GPU-computed visible count
    if (visibleIdx == 0) {
        g_DrawArgs.Store(0, actualVisibleCount * 6);
        g_DrawArgs.Store(4, 1);
        g_DrawArgs.Store(8, 0);
        g_DrawArgs.Store(12, 0);
        g_DrawArgs.Store(16, 0);
    }
}
