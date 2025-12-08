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
    float4x4 g_ViewProj;           // View-projection matrix
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

// Output: Indirect draw arguments (one per batch)
// Using RWByteAddressBuffer because D3D11 doesn't allow structured buffers with indirect args
// Each element is DrawIndexedIndirectArguments (20 bytes = 5 uints)
RWByteAddressBuffer g_DrawArgs : register(u2);

// ═══════════════════════════════════════════════════════
//  HI-Z OCCLUSION CULLING (4-tap)
// ═══════════════════════════════════════════════════════

bool OcclusionTestSphere(float3 center, float radius)
{
    // ─────────────────────────────────────────────────────
    //  1. PROJECT SPHERE CENTER TO CLIP SPACE
    // ─────────────────────────────────────────────────────
    float4 clipPos = mul(g_ViewProj, float4(center, 1.0));

    // Safety: behind camera check
    if (clipPos.w <= 0.001)
        return true;  // Conservatively visible

    // Perspective divide -> NDC
    float3 ndc = clipPos.xyz / clipPos.w;

    // ─────────────────────────────────────────────────────
    //  2. CALCULATE SCREEN-SPACE BOUNDING BOX
    // ─────────────────────────────────────────────────────
    // Approximate screen-space size of the sphere
    // g_ViewProj[0][0] and g_ViewProj[1][1] are projection scale factors
    float projScale = max(abs(g_ViewProj[0][0]), abs(g_ViewProj[1][1]));
    float2 ndcSize = float2(radius, radius) * projScale / clipPos.w;

    // Calculate bounding box in NDC
    float2 minNDC = ndc.xy - ndcSize;
    float2 maxNDC = ndc.xy + ndcSize;

    // CONSERVATIVE: If screen-space box is completely outside after passing frustum test,
    // mark as visible anyway - precision differences between world-space frustum and
    // screen-space projection can cause edge cases. Let the rasterizer handle it.
    if (any(minNDC > 1.0) || any(maxNDC < -1.0))
        return true;  // Conservatively visible

    // ─────────────────────────────────────────────────────
    //  3. CONVERT TO UV AND SELECT MIP
    // ─────────────────────────────────────────────────────
    // Convert NDC to UV space [0, 1] and clamp to valid range
    float2 minUV = saturate(minNDC * 0.5 + 0.5);
    float2 maxUV = saturate(maxNDC * 0.5 + 0.5);

    // Flip Y (NDC Y+ is up, UV Y+ is down)
    minUV.y = 1.0 - minUV.y;
    maxUV.y = 1.0 - maxUV.y;

    // Re-sort after Y flip (minUV.y is now larger)
    float4 boxUV = float4(min(minUV, maxUV), max(minUV, maxUV)); // xy=min, zw=max

    // Calculate box size in pixels
    float boxWidth = (boxUV.z - boxUV.x) * float(g_HiZWidth);
    float boxHeight = (boxUV.w - boxUV.y) * float(g_HiZHeight);

    // Select mip where box is roughly 2x2 pixels (4 taps cover it)
    float mipLevel = floor(log2(max(1.0, max(boxWidth, boxHeight) * 0.5)));
    mipLevel = clamp(mipLevel, 0.0, float(g_HiZMipLevels - 1));

    // ─────────────────────────────────────────────────────
    //  4. SAMPLE HI-Z AT 4 CORNERS
    // ─────────────────────────────────────────────────────
    float d1 = g_HiZPyramid.SampleLevel(g_PointSampler, float2(boxUV.x, boxUV.y), mipLevel); // Min corner
    float d2 = g_HiZPyramid.SampleLevel(g_PointSampler, float2(boxUV.z, boxUV.y), mipLevel); // X-max
    float d3 = g_HiZPyramid.SampleLevel(g_PointSampler, float2(boxUV.x, boxUV.w), mipLevel); // Y-max
    float d4 = g_HiZPyramid.SampleLevel(g_PointSampler, float2(boxUV.z, boxUV.w), mipLevel); // Max corner

    // MAX = farthest depth in region (for standard Z: 0=near, 1=far)
    float hiZDepth = max(max(d1, d2), max(d3, d4));

    // ─────────────────────────────────────────────────────
    //  5. CALCULATE OBJECT'S FRONT DEPTH
    // ─────────────────────────────────────────────────────
    float3 viewDir = normalize(center - g_CameraPos);
    float3 frontPoint = center - viewDir * radius;

    float4 frontClip = mul(g_ViewProj, float4(frontPoint, 1.0));

    // If front point is behind camera, object straddles near plane - visible
    if (frontClip.w <= 0.001)
        return true;

    float objectDepth = saturate(frontClip.z / frontClip.w);

    // ─────────────────────────────────────────────────────
    //  6. DEPTH COMPARISON
    // ─────────────────────────────────────────────────────
    // Standard Z (0=near, 1=far):
    // - hiZDepth = MAX (farthest surface in region)
    // - VISIBLE if objectDepth <= hiZDepth (closer than farthest)
    // - OCCLUDED if objectDepth > hiZDepth (behind everything)
    float depthBias = 0.0001;
    return objectDepth <= (hiZDepth + depthBias);
}

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
    if (!OcclusionTestSphere(obj.position, obj.radius))
        return;

    // ─────────────────────────────────────────────────────
    //  OBJECT IS VISIBLE - Add to output
    // ─────────────────────────────────────────────────────

    // Atomically allocate slot in visible array
    uint visibleIdx;
    g_VisibleCount.InterlockedAdd(0, 1, visibleIdx);

    // Store batch index (for debug/readback)
    g_VisibleIndices[visibleIdx] = obj.batchIndex;

    // Enable indirect draw for this batch by setting instanceCount = 1
    // DrawIndexedIndirectArguments layout (20 bytes):
    //   offset 0:  indexCountPerInstance (uint)
    //   offset 4:  instanceCount (uint) <- we write this
    //   offset 8:  startIndexLocation (uint)
    //   offset 12: baseVertexLocation (int)
    //   offset 16: startInstanceLocation (uint)
    uint argsBaseOffset = obj.batchIndex * 20;  // 20 bytes per DrawArgs
    g_DrawArgs.Store(argsBaseOffset + 4, 1);    // instanceCount = 1
}
