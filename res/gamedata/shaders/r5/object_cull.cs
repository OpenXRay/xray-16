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
//  DISTANCE CULLING
// ═══════════════════════════════════════════════════════

bool DistanceTest(float3 position, float radius)
{
    // Calculate squared distance to object center
    float3 delta = position - g_CameraPos;
    float distSq = dot(delta, delta);

    // Account for object radius (closest point could be closer)
    float effectiveDistSq = distSq - (radius * radius);
    effectiveDistSq = max(0.0, effectiveDistSq);

    // Test against max distance (g_MaxDistance is already squared)
    return effectiveDistSq <= g_MaxDistance;
}

// ═══════════════════════════════════════════════════════
//  FRUSTUM CULLING
// ═══════════════════════════════════════════════════════

bool FrustumTestSphere(float3 center, float radius)
{
    // Test sphere against all 6 frustum planes
    // Plane equation: dot(normal, point) + d = 0
    // Positive = outside, Negative = inside
    // Sphere outside if distance > radius for ANY plane

    for (uint i = 0; i < 6; i++)
    {
        // Signed distance from sphere center to plane
        float dist = dot(g_FrustumPlanes[i].xyz, center) + g_FrustumPlanes[i].w;

        // If sphere is completely outside this plane, cull it
        if (dist > radius)
            return false;
    }

    // Sphere is inside or intersecting all planes = potentially visible
    return true;
}

// ═══════════════════════════════════════════════════════
//  HI-Z OCCLUSION CULLING
// ═══════════════════════════════════════════════════════

bool OcclusionTestSphere(float3 center, float radius)
{
    // DEBUG: Disable Hi-Z occlusion test - always return visible
    // Remove this line once culling is debugged
    return true;

    // ─────────────────────────────────────────────────────
    //  PROJECT SPHERE TO SCREEN SPACE
    // ─────────────────────────────────────────────────────

    // Transform center to clip space
    float4 clipPos = mul(g_ViewProj, float4(center, 1.0));

    // Behind camera check (w <= 0 means behind or at camera plane)
    if (clipPos.w <= 0.0)
        return true;  // Behind camera - conservatively visible

    // Perspective divide -> NDC
    float3 ndc = clipPos.xyz / clipPos.w;

    // Check if center is outside NDC bounds [-1, 1]
    // If outside, conservatively mark as visible (partial visibility)
    if (any(abs(ndc.xy) > 1.0 + radius / clipPos.w))
        return true;

    // Convert NDC to UV space [0, 1]
    float2 uv = ndc.xy * 0.5 + 0.5;
    uv.y = 1.0 - uv.y;  // Flip Y (NDC Y+ is up, UV Y+ is down)

    // ─────────────────────────────────────────────────────
    //  CALCULATE SCREEN-SPACE RADIUS FOR MIP SELECTION
    // ─────────────────────────────────────────────────────

    // Approximate screen-space radius in pixels
    // This is used to select the appropriate Hi-Z mip level
    float screenRadius = (radius / clipPos.w) * float(g_HiZWidth) * 0.5;

    // Select mip level based on object's screen coverage
    // Larger objects need coarser mip (more conservative)
    // log2(diameter) gives us a reasonable mip level
    float mipLevel = log2(max(1.0, screenRadius * 2.0));
    mipLevel = clamp(mipLevel, 0.0, float(g_HiZMipLevels - 1));

    // ─────────────────────────────────────────────────────
    //  SAMPLE HI-Z AND TEST DEPTH
    // ─────────────────────────────────────────────────────

    // Sample Hi-Z pyramid at calculated mip level
    float hiZDepth = g_HiZPyramid.SampleLevel(g_PointSampler, uv, mipLevel);

    // Calculate object's closest depth (front of bounding sphere)
    // We need to test if the closest point of the sphere is occluded
    //
    // Conservative approach: offset center toward camera by radius
    float3 viewDir = normalize(center - g_CameraPos);
    float3 frontPoint = center - viewDir * radius;

    float4 frontClip = mul(g_ViewProj, float4(frontPoint, 1.0));
    float objectDepth = frontClip.z / frontClip.w;

    // Clamp to valid depth range
    objectDepth = saturate(objectDepth);

    // ─────────────────────────────────────────────────────
    //  DEPTH COMPARISON
    // ─────────────────────────────────────────────────────
    //
    // For standard Z-buffer (0=near, 1=far):
    // - Object is visible if its depth <= Hi-Z depth
    // - Hi-Z contains MAX depth of region = farthest surface
    // - If object is closer than farthest surface, it might be visible
    //
    // Add small bias to avoid z-fighting and floating point precision issues
    float depthBias = 0.001;

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
    //  CULLING TESTS (ordered by cost: cheapest first)
    // ─────────────────────────────────────────────────────

    // DEBUG: Disable ALL culling to test if indirect draw works
    // If geometry still doesn't render, the problem is in the draw args buffer
    // or the indirect draw call itself
    #if 0
    // 1. Distance culling (cheapest - just a dot product)
    if (!DistanceTest(obj.position, obj.radius))
        return;

    // 2. Frustum culling (cheap - 6 plane tests)
    if (!FrustumTestSphere(obj.position, obj.radius))
        return;

    // 3. Hi-Z occlusion culling (expensive - texture sample + math)
    // Note: This test is conservative - may mark occluded objects as visible
    // but will never mark visible objects as occluded
    if (!OcclusionTestSphere(obj.position, obj.radius))
        return;
    #endif

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
