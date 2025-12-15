// object_cull.cs - GPU Frustum + Occlusion Culling for World Geometry
// Uses Hi-Z pyramid for conservative occlusion testing
//
// Input:  Array of object bounding spheres
// Output: Buffer of visible object indices + indirect draw arguments
//
// Culling order (cheap to expensive):
// 1. Distance culling (reject objects beyond max distance)
// 2. Frustum culling (reject objects outside view frustum)
// 3. Hi-Z occlusion culling (reject objects behind opaque geometry)
//
#define SM_5_0
#include "common.h"
#include "cull_utils.h"

// ═══════════════════════════════════════════════════════
//  DATA STRUCTURES
// ═══════════════════════════════════════════════════════

// GPU object data (matches C++ GPUObjectData struct)
struct GPUObjectData
{
    float3 position;    // World-space bounding sphere center
    float radius;       // Bounding sphere radius
    uint batchIndex;    // Index into original batch array
    uint flags;         // Object flags (opaque=0x1, alpha-test=0x2, transparent=0x4)
    float2 padding;     // Padding to 32 bytes
};

// Indirect draw arguments (D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS)
struct IndirectDrawArgs
{
    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation;
    int baseVertexLocation;
    uint startInstanceLocation;
};

// ═══════════════════════════════════════════════════════
//  CONSTANTS
// ═══════════════════════════════════════════════════════

cbuffer CullParams : register(b5)  // b5 to avoid conflicts with common.h
{
    float4x4 g_ViewProj;           // Current frame view-projection (for frustum culling)
    float4x4 g_PrevViewProj;       // Previous frame view-projection (for Hi-Z sampling)
    float3 g_CameraPos;            // Camera world position
    float g_MaxDistance;           // Maximum render distance (squared)
    float4 g_FrustumPlanes[6];     // View frustum planes (world space)
    uint g_ObjectCount;            // Total objects to cull
    uint g_HiZWidth;               // Hi-Z pyramid base width
    uint g_HiZHeight;              // Hi-Z pyramid base height
    uint g_HiZMipLevels;           // Number of Hi-Z mip levels
};

// ═══════════════════════════════════════════════════════
//  RESOURCES
// ═══════════════════════════════════════════════════════

// Input: All objects to cull
StructuredBuffer<GPUObjectData> g_Objects : register(t0);

// Input: Hi-Z pyramid from depth prepass
Texture2D<float> g_HiZPyramid : register(t1);

// Sampler for Hi-Z pyramid
SamplerState g_PointSampler : register(s0);

// Output: Indices of visible objects (for debug/readback)
RWStructuredBuffer<uint> g_VisibleIndices : register(u0);

// Output: Atomic counter for visible object count
RWByteAddressBuffer g_VisibleCount : register(u1);

// Output: Visibility buffer (1 uint per object: 0=culled, 1=visible)
// Compaction shader reads this + static draw args to create compact buffer
// Avoids CPU re-uploading draw args every frame
RWStructuredBuffer<uint> g_Visibility : register(u2);

// ═══════════════════════════════════════════════════════
//  MAIN COMPUTE SHADER
// ═══════════════════════════════════════════════════════

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint objectIdx = dtID.x;

    // Bounds check
    if (objectIdx >= g_ObjectCount)
        return;

    // Load object data
    GPUObjectData obj = g_Objects[objectIdx];

    // ─────────────────────────────────────────────────────
    //  SKIP TRANSPARENT GEOMETRY (requires separate pass with blending)
    // ─────────────────────────────────────────────────────
    // Transparent objects need:
    // 1. Back-to-front sorting
    // 2. Alpha blending (not available in single-PSO multi-draw)
    // 3. Depth write disabled
    // For now, let the legacy renderer handle them
    if (obj.flags & 0x4)  // GPU_OBJECT_TRANSPARENT = 0x4
        return;

    // ─────────────────────────────────────────────────────
    //  CULLING TESTS (ordered by cost: cheapest first)
    // ─────────────────────────────────────────────────────

    // 1. Distance culling (cheapest - just a dot product)
    if (!DistanceTestSphere(obj.position, obj.radius, g_CameraPos, g_MaxDistance))
        return;

    // 2. Frustum culling (cheap - 5 plane tests, skip near)
    if (!FrustumTestSphere(obj.position, obj.radius, g_FrustumPlanes))
        return;

    // 3. Hi-Z occlusion culling (expensive - texture sample + math)
    // Note: This test is conservative - may mark occluded objects as visible
    // but will never mark visible objects as occluded
    // TEMPORAL HI-Z: Use prevViewProj for Hi-Z lookup since pyramid was built from previous frame's depth
    if (!HiZTestSphereTemporal(obj.position, obj.radius, g_CameraPos, g_ViewProj, g_PrevViewProj,
                       g_HiZPyramid, g_PointSampler, g_HiZWidth, g_HiZHeight, g_HiZMipLevels))
        return;

    // ─────────────────────────────────────────────────────
    //  OBJECT IS VISIBLE - Add to output
    // ─────────────────────────────────────────────────────

    // Atomically allocate slot in visible array
    uint visibleIdx;
    g_VisibleCount.InterlockedAdd(0, 1, visibleIdx);

    // Store batch index (for debug/readback)
    g_VisibleIndices[visibleIdx] = obj.batchIndex;

    // Mark this object as visible in the visibility buffer
    // Compaction shader will read this + static draw args to create compact buffer
    // This avoids CPU re-uploading draw args every frame!
    g_Visibility[obj.batchIndex] = 1;
}
