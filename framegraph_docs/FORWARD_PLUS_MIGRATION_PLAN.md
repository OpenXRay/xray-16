# Forward+ Renderer Implementation Plan
## X-Ray Engine FrameGraph Migration

**Status:** Largely implemented in-tree (see `STATUS.md`). This document remains a design/history reference; do not treat the 24-week checklist as unfinished work.
**Timeline:** 24 weeks to full production-ready forward+ renderer with volumetrics and particles
**Target Performance:** 100-165fps @ 1920×1080 (from current ~90-110fps)
**Key Features:** PBR lighting, CSM shadows, clustered lights, froxel volumetrics, four-tier particles

---

## Executive Summary

**Current State Analysis:**
- ✅ Geometry collection and batching system working
- ✅ MaterialCache and PSO management functional
- ✅ FGConstantSystem integrated for shader constants
- ✅ FrameGraph infrastructure complete
- ❌ **No lighting implementation** (G-buffer written but never read)
- ❌ No shadow system
- ❌ No light culling
- **Current pipeline:** Pseudo-forward renderer with wasteful G-buffer writes

**Decision:** Migrate to **Pure Forward+ with Clustered Shading**

**Rationale:**
1. G-buffer currently provides zero benefit (outputs ignored)
2. Need to implement lighting from scratch anyway - do it right
3. Forward+ is simpler than deferred (fewer passes, less bandwidth)
4. Better fit for X-Ray's needs (heavy foliage, particles, modest light count)
5. Modern architecture (matches UE5, Unity HDRP, Frostbite)
6. Performance: 60% less bandwidth than current multi-RT approach

---

## Architecture Overview

### Target Forward+ Pipeline

```
┌─────────────────────────────────────────────────────────────┐
│                     FORWARD+ PIPELINE                        │
└─────────────────────────────────────────────────────────────┘

Pass 1: Depth Prepass (Z-only)              [1.5-2.0ms]
├─ Render all opaque geometry depth-only
├─ Early-Z culling for main pass
└─ Output: Depth buffer

Pass 2: Shadow Maps (CSM)                    [2.0-3.0ms]
├─ Render geometry from light POV (4 cascades)
├─ PCF filtering in forward pass
└─ Output: Shadow map atlas (4 cascades)

Pass 3: Light Culling (Compute, Async)      [0.3-0.5ms, overlapped]
├─ Build 3D cluster grid (16x16x24)
├─ Frustum culling per cluster
├─ Assign lights to clusters
└─ Output: Light index buffer + cluster data

Pass 4: Forward Shading                      [4.0-6.0ms]
├─ Render opaque geometry with lighting
├─ Read depth prepass for early-Z
├─ Sample shadow maps (CSM cascades)
├─ Read cluster grid for light culling
├─ Evaluate lighting inline (sun + point/spot lights)
└─ Output: HDR color buffer

Pass 5: Transparency (Forward, sorted)       [1.0-2.0ms]
├─ Render transparent geometry back-to-front
├─ Same lighting as opaque pass
└─ Output: Composite with opaque color

Pass 6: Volumetric Fog (Compute + Pixel)     [2.0-2.5ms]
├─ Froxel injection (multi-source: fog, particles, light shafts)
├─ Volumetric lighting (sun + clustered lights)
├─ Temporal reprojection (reuse 75% from prev frame)
├─ Ray marching (integrate volume)
└─ Output: Composite with opaque color

Pass 7: Transparency (Forward, sorted)       [1.0-2.0ms]
├─ Render transparent geometry back-to-front
├─ Same lighting as opaque pass
└─ Output: Composite with opaque + volumetrics

Pass 8: HUD (Forward)                        [0.5-1.0ms]
├─ Apply FOV adjustment
├─ Render HUD geometry
└─ Output: Composite with scene

Pass 9: Particles (Forward, multi-tier)      [0.5-1.0ms]
├─ Tier 1: Volumetric (already in Pass 6)
├─ Tier 2: Hero smoke (Unity 6-way lightmaps)
├─ Tier 3: Lit particles (clustered lighting)
├─ Tier 4: Emissive particles (no lighting)
└─ Output: Composite with scene

Pass 10: Post-Process (Optional)             [1.0-2.0ms]
├─ SSAO (depth-only, reconstructed normals)
├─ Bloom
├─ Tonemapping
└─ Output: LDR color

Pass 11: UI Composite                        [0.3-0.5ms]
├─ Composite scene + UI layers
└─ Output: Final backbuffer

──────────────────────────────────────────────────────────
Total Frame Time:  ~15-22ms = 45-65fps (before optimization)
After optimization: ~10-14ms = 70-100fps (clustered culling)
With async compute:  ~8-10ms = 100-125fps (shadow overlap)
Final target:       ~6-10ms = 100-165fps (all optimizations)
```

### Key Differences from Current Architecture

| Component | Current (Wasteful) | Forward+ (Target) |
|-----------|-------------------|-------------------|
| **Main pass RTs** | 3 RTs (albedo, normal, position) | 1 RT (color) |
| **Bandwidth** | 128 bits/pixel | 64 bits/pixel |
| **Lighting** | None (G-buffer ignored) | Inline in pixel shader |
| **Shadows** | None | CSM with PCF filtering |
| **Light culling** | None | Clustered 3D grid |
| **Transparency** | Broken | Native support |
| **MSAA** | Complex | Native support |
| **Passes** | 2 (GBuffer, HUD) | 9 (full pipeline) |

---

## Phase 1: Foundation Cleanup (Weeks 1-2)

**Goal:** Remove wasteful G-buffer writes, simplify to true forward rendering

### Task 1.1: Simplify GBufferPassSetup → ForwardColorPass

**File:** `src/Layers/xrRender/FrameGraphPasses/GBufferPassSetup.cpp`

**Changes:**
```cpp
// ═══════════════════════════════════════════════════════
//  BEFORE (Current - Wasteful 3 RT writes)
// ═══════════════════════════════════════════════════════

data.albedo = passBuilder.createTexture2D(
    "rt_Albedo",
    width, height,
    nvrhi::Format::RGBA8_UNORM  // Written but never read!
);

data.normal = passBuilder.createTexture2D(
    "rt_Normal",
    width, height,
    nvrhi::Format::RGBA16_FLOAT  // Written but never read!
);

data.position = passBuilder.createTexture2D(
    "rt_Position",
    width, height,
    nvrhi::Format::RGBA32_FLOAT  // Written but never read!
);

// ═══════════════════════════════════════════════════════
//  AFTER (Forward+ - Single RT write)
// ═══════════════════════════════════════════════════════

data.color = passBuilder.createTexture2D(
    "rt_SceneColor",
    width, height,
    nvrhi::Format::RGBA16_FLOAT  // HDR color (will add lighting later)
);

// Depth buffer stays the same
data.depth = passBuilder.createDepthBuffer("rt_Depth", width, height);

// Update outputs struct
data.outputs.albedo = data.color;  // Reuse albedo field for compatibility
data.outputs.normal = {};           // Remove
data.outputs.material = {};         // Remove
data.outputs.depth = data.depth;
```

**Shader changes:**
```hlsl
// ═══════════════════════════════════════════════════════
//  BEFORE: shaders/r5/gbuffer/gbuffer_output.h
// ═══════════════════════════════════════════════════════

struct PSOutput {
    float4 normal   : SV_Target0;  // REMOVE
    float4 albedo   : SV_Target1;  // REMOVE
    float4 material : SV_Target2;  // REMOVE
};

// ═══════════════════════════════════════════════════════
//  AFTER: shaders/r5/forward/forward_output.h
// ═══════════════════════════════════════════════════════

struct PSOutput {
    float4 color : SV_Target0;  // Final lit color (RGB) + alpha
};

PSOutput PS_ForwardBase(VSOutput input) {
    PSOutput output;

    // Sample material textures
    float4 albedo = t_BaseColor.Sample(s_Material, input.texcoord);
    float3 normal = normalize(input.normal);  // World-space normal from VS

    // TEMPORARY: No lighting yet - just output albedo
    // Phase 3 will add sun lighting here
    output.color = albedo;

    return output;
}
```

**Rename pass:**
- `GBufferPassSetup.cpp` → `ForwardColorPassSetup.cpp`
- `setupGBufferPass()` → `setupForwardColorPass()`
- Update `r_FrameGraphRenderer.cpp` to call new function

**Success Metrics:**
- ✅ RenderDoc shows 1 RT write instead of 3
- ✅ Bandwidth usage drops ~60% (MRT write cost eliminated)
- ✅ Scene renders correctly (unlit albedo for now)
- ✅ Frame time improves 0.5-1.0ms from bandwidth savings

---

### Task 1.2: Restructure Shader Directory for Forward+

**New shader organization:**
```
res/gamedata/shaders/r5/
├── shared/
│   ├── common.h                 (Shared constants, keep existing)
│   ├── lighting_common.h        (NEW: Lighting functions)
│   └── shadow_common.h          (NEW: Shadow sampling)
│
├── forward/
│   ├── forward_base.vs          (NEW: Main vertex shader)
│   ├── forward_base.ps          (NEW: Main pixel shader)
│   ├── forward_skin.vs          (NEW: Skinned mesh vertex shader)
│   ├── forward_terrain.ps       (NEW: Terrain-specific pixel shader)
│   ├── forward_tree.vs/ps       (NEW: SpeedTree shaders)
│   └── forward_output.h         (NEW: Output structure)
│
├── depth/
│   ├── depth_prepass.vs         (NEW: Depth-only vertex shader)
│   ├── depth_prepass.ps         (NEW: Depth-only pixel shader - optional for alpha test)
│   └── depth_skin.vs            (NEW: Skinned depth prepass)
│
├── shadow/
│   ├── shadow_cascade.vs        (NEW: Shadow map vertex shader)
│   └── shadow_cascade.ps        (NEW: Shadow map pixel shader - optional for alpha test)
│
├── compute/
│   ├── light_culling.cs         (NEW: Clustered light culling)
│   └── shadow_mask.cs           (NEW: OPTIONAL - Async shadow pre-compute)
│
└── legacy/ (deprecated)
    └── gbuffer/ (old multi-RT shaders)
```

**Action Items:**
1. Create new shader directories
2. Write stub shaders (albedo-only output for now)
3. Update MaterialCache to recognize new shader names
4. Keep legacy shaders for reference during migration

---

### Task 1.3: Update Shader Constant Buffer Layout

**File:** `src/Layers/xrRender/FrameGraphPasses/ShaderConstants.h`

**Add forward+ specific constants:**
```cpp
// ═══════════════════════════════════════════════════════
//  FORWARD+ CONSTANT BUFFERS
// ═══════════════════════════════════════════════════════

// Per-frame globals (slot b0)
struct alignas(16) ForwardGlobals {
    Fmatrix m_VP;               // View-projection matrix
    Fmatrix m_V;                // View matrix
    Fmatrix m_P;                // Projection matrix
    Fmatrix m_InvVP;            // Inverse view-projection (for position reconstruction)

    Fvector4 camera_position;   // World-space camera position + padding
    Fvector4 camera_direction;  // Camera forward vector + padding

    Fvector4 sun_direction;     // Sun light direction (world space)
    Fvector4 sun_color;         // Sun light color (RGB) + intensity (A)

    // Shadow cascade data
    Fmatrix shadow_matrices[4]; // Shadow view-projection matrices (4 cascades)
    Fvector4 cascade_splits;    // Cascade split distances (x, y, z, w)

    // Cluster grid parameters
    Fvector4 cluster_params;    // (grid_dim_x, grid_dim_y, grid_dim_z, num_lights)
    Fvector4 cluster_scales;    // (z_near, z_far, scale_x, scale_y)

    // Fog parameters
    Fvector4 fog_params;        // (density, start, end, height_falloff)
    Fvector4 fog_color;         // RGB + padding

    // Time and other dynamic params
    Fvector4 time_params;       // (time, sin_time, cos_time, delta_time)
};

// Per-instance data (VCB - managed by FGConstantSystem)
struct alignas(16) ForwardInstance {
    Fmatrix m_World;            // World matrix
    Fmatrix m_WorldInvTranspose; // World inverse transpose (for normals)

    Fvector4 material_params;   // (metallic, roughness, ao, emission)
    Fvector4 dt_params;         // Detail texture params (scale_x, scale_y, scale_z, range)
};

// Material parameters (shader_params b1 - managed by FGConstantSystem)
struct alignas(16) ForwardMaterial {
    float m_AlphaRef;           // Alpha test threshold
    float padding[3];
    Fvector4 dt_params;         // Detail texture params
    Fvector4 material_color;    // Base color multiplier
};

// Helper to fill forward globals
inline void FillForwardGlobals(ForwardGlobals& cb) {
    // Camera matrices
    cb.m_VP = Device.mFullTransform;
    cb.m_V = Device.mView;
    cb.m_P = Device.mProject;

    Fmatrix invVP;
    invVP.invert(Device.mFullTransform);
    cb.m_InvVP = invVP;

    // Camera vectors
    cb.camera_position.set(Device.vCameraPosition.x, Device.vCameraPosition.y,
                           Device.vCameraPosition.z, 1.0f);
    cb.camera_direction.set(Device.vCameraDirection.x, Device.vCameraDirection.y,
                            Device.vCameraDirection.z, 0.0f);

    // Sun light (PLACEHOLDER - will be populated from level in Phase 3)
    cb.sun_direction.set(0.577f, 0.577f, 0.577f, 0.0f);  // Normalized diagonal
    cb.sun_color.set(1.0f, 0.95f, 0.9f, 1.0f);           // Warm white

    // Shadow matrices (PLACEHOLDER - Phase 4)
    for (int i = 0; i < 4; i++) {
        cb.shadow_matrices[i] = Fidentity;
    }
    cb.cascade_splits.set(10.0f, 50.0f, 150.0f, 500.0f);

    // Cluster grid (PLACEHOLDER - Phase 5)
    cb.cluster_params.set(16.0f, 16.0f, 24.0f, 0.0f);
    cb.cluster_scales.set(0.1f, 500.0f, 1.0f, 1.0f);

    // Fog (copy from existing system)
    cb.fog_params.set(0.001f, 50.0f, 500.0f, 0.5f);
    cb.fog_color.set(0.5f, 0.6f, 0.7f, 1.0f);

    // Time
    cb.time_params.set(Device.fTimeGlobal,
                       _sin(Device.fTimeGlobal),
                       _cos(Device.fTimeGlobal),
                       Device.fTimeDelta);
}
```

**Corresponding HLSL:**
```hlsl
// shaders/r5/shared/forward_constants.h

cbuffer ForwardGlobals : register(b0) {
    float4x4 m_VP;
    float4x4 m_V;
    float4x4 m_P;
    float4x4 m_InvVP;

    float4 camera_position;
    float4 camera_direction;

    float4 sun_direction;
    float4 sun_color;

    float4x4 shadow_matrices[4];
    float4 cascade_splits;

    float4 cluster_params;
    float4 cluster_scales;

    float4 fog_params;
    float4 fog_color;

    float4 time_params;
};

cbuffer ForwardInstance : register(b1) {
    float4x4 m_World;
    float4x4 m_WorldInvTranspose;
    float4 material_params;
    float4 dt_params;
};

cbuffer ForwardMaterial : register(b2) {
    float m_AlphaRef;
    float3 _pad0;
    float4 dt_params_mat;
    float4 material_color;
};
```

---

## Phase 2: Depth Prepass (Weeks 3-4)

**Goal:** Implement Z-only prepass for early-Z culling optimization

### Why Depth Prepass?

**Problem:** Overdraw kills performance (especially grass/foliage)
- X-Ray's grass can have 5-10x overdraw in dense areas
- Without depth prepass: Every pixel runs full pixel shader, then fails depth test
- **Waste:** 80-90% of pixel shader invocations discarded

**Solution:** Render depth first, then main pass gets free early-Z rejection
- Depth prepass: Cheap (just transform vertices, output depth)
- Main pass: GPU automatically rejects pixels that fail depth test BEFORE pixel shader

**Performance gain:** 40-60% reduction in pixel shader invocations (measurable in RenderDoc)

---

### Task 2.1: Create DepthPrepassSetup.cpp

**File:** `src/Layers/xrRender/FrameGraphPasses/DepthPrepassSetup.cpp`

```cpp
#include "stdafx.h"
#include "DepthPrepassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"

namespace xray::render::passes {

framegraph::VirtualResourceHandle setupDepthPrepass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    struct DepthPrepassData {
        VirtualResourceHandle depth;
        ng::RenderDevice* device;
        const GeometryCollector* geometry;
        MaterialCache* materialCache;
        u32 width;
        u32 height;
    };

    auto& passData = fg.addCallbackPass<DepthPrepassData>(
        "DepthPrepass",

        // Setup lambda
        [&](FrameGraph& builder, PassHandle passHandle, DepthPrepassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;

            // Create depth buffer
            data.depth = passBuilder.createDepthBuffer("rt_DepthPrepass", width, height);
        },

        // Execute lambda
        [](const DepthPrepassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            if (!depthRT || !data.geometry) return;

            const auto& batches = data.geometry->GetBatches();
            if (batches.empty()) return;

            // ═══════════════════════════════════════════════════════
            //  SETUP DEPTH-ONLY RENDER PASS
            // ═══════════════════════════════════════════════════════

            ng::RenderPassDesc passDesc;
            passDesc.depthStencil = depthRT;
            passDesc.numRenderTargets = 0;  // NO color writes!
            passDesc.clearDepth = true;
            passDesc.clearValue.depth = 1.0f;

            ctx->BeginRenderPass(passDesc);

            // Set viewport
            ctx->SetViewport(0, 0, (float)data.width, (float)data.height);

            ng::Rect scissor{0, 0, (int)data.width, (int)data.height};
            ctx->SetScissor(scissor);

            // ═══════════════════════════════════════════════════════
            //  RENDER DEPTH-ONLY
            // ═══════════════════════════════════════════════════════
            // Use simplified PSO:
            // - Vertex shader transforms positions
            // - Pixel shader can be NULL (or simple alpha test for foliage)
            // - No color writes
            // - Depth write ON, depth test LESS

            for (const auto& batch : batches) {
                // Get depth-only PSO from MaterialCache
                // TODO: Implement MaterialCache::GetOrCreateDepthPSO()
                // For now, skip actual rendering - just clear depth
            }

            ctx->EndRenderPass();
        }
    );

    return passData.depth;
}

} // namespace xray::render::passes
```

**Header:** `DepthPrepassSetup.h`
```cpp
#pragma once
#include "Layers/xrRender/FrameGraph/FrameGraph.h"

namespace xray::render {
    namespace ng { class RenderDevice; }
    class GeometryCollector;
    class MaterialCache;
}

namespace xray::render::passes {

// Setup depth prepass - renders geometry depth-only for early-Z optimization
framegraph::VirtualResourceHandle setupDepthPrepass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height);

} // namespace xray::render::passes
```

---

### Task 2.2: Create Depth Prepass Shaders

**File:** `res/gamedata/shaders/r5/depth/depth_prepass.vs`

```hlsl
#include "../shared/common.h"

struct VSInput {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD0;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;  // Only needed for alpha test materials
};

cbuffer PerInstance : register(b0) {
    float4x4 m_WVP;  // World-View-Projection matrix
};

VSOutput main(VSInput input) {
    VSOutput output;

    // Transform to clip space
    float4 worldPos = float4(input.position, 1.0);
    output.position = mul(m_WVP, worldPos);

    // Pass through texcoord (for alpha test in PS)
    output.texcoord = input.texcoord;

    return output;
}
```

**File:** `res/gamedata/shaders/r5/depth/depth_prepass.ps`

```hlsl
// Pixel shader for depth prepass
// OPTIONAL: Can be null for opaque geometry (depth write only)
// REQUIRED: For alpha-tested materials (grass, foliage)

Texture2D t_BaseColor : register(t0);
SamplerState s_Material : register(s0);

cbuffer Material : register(b0) {
    float m_AlphaRef;  // Alpha test threshold
};

struct PSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

void main(PSInput input) {
    // Alpha test (discard transparent pixels)
    float alpha = t_BaseColor.Sample(s_Material, input.texcoord).a;

    if (alpha < m_AlphaRef) {
        discard;  // Don't write depth for transparent pixels
    }

    // No output - just depth write
    // GPU automatically writes SV_Position.z to depth buffer
}
```

---

### Task 2.3: Integrate Depth Prepass into Pipeline

**File:** `src/Layers/xrRender/r_FrameGraphRenderer.cpp`

**Update `SetupFrameGraphPasses():`**

```cpp
void FrameGraphRenderer::SetupFrameGraphPasses() {
    const u32 width = Device.dwWidth;
    const u32 height = Device.dwHeight;

    // ═══════════════════════════════════════════════════════
    //  1. DEPTH PREPASS (NEW - Phase 2)
    // ═══════════════════════════════════════════════════════
    // Render all opaque geometry depth-only for early-Z optimization
    auto depthBuffer = passes::setupDepthPrepass(
        *m_framegraph,
        m_device,
        m_geometryCollector.get(),
        m_materialCache.get(),
        width,
        height
    );

    // ═══════════════════════════════════════════════════════
    //  2. FORWARD COLOR PASS (Modified to use depth prepass)
    // ═══════════════════════════════════════════════════════
    // Main rendering pass - now benefits from early-Z culling!
    auto forwardOutputs = passes::setupForwardColorPass(
        *m_framegraph,
        m_device,
        depthBuffer,  // REUSE depth from prepass!
        m_geometryCollector.get(),
        m_materialCache.get(),
        width,
        height
    );

    // Rest of pipeline unchanged...
    auto hudOutputs = passes::setupHUDPass(...);
    // etc.
}
```

**Critical: ForwardColorPass must REUSE depth buffer**

Update `ForwardColorPassSetup.cpp` to accept depth input:
```cpp
framegraph::DefaultOutputLayout setupForwardColorPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,  // NEW: Reuse from prepass
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height)
{
    // In setup lambda:
    if (depthInput.is_valid()) {
        // REUSE depth from prepass - read-only during main pass!
        data.depth = passBuilder.read(depthInput, ResourceState::DepthStencilRead);
    } else {
        // Fallback: create new depth buffer
        data.depth = passBuilder.createDepthBuffer("rt_Depth", width, height);
    }

    // In execute lambda:
    ng::RenderPassDesc passDesc;
    passDesc.depthStencil = depthRT;
    passDesc.clearDepth = false;  // DON'T clear - reuse prepass depth!
    passDesc.depthWriteEnable = false;  // Read-only depth test
    // ...
}
```

---

### Task 2.4: Add Depth PSO Creation to MaterialCache

**File:** `src/Layers/xrRender/Geometry/MaterialCache.h`

```cpp
class MaterialCache {
public:
    // Existing
    MaterialPSO* GetOrCreatePSO(dxRender_Visual* visual, ...);

    // NEW: Depth-only PSO for prepass
    MaterialPSO* GetOrCreateDepthPSO(dxRender_Visual* visual);

private:
    // Depth PSO cache (separate from main PSO cache)
    xr_map<shared_str, MaterialPSO*> m_depthPSOCache;
};
```

**Implementation:**
```cpp
MaterialPSO* MaterialCache::GetOrCreateDepthPSO(dxRender_Visual* visual) {
    // Extract shader from visual
    RENDER_NAMESPACE::Shader* shader = ExtractShader(visual);
    if (!shader) return nullptr;

    // Build cache key
    xr_string cacheKey = "depth_" + xr_string(shader->Name.c_str());

    // Check cache
    auto it = m_depthPSOCache.find(cacheKey.c_str());
    if (it != m_depthPSOCache.end()) {
        return it->second;
    }

    // Create new depth PSO
    MaterialPSO* depthPSO = xr_new<MaterialPSO>();

    // Use depth prepass shaders (or NULL PS for opaque)
    bool needsAlphaTest = /* check visual flags */;

    nvrhi::ShaderHandle vs = LoadShader("depth_prepass.vs");
    nvrhi::ShaderHandle ps = needsAlphaTest ? LoadShader("depth_prepass.ps") : nullptr;

    // Create PSO descriptor
    ng::PipelineStateDesc psoDesc;
    psoDesc.vertexShader = vs.Get();
    psoDesc.pixelShader = ps.Get();  // Can be NULL!

    // Depth state
    psoDesc.depthStencilState.depthTestEnable = true;
    psoDesc.depthStencilState.depthWriteEnable = true;
    psoDesc.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Less;

    // NO color writes
    psoDesc.blendState.renderTargets[0].writeMask = nvrhi::ColorWriteMask::None;

    // Rasterizer (cull back faces)
    psoDesc.rasterizerState.cullMode = nvrhi::CullMode::Back;

    // Input layout (position only, or + texcoord for alpha test)
    // ... setup vertex attributes

    // Create PSO
    depthPSO->pso = m_device->GetPipelineCache()->GetOrCreate(psoDesc);

    // Cache and return
    m_depthPSOCache[cacheKey.c_str()] = depthPSO;
    return depthPSO;
}
```

---

### Success Metrics (Phase 2)

**Validation:**
1. Open RenderDoc, capture frame
2. Check "DepthPrepass" event:
   - No color RT bound (depth-only)
   - Depth buffer cleared to 1.0
   - All opaque geometry rendered
3. Check "ForwardColor" event:
   - Depth buffer reused (NOT cleared)
   - "Input Assembler" → "VS Invocations" = same as prepass
   - **"Pixel Shader Invocations" = 40-60% LESS than VS invocations**
4. Frame time improvement: 0.5-1.5ms faster (from reduced pixel shader work)

**Expected Performance:**
- Before depth prepass: 90-110fps (grass scenes)
- After depth prepass: 110-130fps (grass scenes)
- **Gain: 20-25% FPS improvement**

---

## Phase 2.5: PBR Material Conversion (Week 5)

**Goal:** Convert legacy diffuse/specular/gloss materials to PBR workflow

### Implementation status (shipped)

- CLI: `src/Tools/MaterialConverter/` → target `MaterialConverter`
  - `scan|convert|fix|report --root <textures_dir>`
  - `--mode heuristic` (default), `--mode ai` when `-DXRAY_USE_AI_PBR=ON`
- Output: packed `<base>_pbr.dds` (R=M, G=R, B=AO, A=Parallax) + `.thm` `pbr_name`
- Runtime fallback: `PBRTextureConverter::ConvertTexturesToPBR` uses heuristic when AI unavailable
- Shader PBR: `ResolveMaterialPBR` / `SamplePBRFull` in `bindless_common.h`; LOD/scope/particle_lit/decal consume PBR
- Smoke:
  1. `MaterialConverter convert --root res/gamedata/textures --kind OpaqueDielectric`
  2. Load level with `r4_use_pbr 1`, confirm `MAT_FLAG_HAS_PBR` on forward/skinned/terrain/LOD

This is the **perfect time** to modernize to PBR because:
- Shaders are being rewritten anyway (no legacy baggage)
- Texture conversion can happen offline (one-time cost)
- Algorithmic conversion is "good enough" for most assets
- Hero assets can be hand-tuned later

### Overview: Legacy vs PBR Workflow

**X-Ray Legacy Workflow (Blinn-Phong):**
```
Textures:
- Diffuse map (RGB)       → Base color
- Specular map (RGB)      → Specular intensity/color
- Gloss map (R)           → Shininess exponent
- Normal map (RGB)        → Tangent-space normals

Shader:
- Diffuse = Lambertian(albedo, NdotL)
- Specular = BlinnPhong(specColor, gloss, NdotH)
- Final = Diffuse + Specular
```

**PBR Workflow (Physically Based):**
```
Textures:
- Base Color map (RGB)    → Albedo (non-metal) or F0 (metal)
- Metallic map (R)        → 0=dielectric, 1=metal
- Roughness map (R)       → 0=mirror, 1=diffuse
- Normal map (RGB)        → Tangent-space normals
- AO map (R) [optional]   → Ambient occlusion

Shader:
- Diffuse = Disney/Lambert(baseColor, metallic, NdotL)
- Specular = GGX(F0, roughness, NdotH, NdotV, NdotL)
- Final = (Diffuse * (1-metallic)) + Specular
```

**Key Differences:**
- Metallic determines material type (0=plastic/wood, 1=metal)
- Roughness replaces gloss (inverted: rough = 1 - gloss)
- Energy conservation (diffuse+specular ≤ 1.0)
- Physically accurate (looks correct under all lighting)

---

### Task 2.5.1: Implement Texture Converter Tool

**File:** `src/Tools/MaterialConverter/PBRConverter.h` (NEW)

```cpp
#pragma once
#include "xrCore/xrCore.h"

namespace xray::tools {

// PBR conversion parameters
struct ConversionParams {
    // Texture paths (input)
    xr_string diffusePath;
    xr_string specularPath;
    xr_string glossPath;
    xr_string normalPath;

    // Output paths
    xr_string baseColorPath;
    xr_string metallicPath;
    xr_string roughnessPath;
    xr_string normalOutPath;
    xr_string aoPath;  // Optional: extract AO from diffuse darkness

    // Conversion heuristics
    float defaultMetallic = 0.0f;     // Most assets are non-metal
    float defaultRoughness = 0.7f;    // Slightly rough default
    float metallicThreshold = 0.5f;   // Specular > this = maybe metal
    bool extractAO = true;            // Extract AO from diffuse shadows
    bool preserveSpecularColor = false; // Use specular color for metallic tint
};

// Material type detection
enum class MaterialType {
    Dielectric,  // Wood, plastic, stone, etc. (metallic=0)
    Metal,       // Iron, gold, copper, etc. (metallic=1)
    Unknown      // Need manual classification
};

class PBRConverter {
public:
    PBRConverter() = default;
    ~PBRConverter() = default;

    // Convert single material
    bool ConvertMaterial(const ConversionParams& params);

    // Batch convert entire texture directory
    u32 ConvertDirectory(const char* inputDir, const char* outputDir);

    // Material type detection (heuristics)
    MaterialType DetectMaterialType(const u8* diffuseRGB, const u8* specularRGB, u32 pixelCount);

private:
    // Per-pixel conversion functions
    void ConvertPixel(
        const u8* diffuse,      // Input: RGB
        const u8* specular,     // Input: RGB
        const u8* gloss,        // Input: R
        u8* baseColor,          // Output: RGB
        u8* metallic,           // Output: R
        u8* roughness,          // Output: R
        const ConversionParams& params
    );

    // AO extraction from diffuse darkness
    u8 ExtractAO(const u8* diffuseRGB);

    // Specular to metallic conversion
    float SpecularToMetallic(const u8* specularRGB, float specularIntensity);
};

} // namespace xray::tools
```

**Implementation:** `PBRConverter.cpp`

```cpp
#include "stdafx.h"
#include "PBRConverter.h"
#include "DDSTextureLoader.h"  // Use DirectXTex for DDS support
#include <algorithm>

namespace xray::tools {

bool PBRConverter::ConvertMaterial(const ConversionParams& params) {
    // ═══════════════════════════════════════════════════════
    //  LOAD INPUT TEXTURES
    // ═══════════════════════════════════════════════════════

    // Load diffuse texture (required)
    // Load specular texture (optional, use white if missing)
    // Load gloss texture (optional, use 0.5 if missing)
    // Load normal texture (optional, pass through)

    // TODO: Implement DirectXTex loading
    // For now, pseudocode:

    u32 width = 0, height = 0;
    xr_vector<u8> diffuseData;   // RGBA
    xr_vector<u8> specularData;  // RGBA
    xr_vector<u8> glossData;     // R (or extract from specular.a)

    // ... load textures ...

    // ═══════════════════════════════════════════════════════
    //  DETECT MATERIAL TYPE (optional optimization)
    // ═══════════════════════════════════════════════════════

    MaterialType type = DetectMaterialType(
        diffuseData.data(),
        specularData.data(),
        width * height
    );

    // Override conversion params based on detected type
    ConversionParams adjustedParams = params;
    if (type == MaterialType::Metal) {
        adjustedParams.defaultMetallic = 1.0f;
        adjustedParams.preserveSpecularColor = true;
    } else if (type == MaterialType::Dielectric) {
        adjustedParams.defaultMetallic = 0.0f;
    }

    // ═══════════════════════════════════════════════════════
    //  ALLOCATE OUTPUT TEXTURES
    // ═══════════════════════════════════════════════════════

    xr_vector<u8> baseColorData(width * height * 4);  // RGBA
    xr_vector<u8> metallicData(width * height);        // R
    xr_vector<u8> roughnessData(width * height);       // R
    xr_vector<u8> aoData;
    if (adjustedParams.extractAO) {
        aoData.resize(width * height);  // R
    }

    // ═══════════════════════════════════════════════════════
    //  CONVERT PIXELS
    // ═══════════════════════════════════════════════════════

    for (u32 y = 0; y < height; y++) {
        for (u32 x = 0; x < width; x++) {
            u32 idx = y * width + x;

            const u8* diffuse = &diffuseData[idx * 4];
            const u8* specular = &specularData[idx * 4];
            const u8* gloss = &glossData[idx];  // Single channel

            u8* baseColor = &baseColorData[idx * 4];
            u8* metallic = &metallicData[idx];
            u8* roughness = &roughnessData[idx];

            // Convert this pixel
            ConvertPixel(
                diffuse, specular, gloss,
                baseColor, metallic, roughness,
                adjustedParams
            );

            // Extract AO (if enabled)
            if (adjustedParams.extractAO) {
                aoData[idx] = ExtractAO(diffuse);
            }
        }
    }

    // ═══════════════════════════════════════════════════════
    //  SAVE OUTPUT TEXTURES
    // ═══════════════════════════════════════════════════════

    // Save baseColorData to params.baseColorPath (DDS RGBA8)
    // Save metallicData to params.metallicPath (DDS R8)
    // Save roughnessData to params.roughnessPath (DDS R8)
    // Copy normal map directly
    // Save aoData (if extracted)

    // TODO: Implement DirectXTex saving

    Msg("✓ Converted: %s → PBR", params.diffusePath.c_str());
    return true;
}

void PBRConverter::ConvertPixel(
    const u8* diffuse,
    const u8* specular,
    const u8* gloss,
    u8* baseColor,
    u8* metallic,
    u8* roughness,
    const ConversionParams& params)
{
    // ═══════════════════════════════════════════════════════
    //  BASE COLOR CONVERSION
    // ═══════════════════════════════════════════════════════

    // For dielectrics: diffuse → base color (direct copy)
    // For metals: diffuse → base color, but tinted by specular

    if (params.preserveSpecularColor) {
        // Metal: Use specular color as base color tint
        // Mix diffuse with specular color
        baseColor[0] = (diffuse[0] + specular[0]) / 2;  // R
        baseColor[1] = (diffuse[1] + specular[1]) / 2;  // G
        baseColor[2] = (diffuse[2] + specular[2]) / 2;  // B
        baseColor[3] = diffuse[3];                       // A
    } else {
        // Dielectric: Direct copy of diffuse
        baseColor[0] = diffuse[0];
        baseColor[1] = diffuse[1];
        baseColor[2] = diffuse[2];
        baseColor[3] = diffuse[3];
    }

    // ═══════════════════════════════════════════════════════
    //  METALLIC CONVERSION
    // ═══════════════════════════════════════════════════════

    // Heuristic: High specular intensity = likely metal
    // Specular luminance → metallic

    float specIntensity = (specular[0] + specular[1] + specular[2]) / (3.0f * 255.0f);
    float metallicValue = SpecularToMetallic(specular, specIntensity);

    // Clamp and quantize
    metallicValue = std::clamp(metallicValue, 0.0f, 1.0f);
    *metallic = (u8)(metallicValue * 255.0f);

    // ═══════════════════════════════════════════════════════
    //  ROUGHNESS CONVERSION
    // ═══════════════════════════════════════════════════════

    // Gloss → Roughness (invert)
    // X-Ray gloss: 0 = rough, 255 = shiny
    // PBR roughness: 0 = shiny, 255 = rough

    float glossValue = gloss[0] / 255.0f;
    float roughnessValue = 1.0f - glossValue;

    // Apply default if gloss is missing
    if (gloss[0] == 128) {  // Default middle value
        roughnessValue = params.defaultRoughness;
    }

    *roughness = (u8)(roughnessValue * 255.0f);
}

float PBRConverter::SpecularToMetallic(const u8* specularRGB, float specularIntensity) {
    // Simple heuristic:
    // - Low specular (< 0.3) → dielectric (metallic = 0)
    // - High specular (> 0.7) → metal (metallic = 1)
    // - Medium specular → blend

    if (specularIntensity < 0.3f) {
        return 0.0f;  // Dielectric
    } else if (specularIntensity > 0.7f) {
        return 1.0f;  // Metal
    } else {
        // Linear blend in middle range
        return (specularIntensity - 0.3f) / 0.4f;
    }
}

u8 PBRConverter::ExtractAO(const u8* diffuseRGB) {
    // AO heuristic: Dark areas in diffuse = likely occluded
    // Compute luminance and invert (dark = low AO)

    float luminance = (diffuseRGB[0] * 0.299f +
                       diffuseRGB[1] * 0.587f +
                       diffuseRGB[2] * 0.114f) / 255.0f;

    // Remap: very dark (< 0.2) = strong AO (0.5), bright (> 0.8) = no AO (1.0)
    float ao = std::clamp(luminance * 1.25f, 0.5f, 1.0f);

    return (u8)(ao * 255.0f);
}

MaterialType PBRConverter::DetectMaterialType(
    const u8* diffuseRGB,
    const u8* specularRGB,
    u32 pixelCount)
{
    // Sample pixels and compute average specular intensity
    float avgSpecular = 0.0f;
    u32 sampleCount = std::min(pixelCount, 1000u);  // Sample up to 1000 pixels

    for (u32 i = 0; i < sampleCount; i++) {
        u32 idx = (i * pixelCount) / sampleCount;  // Evenly spaced samples
        const u8* spec = &specularRGB[idx * 4];
        avgSpecular += (spec[0] + spec[1] + spec[2]) / (3.0f * 255.0f);
    }
    avgSpecular /= sampleCount;

    // Classification thresholds
    if (avgSpecular < 0.3f) {
        return MaterialType::Dielectric;  // Low specular = non-metal
    } else if (avgSpecular > 0.7f) {
        return MaterialType::Metal;       // High specular = metal
    } else {
        return MaterialType::Unknown;     // Needs manual inspection
    }
}

u32 PBRConverter::ConvertDirectory(const char* inputDir, const char* outputDir) {
    // Scan inputDir for texture sets
    // For each material:
    //   - Find diffuse, specular, gloss, normal textures
    //   - Run ConvertMaterial()
    //   - Output to outputDir

    u32 convertedCount = 0;

    // TODO: Implement directory scanning
    // Pseudocode:
    // for each material in inputDir:
    //     ConversionParams params;
    //     params.diffusePath = inputDir + "/" + material + "_diff.dds";
    //     params.specularPath = inputDir + "/" + material + "_spec.dds";
    //     // ... etc.
    //     params.baseColorPath = outputDir + "/" + material + "_basecolor.dds";
    //     // ... etc.
    //     if (ConvertMaterial(params)) convertedCount++;

    Msg("✓ Batch conversion complete: %u materials", convertedCount);
    return convertedCount;
}

} // namespace xray::tools
```

---

### Task 2.5.2: Create Batch Conversion Script

**File:** `src/Tools/MaterialConverter/convert_all_materials.cpp`

```cpp
#include "stdafx.h"
#include "PBRConverter.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: convert_materials <input_dir> <output_dir>\n");
        return 1;
    }

    const char* inputDir = argv[1];
    const char* outputDir = argv[2];

    xray::tools::PBRConverter converter;
    u32 count = converter.ConvertDirectory(inputDir, outputDir);

    printf("\n✓ Converted %u materials to PBR\n", count);
    printf("  Input:  %s\n", inputDir);
    printf("  Output: %s\n", outputDir);

    return 0;
}
```

**Build and run:**
```bash
# Build converter tool
cmake --build . --target MaterialConverter

# Convert all gamedata textures
./MaterialConverter \
    "res/gamedata/textures" \
    "res/gamedata/textures_pbr"

# This creates PBR textures:
# - textures_pbr/mtl_name_basecolor.dds
# - textures_pbr/mtl_name_metallic.dds
# - textures_pbr/mtl_name_roughness.dds
# - textures_pbr/mtl_name_normal.dds (copied)
# - textures_pbr/mtl_name_ao.dds (extracted)
```

---

### Task 2.5.3: Update Shader Texture Sampling

**File:** `res/gamedata/shaders/r5/forward/forward_base.ps`

Update to sample PBR textures:

```hlsl
// ═══════════════════════════════════════════════════════
//  PBR TEXTURE INPUTS
// ═══════════════════════════════════════════════════════

Texture2D t_BaseColor : register(t0);   // RGB = albedo, A = opacity
Texture2D t_Normal : register(t1);      // RGB = tangent-space normal
Texture2D t_Material : register(t2);    // R = metallic, G = roughness, B = AO
SamplerState s_Material : register(s0);

PSOutput main(PSInput input) {
    // ═══════════════════════════════════════════════════════
    //  SAMPLE PBR TEXTURES
    // ═══════════════════════════════════════════════════════

    float4 baseColorSample = t_BaseColor.Sample(s_Material, input.texcoord);
    float3 baseColor = baseColorSample.rgb;
    float alpha = baseColorSample.a;

    // Sample material properties
    float3 materialSample = t_Material.Sample(s_Material, input.texcoord).rgb;
    float metallic = materialSample.r;   // 0 = dielectric, 1 = metal
    float roughness = materialSample.g;  // 0 = mirror, 1 = rough
    float ao = materialSample.b;         // Ambient occlusion

    // Sample normal map (TODO: implement tangent-space transform)
    float3 tangentNormal = t_Normal.Sample(s_Material, input.texcoord).rgb * 2.0 - 1.0;
    float3 normal = normalize(input.normal);  // Placeholder - use vertex normal for now

    // Alpha test
    if (alpha < m_AlphaRef) {
        discard;
    }

    // ... rest of shader uses PBR values ...
}
```

---

### Task 2.5.4: Material Naming Convention

**Update X-Ray material files (`.thm`) to reference PBR textures:**

```
// OLD (legacy workflow):
material "concrete_wall" {
    diffuse  = "textures/concrete_wall_diff.dds"
    specular = "textures/concrete_wall_spec.dds"
    gloss    = "textures/concrete_wall_gloss.dds"
    normal   = "textures/concrete_wall_bump.dds"
}

// NEW (PBR workflow):
material "concrete_wall" {
    basecolor = "textures_pbr/concrete_wall_basecolor.dds"
    material  = "textures_pbr/concrete_wall_material.dds"  // R=metallic, G=roughness, B=AO
    normal    = "textures_pbr/concrete_wall_normal.dds"
}
```

**Or pack into single material texture:**
```
material "concrete_wall" {
    basecolor = "textures_pbr/concrete_wall_basecolor.dds"  // RGBA8
    material  = "textures_pbr/concrete_wall_material.dds"   // RGBA8: R=metallic, G=roughness, B=AO, A=unused
    normal    = "textures_pbr/concrete_wall_normal.dds"     // RGBA8: RG=normal XY, B=unused, A=height
}
```

---

### Task 2.5.5: Material Migration Strategy

**Three-phase approach:**

**Phase A: Automatic Conversion (Week 5, Day 1-2)**
```bash
# Convert ALL textures automatically
./MaterialConverter res/gamedata/textures res/gamedata/textures_pbr

# Update material database to reference new textures
./UpdateMaterialPaths textures → textures_pbr
```

**Result:**
- All materials now use PBR textures
- Quality is "good enough" for most assets (80-90% acceptable)
- Renderer uses PBR workflow from this point forward

**Phase B: Classify Problem Materials (Week 5, Day 3)**
```bash
# Generate quality report
./AnalyzePBRConversion textures_pbr > pbr_quality_report.txt

# Produces:
# ✓ concrete_wall.dds - Good (dielectric detected)
# ✓ metal_door.dds - Good (metal detected)
# ⚠ wood_plank.dds - Uncertain (metallic=0.4, needs review)
# ✗ gold_trim.dds - Poor (should be metal but detected as dielectric)
```

**Phase C: Hand-Tune Heroes (Ongoing)**
```
Priority 1 (Week 5, Day 4-5): Fix critical assets
- Weapons (player sees constantly)
- Character faces/skin
- Key environmental pieces

Priority 2 (Post-release): Polish all assets
- Background props
- Far LOD materials
- Minor details
```

**Fallback Mode (Legacy Compatibility):**
```cpp
// In MaterialCache, support BOTH workflows during transition:
if (material->HasTexture("basecolor")) {
    // PBR workflow - use new shaders
    usePBR = true;
} else if (material->HasTexture("diffuse")) {
    // Legacy workflow - use old shaders (temporary)
    usePBR = false;
} else {
    // Error
}
```

---

### Success Metrics (Phase 2.5)

**Validation:**
1. Run converter on sample assets:
   - Input: `textures/concrete_diff.dds`, `concrete_spec.dds`, `concrete_gloss.dds`
   - Output: `textures_pbr/concrete_basecolor.dds`, `concrete_material.dds`
2. Load converted material in game
3. Compare side-by-side with original:
   - Should look **similar** (not identical, but acceptable)
   - PBR should look **better** under varied lighting
4. Check problematic cases:
   - Pure metals (gold, iron) - should have metallic=1.0
   - Wood/plastic - should have metallic=0.0
   - Glass/water - may need manual tweaking

**Performance:**
- Conversion tool: ~1-5 minutes for full gamedata (offline, one-time cost)
- Runtime: Same as before (just different texture names)

**Quality Guidelines:**
- **Acceptable:** 80%+ of assets look decent after auto-conversion
- **Good:** 10-15% need minor tweaks (adjust metallic/roughness values)
- **Manual Required:** 5-10% need artist intervention (hand-painted material maps)

**Timeline:**
- Days 1-2: Implement converter tool
- Day 3: Batch convert all textures
- Day 4-5: Fix critical assets, validate quality
- **Total: 1 week**

---

## Phase 3: PBR Forward Lighting (Weeks 6-7)

**Goal:** Implement physically-based sun lighting (now using PBR instead of Blinn-Phong)

This phase brings the renderer to life - geometry will be lit with **physically accurate** PBR!

### Task 3.1: Implement PBR Lighting Helper Functions

**File:** `res/gamedata/shaders/r5/shared/lighting_common.h`

```hlsl
#ifndef LIGHTING_COMMON_H
#define LIGHTING_COMMON_H

// ═══════════════════════════════════════════════════════
//  PBR LIGHTING MODELS (Cook-Torrance BRDF)
// ═══════════════════════════════════════════════════════

#define PI 3.14159265359

// ───────────────────────────────────────────────────────
//  DIFFUSE BRDF
// ───────────────────────────────────────────────────────

// Disney diffuse (energy-conserving Lambertian)
float3 Diffuse_Disney(float3 albedo, float NdotL) {
    return albedo * saturate(NdotL) / PI;
}

// Standard Lambertian (simpler, slightly faster)
float3 Diffuse_Lambert(float3 albedo, float NdotL) {
    return albedo * saturate(NdotL) / PI;
}

// ───────────────────────────────────────────────────────
//  SPECULAR BRDF (Cook-Torrance)
// ───────────────────────────────────────────────────────

// GGX/Trowbridge-Reitz Normal Distribution Function
float D_GGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001);
}

// Schlick-GGX Geometry Function (single direction)
float G_SchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;  // For direct lighting

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / max(denom, 0.0000001);
}

// Smith's method - combines geometry for both view and light directions
float G_Smith(float NdotV, float NdotL, float roughness) {
    float ggx1 = G_SchlickGGX(NdotV, roughness);
    float ggx2 = G_SchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// Fresnel-Schlick approximation
float3 F_Schlick(float3 F0, float VdotH) {
    return F0 + (1.0 - F0) * pow(saturate(1.0 - VdotH), 5.0);
}

// Fresnel with roughness (for environment reflections)
float3 F_SchlickRoughness(float3 F0, float VdotH, float roughness) {
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(saturate(1.0 - VdotH), 5.0);
}

// ───────────────────────────────────────────────────────
//  HELPER FUNCTIONS
// ───────────────────────────────────────────────────────

// Compute F0 (surface reflection at zero incidence)
float3 ComputeF0(float3 albedo, float metallic) {
    // Dielectrics have F0 ≈ 0.04 (4% reflectance)
    // Metals use albedo as F0 (colored reflection)
    float3 F0_dielectric = float3(0.04, 0.04, 0.04);
    return lerp(F0_dielectric, albedo, metallic);
}

// ═══════════════════════════════════════════════════════
//  LIGHT EVALUATION (PBR)
// ═══════════════════════════════════════════════════════

struct SurfaceData {
    float3 position;       // World-space position
    float3 normal;         // World-space normal (normalized)
    float3 view_dir;       // Direction to camera (normalized)
    float3 albedo;         // Base color
    float metallic;        // Metallic factor [0, 1]
    float roughness;       // Roughness factor [0, 1]
    float occlusion;       // Ambient occlusion [0, 1]
};

// Evaluate directional light (sun) using PBR
float3 EvaluateDirectionalLight(
    SurfaceData surface,
    float3 light_dir,      // Direction TO light (normalized)
    float3 light_color,    // Light color * intensity
    float shadow)          // Shadow factor [0, 1]
{
    // ═══════════════════════════════════════════════════════
    //  SETUP VECTORS
    // ═══════════════════════════════════════════════════════

    float3 N = surface.normal;
    float3 V = surface.view_dir;
    float3 L = light_dir;
    float3 H = normalize(V + L);  // Half vector

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    // Early out if surface faces away from light
    if (NdotL <= 0.0) return float3(0, 0, 0);

    // ═══════════════════════════════════════════════════════
    //  COOK-TORRANCE BRDF
    // ═══════════════════════════════════════════════════════

    // F0 = surface reflection at zero incidence
    float3 F0 = ComputeF0(surface.albedo, surface.metallic);

    // D = Normal Distribution (GGX)
    float D = D_GGX(NdotH, surface.roughness);

    // G = Geometry term (Smith's method)
    float G = G_Smith(NdotV, NdotL, surface.roughness);

    // F = Fresnel (Schlick approximation)
    float3 F = F_Schlick(F0, VdotH);

    // ═══════════════════════════════════════════════════════
    //  COMPUTE SPECULAR
    // ═══════════════════════════════════════════════════════

    float3 specular_num = D * G * F;
    float specular_denom = 4.0 * NdotV * NdotL;
    float3 specular = specular_num / max(specular_denom, 0.001);

    // ═══════════════════════════════════════════════════════
    //  COMPUTE DIFFUSE
    // ═══════════════════════════════════════════════════════

    // kS = specular contribution (Fresnel)
    // kD = diffuse contribution (1 - kS, energy conservation)
    float3 kS = F;
    float3 kD = 1.0 - kS;

    // Metals have no diffuse (all energy goes to specular)
    kD *= (1.0 - surface.metallic);

    float3 diffuse = kD * Diffuse_Lambert(surface.albedo, NdotL);

    // ═══════════════════════════════════════════════════════
    //  FINAL LIGHTING
    // ═══════════════════════════════════════════════════════

    float3 lighting = (diffuse + specular) * light_color * NdotL * shadow;

    return lighting;
}

// Evaluate point light using PBR
float3 EvaluatePointLight(
    SurfaceData surface,
    float3 light_pos,      // Light world position
    float3 light_color,    // Light color * intensity
    float light_range)     // Light max range
{
    // Vector to light
    float3 L = light_pos - surface.position;
    float distance = length(L);
    L /= distance;  // Normalize

    // Attenuation (inverse square with range cutoff)
    float attenuation = saturate(1.0 - (distance / light_range));
    attenuation *= attenuation;  // Smooth falloff
    attenuation /= (distance * distance + 1.0);  // Inverse square

    if (attenuation <= 0.001) return float3(0, 0, 0);  // Early out

    // ═══════════════════════════════════════════════════════
    //  COOK-TORRANCE BRDF (same as directional)
    // ═══════════════════════════════════════════════════════

    float3 N = surface.normal;
    float3 V = surface.view_dir;
    float3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    if (NdotL <= 0.0) return float3(0, 0, 0);

    // F0, D, G, F
    float3 F0 = ComputeF0(surface.albedo, surface.metallic);
    float D = D_GGX(NdotH, surface.roughness);
    float G = G_Smith(NdotV, NdotL, surface.roughness);
    float3 F = F_Schlick(F0, VdotH);

    // Specular
    float3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);

    // Diffuse
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - surface.metallic);
    float3 diffuse = kD * Diffuse_Lambert(surface.albedo, NdotL);

    // Apply attenuation
    return (diffuse + specular) * light_color * NdotL * attenuation;
}

// Evaluate spot light
float3 EvaluateSpotLight(
    SurfaceData surface,
    float3 light_pos,
    float3 light_dir,      // Spot direction (normalized)
    float3 light_color,
    float light_range,
    float inner_angle,     // Inner cone angle (radians)
    float outer_angle)     // Outer cone angle (radians)
{
    // Vector to light
    float3 L = light_pos - surface.position;
    float distance = length(L);
    L /= distance;

    // Spot cone attenuation
    float spot_factor = dot(-L, light_dir);
    float inner_cos = cos(inner_angle);
    float outer_cos = cos(outer_angle);
    float spot_attenuation = smoothstep(outer_cos, inner_cos, spot_factor);

    if (spot_attenuation <= 0.001) return float3(0, 0, 0);

    // Same as point light + spot attenuation
    float attenuation = saturate(1.0 - (distance / light_range));
    attenuation *= attenuation;
    attenuation /= (distance * distance + 1.0);
    attenuation *= spot_attenuation;

    float3 N = surface.normal;
    float3 V = surface.view_dir;
    float3 H = normalize(V + L);

    float NdotL = dot(N, L);
    float NdotH = dot(N, H);

    if (NdotL <= 0.0) return float3(0, 0, 0);

    float3 diffuse = Diffuse_Lambert(surface.albedo, NdotL);
    float3 specColor = lerp(float3(0.04, 0.04, 0.04), surface.albedo, surface.metallic);
    float3 specular = Specular_BlinnPhong(specColor, surface.roughness, NdotH, NdotL);

    return (diffuse + specular) * light_color * attenuation;
}

#endif // LIGHTING_COMMON_H
```

---

### Task 3.2: Update Forward Pixel Shader with Lighting

**File:** `res/gamedata/shaders/r5/forward/forward_base.ps`

```hlsl
#include "../shared/common.h"
#include "../shared/forward_constants.h"
#include "../shared/lighting_common.h"

// Textures
Texture2D t_BaseColor : register(t0);
Texture2D t_Normal : register(t1);     // Normal map (optional)
Texture2D t_Material : register(t2);   // R=metallic, G=roughness, B=AO
SamplerState s_Material : register(s0);

struct PSInput {
    float4 position    : SV_Position;
    float3 worldPos    : WORLD_POS;
    float3 normal      : NORMAL;
    float3 tangent     : TANGENT;
    float2 texcoord    : TEXCOORD0;
};

struct PSOutput {
    float4 color : SV_Target0;
};

PSOutput main(PSInput input) {
    PSOutput output;

    // ═══════════════════════════════════════════════════════
    //  SAMPLE MATERIAL TEXTURES
    // ═══════════════════════════════════════════════════════

    float4 albedoSample = t_BaseColor.Sample(s_Material, input.texcoord);
    float3 albedo = albedoSample.rgb;
    float alpha = albedoSample.a;

    // Alpha test (for foliage, fences, etc.)
    if (alpha < m_AlphaRef) {
        discard;
    }

    // Sample material properties (metallic, roughness, AO)
    float3 materialSample = t_Material.Sample(s_Material, input.texcoord).rgb;
    float metallic = materialSample.r;
    float roughness = materialSample.g;
    float occlusion = materialSample.b;

    // Sample normal map (if available)
    // TODO: Implement tangent-space normal mapping
    float3 normal = normalize(input.normal);

    // ═══════════════════════════════════════════════════════
    //  PREPARE SURFACE DATA
    // ═══════════════════════════════════════════════════════

    SurfaceData surface;
    surface.position = input.worldPos;
    surface.normal = normal;
    surface.view_dir = normalize(camera_position.xyz - input.worldPos);
    surface.albedo = albedo;
    surface.metallic = metallic;
    surface.roughness = max(roughness, 0.04);  // Avoid division by zero
    surface.occlusion = occlusion;

    // ═══════════════════════════════════════════════════════
    //  LIGHTING EVALUATION
    // ═══════════════════════════════════════════════════════

    float3 lighting = float3(0, 0, 0);

    // Sun light (directional)
    float shadow = 1.0;  // No shadows yet - Phase 4
    lighting += EvaluateDirectionalLight(
        surface,
        sun_direction.xyz,   // From ForwardGlobals CB
        sun_color.rgb * sun_color.a,  // Color * intensity
        shadow
    );

    // Ambient lighting (simple hemisphere lighting for now)
    float3 ambient_sky = float3(0.4, 0.5, 0.7);    // Blue sky
    float3 ambient_ground = float3(0.3, 0.25, 0.2); // Brown ground
    float hemisphere_blend = normal.y * 0.5 + 0.5;  // Map [-1,1] to [0,1]
    float3 ambient = lerp(ambient_ground, ambient_sky, hemisphere_blend);
    lighting += albedo * ambient * 0.3 * occlusion;  // Scale ambient

    // ═══════════════════════════════════════════════════════
    //  FOG (optional - matches legacy X-Ray)
    // ═══════════════════════════════════════════════════════

    float dist = length(camera_position.xyz - input.worldPos);
    float fog_factor = saturate((dist - fog_params.y) / (fog_params.z - fog_params.y));
    lighting = lerp(lighting, fog_color.rgb, fog_factor);

    // ═══════════════════════════════════════════════════════
    //  OUTPUT
    // ═══════════════════════════════════════════════════════

    output.color = float4(lighting, alpha);

    return output;
}
```

---

### Task 3.3: Update Vertex Shader to Output World Position

**File:** `res/gamedata/shaders/r5/forward/forward_base.vs`

```hlsl
#include "../shared/common.h"
#include "../shared/forward_constants.h"

struct VSInput {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float2 texcoord : TEXCOORD0;
};

struct VSOutput {
    float4 position : SV_Position;      // Clip-space position
    float3 worldPos : WORLD_POS;        // World-space position (for lighting)
    float3 normal   : NORMAL;           // World-space normal
    float3 tangent  : TANGENT;          // World-space tangent
    float2 texcoord : TEXCOORD0;
};

VSOutput main(VSInput input) {
    VSOutput output;

    // Transform to world space
    float4 worldPos = mul(m_World, float4(input.position, 1.0));
    output.worldPos = worldPos.xyz;

    // Transform to clip space
    output.position = mul(m_VP, worldPos);

    // Transform normal to world space (using inverse transpose for non-uniform scaling)
    output.normal = mul((float3x3)m_WorldInvTranspose, input.normal);

    // Transform tangent to world space
    output.tangent = mul((float3x3)m_World, input.tangent);

    // Pass through texcoord
    output.texcoord = input.texcoord;

    return output;
}
```

---

### Task 3.4: Update FGConstantSystem Integration

**File:** `src/Layers/xrRender/FrameGraphPasses/ForwardColorPassSetup.cpp`

Update rendering loop to set per-instance world matrix:

```cpp
// In execute lambda, rendering loop:
fgconstants::FGConstantSystem constants(matPSO, data.materialCache->GetVCBPool());

for (const auto& batch : batches) {
    // ... PSO setup ...

    // ═══════════════════════════════════════════════════════
    //  SET PER-INSTANCE CONSTANTS (m_World, m_WorldInvTranspose)
    // ═══════════════════════════════════════════════════════

    // World matrix
    constants.Set("m_World", batch.worldMatrix);

    // World inverse transpose (for normals)
    Fmatrix worldInvTranspose;
    worldInvTranspose.invert(batch.worldMatrix);
    worldInvTranspose.transpose();
    constants.Set("m_WorldInvTranspose", worldInvTranspose);

    // Material parameters
    constants.Set("material_params", Fvector4(0.0f, 0.5f, 1.0f, 0.0f)); // metallic, roughness, AO, emission

    // Commit instance constants (creates/updates VCB)
    constants.CommitInstance(ctx);

    // Draw
    ctx->DrawIndexed(batch.indexCount, batch.startIndex, batch.baseVertex);
}
```

---

### Task 3.5: Extract Sun Light from X-Ray Level Data

X-Ray stores environment lighting in `CEnvDescriptor`:

**File:** `src/Layers/xrRender/r_FrameGraphRenderer.cpp`

Update `FillForwardGlobals()` to read real sun data:

```cpp
inline void FillForwardGlobals(ForwardGlobals& cb) {
    // ... camera matrices ...

    // ═══════════════════════════════════════════════════════
    //  EXTRACT SUN LIGHT FROM LEVEL ENVIRONMENT
    // ═══════════════════════════════════════════════════════

    if (g_pGamePersistent && g_pGamePersistent->Environment().CurrentEnv) {
        CEnvDescriptor* env = g_pGamePersistent->Environment().CurrentEnv;

        // Sun direction (world space)
        Fvector sun_dir = env->sun_dir;
        cb.sun_direction.set(sun_dir.x, sun_dir.y, sun_dir.z, 0.0f);

        // Sun color and intensity
        Fvector sun_col = env->sun_color;
        float sun_intensity = env->sun_lum;  // Brightness multiplier
        cb.sun_color.set(sun_col.x, sun_col.y, sun_col.z, sun_intensity);

    } else {
        // Fallback: Default sun
        cb.sun_direction.set(0.577f, 0.577f, 0.577f, 0.0f);
        cb.sun_color.set(1.0f, 0.95f, 0.9f, 1.0f);
    }

    // ... rest of globals ...
}
```

---

### Success Metrics (Phase 3)

**Validation:**
1. Load a level
2. Geometry should be **fully lit** (not just albedo)
3. Sun light direction should match time-of-day
4. Rotating camera should show correct specular highlights
5. Ambient lighting should illuminate shadowed areas

**Visual Quality:**
- Compare with original renderer (without shadows)
- Lighting direction should match
- Material response (rough/smooth) should be visible
- Day/night cycle should work

**Performance:**
- Should be similar to Phase 2 (lighting math is cheap)
- Possibly 0.5-1.0ms slower (pixel shader work increased)
- Still faster than original due to depth prepass optimization

**Expected Frame Time:**
- 110-130fps (same as Phase 2, lighting cost offset by GPU optimizations)

---

## Phase 3.5: GPU-Driven Rendering & Culling (Week 8)

**Goal:** Implement GPU frustum/occlusion culling and modernize DetailManager for grass rendering

This phase is **critical for performance** - GPU culling enables rendering massive scenes (100K+ objects) at high framerates.

### Why Now?

**Perfect timing because:**
- ✅ Depth prepass is working (enables Hi-Z occlusion culling)
- ✅ Basic rendering pipeline stable (can measure gains)
- ✅ Before shadows (culling helps shadow pass too)
- ✅ Already have `detail_cull` compute shader (just needs porting)

**Performance Impact:**
- CPU-side culling: ~2-5ms for 10K objects
- GPU culling: ~0.3-0.5ms for 100K objects
- **Gain: 10-20× faster culling, enables 10× more geometry**

---

### Overview: GPU-Driven Rendering Pipeline

**Traditional (CPU-driven):**
```cpp
// CPU iterates all objects
for (object in scene) {
    if (frustum.contains(object)) {  // CPU frustum test
        if (occlusion.visible(object)) {  // CPU occlusion query
            batches.push_back(object);  // Add to draw list
        }
    }
}

// CPU uploads draw commands
for (batch in batches) {
    ctx->DrawIndexed(batch.indexCount, ...);  // CPU submits each draw
}

Problems:
- CPU bottleneck (serial iteration)
- Draw call overhead (thousands of API calls)
- Stale occlusion data (1-2 frames latency)
```

**GPU-Driven:**
```cpp
// GPU compute shader culls all objects in parallel
DispatchCompute(object_count / 64, 1, 1);
  ├─ Frustum culling (parallel, per-object)
  ├─ Hi-Z occlusion culling (parallel, hierarchical depth)
  └─ Output: Indirect draw buffer (visible objects only)

// GPU executes draws from indirect buffer (NO CPU involvement!)
DrawIndirect(indirectBuffer, drawCount);  // Single API call, GPU reads commands

Benefits:
- GPU parallelism (64 objects culled per wave)
- Zero draw call overhead (1 indirect draw)
- Current-frame occlusion (no latency)
- 10-100× more geometry possible
```

---

### Task 3.5.1: Modernize DetailManager for NVRHI/FrameGraph

**Current Issue:** DetailManager has DX11 dependencies, disabled in current renderer

**File:** `src/Layers/xrRender/DetailManager.h`

**Changes needed:**
```cpp
// ═══════════════════════════════════════════════════════
//  BEFORE (DX11-specific)
// ═══════════════════════════════════════════════════════

class CDetailManager {
    ID3D11Buffer* m_instanceBuffer;        // DX11
    ID3D11Buffer* m_indirectDrawBuffer;    // DX11
    ID3D11UnorderedAccessView* m_uav;      // DX11

    void Render() {
        // Direct DX11 calls
        pContext->DrawIndexedInstancedIndirect(m_indirectDrawBuffer, 0);
    }
};

// ═══════════════════════════════════════════════════════
//  AFTER (NVRHI/FrameGraph)
// ═══════════════════════════════════════════════════════

class CDetailManager {
    nvrhi::BufferHandle m_instanceBuffer;       // NVRHI
    nvrhi::BufferHandle m_indirectDrawBuffer;   // NVRHI
    nvrhi::BufferHandle m_visibleCountBuffer;   // GPU writes visible count

    // FrameGraph integration
    framegraph::VirtualResourceHandle m_instanceBufferHandle;
    framegraph::VirtualResourceHandle m_indirectDrawHandle;

    void SetupCullingPass(framegraph::FrameGraph& fg, framegraph::VirtualResourceHandle depthBuffer);
    void SetupRenderPass(framegraph::FrameGraph& fg);
};
```

**Implementation:** `DetailManager.cpp`

```cpp
#include "stdafx.h"
#include "DetailManager.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"

void CDetailManager::Initialize(ng::RenderDevice* device) {
    // ═══════════════════════════════════════════════════════
    //  CREATE GPU BUFFERS
    // ═══════════════════════════════════════════════════════

    // Instance buffer (per-grass-blade data)
    nvrhi::BufferDesc instanceDesc;
    instanceDesc.byteSize = MAX_GRASS_INSTANCES * sizeof(GrassInstance);
    instanceDesc.structStride = sizeof(GrassInstance);
    instanceDesc.canHaveUAVs = true;  // GPU writes to this
    instanceDesc.isVertexBuffer = true;
    instanceDesc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    instanceDesc.keepInitialState = false;
    instanceDesc.debugName = "GrassInstanceBuffer";

    m_instanceBuffer = device->GetNVRHIDevice()->createBuffer(instanceDesc);

    // Indirect draw buffer (GPU writes draw commands)
    nvrhi::BufferDesc indirectDesc;
    indirectDesc.byteSize = sizeof(nvrhi::DrawArguments) * MAX_GRASS_BATCHES;
    indirectDesc.structStride = sizeof(nvrhi::DrawArguments);
    indirectDesc.canHaveUAVs = true;
    indirectDesc.isDrawIndirectArgs = true;
    indirectDesc.initialState = nvrhi::ResourceStates::IndirectArgument;
    indirectDesc.keepInitialState = false;
    indirectDesc.debugName = "GrassIndirectDrawBuffer";

    m_indirectDrawBuffer = device->GetNVRHIDevice()->createBuffer(indirectDesc);

    // Visible count buffer (atomic counter for GPU)
    nvrhi::BufferDesc countDesc;
    countDesc.byteSize = sizeof(u32);
    countDesc.canHaveUAVs = true;
    countDesc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    countDesc.debugName = "GrassVisibleCount";

    m_visibleCountBuffer = device->GetNVRHIDevice()->createBuffer(countDesc);

    Msg("✓ DetailManager initialized (NVRHI)");
}

void CDetailManager::SetupCullingPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle depthBuffer)
{
    using namespace framegraph;

    struct CullingPassData {
        VirtualResourceHandle instanceBuffer;
        VirtualResourceHandle indirectBuffer;
        VirtualResourceHandle visibleCount;
        VirtualResourceHandle depthHiZ;  // Hi-Z depth pyramid
        CDetailManager* detailMgr;
    };

    auto& passData = fg.addCallbackPass<CullingPassData>(
        "GrassCulling",
        PassFlags::Compute,

        // Setup lambda
        [&](FrameGraph& builder, PassHandle passHandle, CullingPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.detailMgr = this;

            // Create virtual handles for persistent buffers
            // (These are created once, reused every frame)
            data.instanceBuffer = passBuilder.importBuffer(
                "GrassInstances", m_instanceBuffer, ResourceState::UnorderedAccess);
            data.indirectBuffer = passBuilder.importBuffer(
                "GrassIndirectDraw", m_indirectDrawBuffer, ResourceState::UnorderedAccess);
            data.visibleCount = passBuilder.importBuffer(
                "GrassVisibleCount", m_visibleCountBuffer, ResourceState::UnorderedAccess);

            // Read Hi-Z depth for occlusion culling
            data.depthHiZ = passBuilder.read(depthBuffer, ResourceState::ShaderResource);
        },

        // Execute lambda
        [](const CullingPassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            // Run GPU culling compute shader
            data.detailMgr->ExecuteCulling(ctx, fg);
        }
    );

    // Store handles for render pass
    m_instanceBufferHandle = passData.instanceBuffer;
    m_indirectDrawHandle = passData.indirectBuffer;
}

void CDetailManager::ExecuteCulling(ng::RenderContext* ctx, const framegraph::FrameGraph& fg) {
    // ═══════════════════════════════════════════════════════
    //  BIND CULLING COMPUTE SHADER
    // ═══════════════════════════════════════════════════════

    // Load detail_cull compute shader (already exists in your codebase)
    nvrhi::ShaderHandle cullingCS = LoadComputeShader("detail_cull.cs");

    nvrhi::ComputePipelineDesc pipelineDesc;
    pipelineDesc.computeShader = cullingCS.Get();

    nvrhi::ComputePipelineHandle pipeline =
        ctx->GetDevice()->createComputePipeline(pipelineDesc);

    ctx->SetComputePipeline(pipeline.Get());

    // ═══════════════════════════════════════════════════════
    //  BIND RESOURCES
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingSetDesc bindingDesc;
    bindingDesc.bindings.push_back(
        nvrhi::BindingSetItem::StructuredBuffer_UAV(0, m_instanceBuffer));
    bindingDesc.bindings.push_back(
        nvrhi::BindingSetItem::StructuredBuffer_UAV(1, m_indirectDrawBuffer));
    bindingDesc.bindings.push_back(
        nvrhi::BindingSetItem::StructuredBuffer_UAV(2, m_visibleCountBuffer));

    // TODO: Bind Hi-Z depth texture for occlusion culling

    nvrhi::BindingSetHandle bindingSet =
        ctx->GetDevice()->createBindingSet(bindingDesc, /* layout */);

    ctx->SetComputeBindingSet(bindingSet.Get());

    // ═══════════════════════════════════════════════════════
    //  DISPATCH COMPUTE
    // ═══════════════════════════════════════════════════════

    // Clear visible count to 0
    u32 zero = 0;
    ctx->WriteBuffer(m_visibleCountBuffer.Get(), &zero, sizeof(u32));

    // Dispatch culling (64 threads per group)
    u32 totalInstances = m_grassInstances.size();
    u32 groupCount = (totalInstances + 63) / 64;
    ctx->Dispatch(groupCount, 1, 1);
}

void CDetailManager::SetupRenderPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle colorTarget,
    framegraph::VirtualResourceHandle depthTarget)
{
    using namespace framegraph;

    struct GrassRenderData {
        VirtualResourceHandle color;
        VirtualResourceHandle depth;
        VirtualResourceHandle instanceBuffer;
        VirtualResourceHandle indirectBuffer;
        CDetailManager* detailMgr;
    };

    auto& passData = fg.addCallbackPass<GrassRenderData>(
        "GrassRender",

        // Setup lambda
        [&](FrameGraph& builder, PassHandle passHandle, GrassRenderData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.detailMgr = this;

            // Write to color and depth
            data.color = passBuilder.write(colorTarget, ResourceState::RenderTarget);
            data.depth = passBuilder.write(depthTarget, ResourceState::DepthStencilWrite);

            // Read instance buffer and indirect draw buffer
            data.instanceBuffer = passBuilder.read(
                m_instanceBufferHandle, ResourceState::VertexBuffer);
            data.indirectBuffer = passBuilder.read(
                m_indirectDrawHandle, ResourceState::IndirectArgument);
        },

        // Execute lambda
        [](const GrassRenderData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            data.detailMgr->ExecuteRender(ctx, fg);
        }
    );
}

void CDetailManager::ExecuteRender(ng::RenderContext* ctx, const framegraph::FrameGraph& fg) {
    // ═══════════════════════════════════════════════════════
    //  RENDER GRASS (INDIRECT DRAW)
    // ═══════════════════════════════════════════════════════

    // Bind grass PSO (forward shader with grass-specific features)
    // TODO: Get from MaterialCache

    // Bind instance buffer as vertex buffer
    ctx->SetVertexBuffer(1, m_instanceBuffer.Get(), 0);  // Slot 1 = instance data

    // Draw from indirect buffer (GPU-driven!)
    // GPU reads draw commands from m_indirectDrawBuffer
    // Only visible instances are drawn (culled by compute shader)
    ctx->DrawIndirect(
        m_indirectDrawBuffer.Get(),
        0,  // Offset
        MAX_GRASS_BATCHES,  // Max count
        sizeof(nvrhi::DrawArguments)
    );
}
```

---

### Task 3.5.2: Update detail_cull Compute Shader

**File:** `res/gamedata/shaders/r5/compute/detail_cull.cs` (EXISTING - needs update)

**Current:** DX11-specific HLSL
**Update:** Add frustum culling + Hi-Z occlusion culling

```hlsl
// Grass culling compute shader (GPU-driven)

// ═══════════════════════════════════════════════════════
//  INPUTS
// ═══════════════════════════════════════════════════════

// All grass instances (CPU uploaded once)
struct GrassInstance {
    float3 position;       // World position
    float scale;           // Size multiplier
    float2 texcoord;       // Texture variation
    float rotation;        // Random rotation
    uint grassType;        // Grass mesh variant
};
StructuredBuffer<GrassInstance> g_AllInstances : register(t0);

// Hi-Z depth pyramid (for occlusion culling)
Texture2D<float> g_HiZDepth : register(t1);

// ═══════════════════════════════════════════════════════
//  OUTPUTS
// ═══════════════════════════════════════════════════════

// Visible instances (GPU writes)
RWStructuredBuffer<GrassInstance> g_VisibleInstances : register(u0);

// Indirect draw arguments (GPU writes)
struct DrawArguments {
    uint indexCount;
    uint instanceCount;
    uint startIndex;
    uint baseVertex;
    uint startInstance;
};
RWStructuredBuffer<DrawArguments> g_IndirectDraw : register(u1);

// Atomic counter for visible instances
RWStructuredBuffer<uint> g_VisibleCount : register(u2);

// ═══════════════════════════════════════════════════════
//  CONSTANTS
// ═══════════════════════════════════════════════════════

cbuffer CullingParams : register(b0) {
    float4x4 viewProj;           // View-projection matrix
    float4 frustumPlanes[6];     // Frustum planes (world space)
    float4 cameraPosition;       // Camera world position

    uint totalInstanceCount;     // Total grass instances
    uint grassIndexCount;        // Indices per grass mesh
    float grassRadius;           // Bounding sphere radius
    float detailDistance;        // Max render distance
};

// ═══════════════════════════════════════════════════════
//  FRUSTUM CULLING
// ═══════════════════════════════════════════════════════

bool FrustumCullSphere(float3 center, float radius) {
    // Test sphere against all 6 frustum planes
    for (int i = 0; i < 6; i++) {
        float dist = dot(frustumPlanes[i].xyz, center) + frustumPlanes[i].w;
        if (dist < -radius) {
            return false;  // Outside frustum
        }
    }
    return true;  // Inside or intersecting
}

// ═══════════════════════════════════════════════════════
//  DISTANCE CULLING
// ═══════════════════════════════════════════════════════

bool DistanceCull(float3 position) {
    float dist = distance(position, cameraPosition.xyz);
    return dist <= detailDistance;
}

// ═══════════════════════════════════════════════════════
//  HI-Z OCCLUSION CULLING
// ═══════════════════════════════════════════════════════

bool OcclusionCull(float3 worldPos, float radius) {
    // Transform to clip space
    float4 clipPos = mul(viewProj, float4(worldPos, 1.0));

    // Perspective divide
    float3 ndc = clipPos.xyz / clipPos.w;

    // Check if behind camera
    if (ndc.z < 0.0 || ndc.z > 1.0) return false;

    // Convert NDC to UV (0-1)
    float2 uv = ndc.xy * 0.5 + 0.5;
    uv.y = 1.0 - uv.y;  // Flip Y

    // Sample Hi-Z at appropriate mip level
    // Use conservative mip (covers multiple pixels)
    float depthBufferValue = g_HiZDepth.SampleLevel(
        SamplerState{Filter=MIN_MAG_MIP_POINT},
        uv,
        0  // Mip level (TODO: calculate based on screen coverage)
    );

    // Conservative depth test (add bias for safety)
    float depthBias = 0.001;
    return ndc.z > (depthBufferValue + depthBias);  // Visible if further than depth
}

// ═══════════════════════════════════════════════════════
//  MAIN COMPUTE SHADER
// ═══════════════════════════════════════════════════════

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID) {
    uint instanceID = dtID.x;

    // Bounds check
    if (instanceID >= totalInstanceCount) return;

    // Load instance data
    GrassInstance instance = g_AllInstances[instanceID];

    // ═══════════════════════════════════════════════════════
    //  CULLING TESTS
    // ═══════════════════════════════════════════════════════

    // 1. Distance culling (cheap, early out)
    if (!DistanceCull(instance.position)) return;

    // 2. Frustum culling (cheap, parallel)
    if (!FrustumCullSphere(instance.position, grassRadius * instance.scale)) return;

    // 3. Occlusion culling (expensive, but accurate)
    if (!OcclusionCull(instance.position, grassRadius * instance.scale)) return;

    // ═══════════════════════════════════════════════════════
    //  INSTANCE IS VISIBLE - ADD TO DRAW LIST
    // ═══════════════════════════════════════════════════════

    // Allocate slot in visible array (atomic increment)
    uint visibleIndex;
    InterlockedAdd(g_VisibleCount[0], 1, visibleIndex);

    // Write instance to visible list
    g_VisibleInstances[visibleIndex] = instance;

    // Update indirect draw arguments
    // (Multiple threads may write to same draw call - use atomics)
    InterlockedAdd(g_IndirectDraw[0].instanceCount, 1);

    // Set draw parameters (first thread only)
    if (visibleIndex == 0) {
        g_IndirectDraw[0].indexCount = grassIndexCount;
        g_IndirectDraw[0].startIndex = 0;
        g_IndirectDraw[0].baseVertex = 0;
        g_IndirectDraw[0].startInstance = 0;
    }
}
```

---

### Task 3.5.3: Extend to General Geometry Culling

**Goal:** Use same system for ALL geometry, not just grass

**File:** `src/Layers/xrRender/GPUCullingManager.h` (NEW)

```cpp
#pragma once
#include "xrCore/xrCore.h"

namespace xray::render {

// GPU culling for arbitrary geometry
class GPUCullingManager {
public:
    GPUCullingManager();
    ~GPUCullingManager();

    void Initialize(ng::RenderDevice* device);

    // Upload all scene objects to GPU (once per frame)
    void UploadSceneObjects(const xr_vector<GeometryBatch>& batches);

    // Setup culling pass in FrameGraph
    void SetupCullingPass(
        framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle depthBuffer);

    // Get culled batches for rendering
    const xr_vector<GeometryBatch>& GetVisibleBatches() const { return m_visibleBatches; }

private:
    nvrhi::BufferHandle m_objectBuffer;      // All objects
    nvrhi::BufferHandle m_visibleBuffer;     // Visible objects (GPU writes)
    nvrhi::BufferHandle m_indirectBuffer;    // Indirect draw commands
    nvrhi::BufferHandle m_visibleCountBuffer;

    xr_vector<GeometryBatch> m_visibleBatches;  // CPU-side copy (for debugging)
};

} // namespace xray::render
```

**Usage in FrameGraphRenderer:**
```cpp
void FrameGraphRenderer::SetupFrameGraphPasses() {
    // ... depth prepass ...

    // GPU CULLING (NEW - Phase 3.5)
    m_gpuCullingManager->UploadSceneObjects(m_geometryCollector->GetBatches());
    m_gpuCullingManager->SetupCullingPass(*m_framegraph, depthBuffer);

    // Forward pass now renders ONLY visible objects
    auto forwardOutputs = passes::setupForwardColorPass(
        *m_framegraph,
        m_device,
        depthBuffer,
        m_gpuCullingManager->GetVisibleBatches(),  // Culled on GPU!
        // ...
    );
}
```

---

### Task 3.5.4: Hi-Z Pyramid Generation

**For accurate occlusion culling, generate depth pyramid (Hi-Z):**

**File:** `res/gamedata/shaders/r5/compute/hiz_build.cs`

```hlsl
// Hi-Z pyramid generation (downsample depth buffer)

Texture2D<float> g_InputDepth : register(t0);
RWTexture2D<float> g_OutputHiZ : register(u0);

SamplerState g_PointSampler : register(s0);

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID) {
    // Read 2x2 quad from higher mip
    float2 uv = (dtID.xy * 2.0 + 0.5) / float2(textureSize);

    float d0 = g_InputDepth.SampleLevel(g_PointSampler, uv, mipLevel, int2(0, 0));
    float d1 = g_InputDepth.SampleLevel(g_PointSampler, uv, mipLevel, int2(1, 0));
    float d2 = g_InputDepth.SampleLevel(g_PointSampler, uv, mipLevel, int2(0, 1));
    float d3 = g_InputDepth.SampleLevel(g_PointSampler, uv, mipLevel, int2(1, 1));

    // Take farthest depth (conservative - avoids false occlusion)
    float maxDepth = max(max(d0, d1), max(d2, d3));

    // Write to output mip
    g_OutputHiZ[dtID.xy] = maxDepth;
}
```

---

### Success Metrics (Phase 3.5)

**Validation:**
1. RenderDoc: Check "GrassCulling" compute pass
   - Dispatches with (instance_count / 64) thread groups
   - Writes to visible instance buffer
   - Atomic counter increments correctly

2. Check "GrassRender" pass
   - Uses `DrawIndirect` (not `Draw`)
   - Only visible instances rendered
   - FPS improvement vs CPU culling

3. Performance comparison:
   - **Before (CPU culling):** 10K grass instances = 90fps (2-5ms CPU culling)
   - **After (GPU culling):** 100K grass instances = 120fps (0.3ms GPU culling)
   - **Gain: 10× more geometry at higher framerate**

**Expected Performance:**
- DetailManager modernization: +0.5ms (one-time refactor cost)
- GPU culling compute: +0.3ms
- Grass rendering (indirect): +1-2ms (10× more grass than before)
- **Net result: More grass at similar/better performance**

**Quality:**
- Grass density 10× higher than original engine
- No pop-in (smooth LOD transitions via distance culling)
- No overdraw waste (occlusion culling catches hidden grass)

---

## Phase 4: Cascaded Shadow Maps (Weeks 9-10)

**Goal:** Add dynamic shadows for sun light using cascaded shadow maps (CSM)

**Note:** Shadow pass also benefits from GPU culling (Phase 3.5) - shadow casters are culled per cascade.

This is a complex phase - shadows are expensive and tricky to get right.

### Overview: Cascaded Shadow Maps

**Problem:** Single shadow map doesn't work for large outdoor scenes
- Near camera: Need high resolution (see detail on nearby objects)
- Far from camera: Can use low resolution (distant shadows less important)
- Single shadow map: Either waste resolution far away, or lack detail nearby

**Solution:** Multiple shadow maps at different distances (cascades)
- Cascade 0: 0-10m (high detail, small area)
- Cascade 1: 10-50m (medium detail, medium area)
- Cascade 2: 50-150m (low detail, large area)
- Cascade 3: 150-500m (lowest detail, huge area)

### Architecture

```
Shadow Map Atlas (4096x4096)
┌────────┬────────┬────────┬────────┐
│ Casc 0 │ Casc 1 │ Casc 2 │ Casc 3 │
│ 1024²  │ 1024²  │ 1024²  │ 1024²  │
│ 0-10m  │ 10-50m │ 50-150m│150-500m│
└────────┴────────┴────────┴────────┘

Each cascade:
1. Compute light-space frustum
2. Render geometry from light POV
3. Store depth in shadow map
4. Forward pass samples appropriate cascade
```

---

### Task 4.1: Create ShadowPassSetup.cpp

**File:** `src/Layers/xrRender/FrameGraphPasses/ShadowPassSetup.h`

```cpp
#pragma once
#include "Layers/xrRender/FrameGraph/FrameGraph.h"

namespace xray::render {
    namespace ng { class RenderDevice; }
    class GeometryCollector;
    class MaterialCache;
}

namespace xray::render::passes {

// Shadow cascade outputs
struct ShadowCascadeOutputs {
    framegraph::VirtualResourceHandle shadowAtlas;  // 4096x4096 shadow map atlas
    Fmatrix cascadeMatrices[4];                     // Shadow view-projection matrices
    Fvector4 cascadeSplits;                         // Split distances (x, y, z, w)
};

// Setup cascaded shadow map pass
ShadowCascadeOutputs setupCascadedShadowPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    const Fvector& sunDirection,
    u32 atlasSize = 4096);

} // namespace xray::render::passes
```

**Implementation:** `ShadowPassSetup.cpp`

```cpp
#include "stdafx.h"
#include "ShadowPassSetup.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"

namespace xray::render::passes {

// Compute cascade split distances (PSSM - Practical Split Scheme)
Fvector4 ComputeCascadeSplits(float nearPlane, float farPlane) {
    // Split scheme: Logarithmic + linear blend (λ = 0.5)
    // This provides good balance between near detail and far coverage

    float lambda = 0.5f;  // Blend factor (0 = uniform, 1 = log)
    float ratio = farPlane / nearPlane;

    Fvector4 splits;
    for (int i = 0; i < 4; i++) {
        float p = (i + 1) / 4.0f;

        // Logarithmic split
        float logSplit = nearPlane * pow(ratio, p);

        // Linear split
        float linearSplit = nearPlane + (farPlane - nearPlane) * p;

        // Blend
        float split = lambda * logSplit + (1.0f - lambda) * linearSplit;

        splits[i] = split;
    }

    return splits;
}

// Compute cascade view-projection matrix
Fmatrix ComputeCascadeMatrix(
    const Fvector& sunDirection,
    const CFrustum& viewFrustum,
    float nearDist,
    float farDist)
{
    // Extract frustum corners in world space
    Fvector corners[8];
    // ... extract 8 corners from view frustum between nearDist and farDist
    // This is complex - see reference implementation

    // Compute frustum center
    Fvector center = {};
    for (int i = 0; i < 8; i++) {
        center.add(corners[i]);
    }
    center.div(8.0f);

    // Compute light view matrix (look from sun towards frustum)
    Fvector lightPos = center;
    lightPos.mad(center, sunDirection, -100.0f);  // Move back along sun dir

    Fmatrix lightView;
    lightView.build_camera_dir(lightPos, sunDirection, Fvector().set(0, 1, 0));

    // Transform corners to light space
    Fvector lightSpaceCorners[8];
    for (int i = 0; i < 8; i++) {
        lightView.transform_tiny(lightSpaceCorners[i], corners[i]);
    }

    // Find AABB in light space
    Fvector minBounds(FLT_MAX, FLT_MAX, FLT_MAX);
    Fvector maxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (int i = 0; i < 8; i++) {
        minBounds.min(lightSpaceCorners[i]);
        maxBounds.max(lightSpaceCorners[i]);
    }

    // Extend Z range to include shadow casters behind frustum
    minBounds.z -= 100.0f;  // Extend backwards to catch occluders

    // Build orthographic projection for this cascade
    Fmatrix lightProj;
    lightProj.build_projection_ortho(
        maxBounds.x - minBounds.x,  // width
        maxBounds.y - minBounds.y,  // height
        minBounds.z,                 // zNear
        maxBounds.z                  // zFar
    );

    // Combine
    Fmatrix cascadeMatrix;
    cascadeMatrix.mul(lightProj, lightView);

    // TODO: Snap to texel grid to prevent shimmering

    return cascadeMatrix;
}

ShadowCascadeOutputs setupCascadedShadowPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    const Fvector& sunDirection,
    u32 atlasSize)
{
    using namespace framegraph;

    struct ShadowPassData {
        VirtualResourceHandle shadowAtlas;
        ng::RenderDevice* device;
        const GeometryCollector* geometry;
        MaterialCache* materialCache;
        Fvector sunDirection;
        u32 atlasSize;
        Fmatrix cascadeMatrices[4];
        Fvector4 cascadeSplits;
    };

    auto& passData = fg.addCallbackPass<ShadowPassData>(
        "CascadedShadows",

        // Setup lambda
        [&](FrameGraph& builder, PassHandle passHandle, ShadowPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;
            data.sunDirection = sunDirection;
            data.atlasSize = atlasSize;

            // Create shadow map atlas (depth-only texture)
            ResourceDesc atlasDesc;
            atlasDesc.type = ResourceDesc::Type::Texture2D;
            atlasDesc.width = atlasSize;
            atlasDesc.height = atlasSize;
            atlasDesc.format = nvrhi::Format::D32;  // 32-bit depth
            atlasDesc.isDepthStencil = true;
            atlasDesc.debugName = "rt_ShadowAtlas";

            data.shadowAtlas = builder.CreateTexture("rt_ShadowAtlas", atlasDesc);
            passBuilder.write(data.shadowAtlas, ResourceState::DepthStencilWrite);

            // Compute cascade splits
            data.cascadeSplits = ComputeCascadeSplits(0.1f, 500.0f);

            // Compute cascade matrices (in setup, so they're available to forward pass)
            CFrustum viewFrustum;
            viewFrustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_ALL);

            float prevSplit = 0.1f;
            for (int i = 0; i < 4; i++) {
                float split = data.cascadeSplits[i];
                data.cascadeMatrices[i] = ComputeCascadeMatrix(
                    sunDirection,
                    viewFrustum,
                    prevSplit,
                    split
                );
                prevSplit = split;
            }
        },

        // Execute lambda
        [](const ShadowPassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            auto* shadowAtlas = fg.GetPhysicalTexture(data.shadowAtlas);
            if (!shadowAtlas || !data.geometry) return;

            const auto& batches = data.geometry->GetBatches();
            if (batches.empty()) return;

            // ═══════════════════════════════════════════════════════
            //  RENDER ALL 4 CASCADES TO ATLAS
            // ═══════════════════════════════════════════════════════

            u32 cascadeSize = data.atlasSize / 4;  // 1024x1024 per cascade

            for (int cascadeIndex = 0; cascadeIndex < 4; cascadeIndex++) {
                // Setup viewport for this cascade
                ng::Viewport viewport;
                viewport.x = (float)(cascadeIndex * cascadeSize);
                viewport.y = 0.0f;
                viewport.width = (float)cascadeSize;
                viewport.height = (float)cascadeSize;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                // Setup scissor
                ng::Rect scissor;
                scissor.x = cascadeIndex * cascadeSize;
                scissor.y = 0;
                scissor.width = cascadeSize;
                scissor.height = cascadeSize;

                // Setup render pass (depth-only)
                ng::RenderPassDesc passDesc;
                passDesc.depthStencil = shadowAtlas;
                passDesc.numRenderTargets = 0;
                passDesc.clearDepth = true;
                passDesc.clearValue.depth = 1.0f;

                ctx->BeginRenderPass(passDesc);
                ctx->SetViewport(viewport);
                ctx->SetScissor(scissor);

                // Render geometry from light POV
                // TODO: Use shadow PSO (similar to depth PSO)
                // TODO: Cull geometry outside cascade frustum

                for (const auto& batch : batches) {
                    // Get shadow PSO
                    // Set shadow matrix constant (cascade VP matrix)
                    // Draw
                }

                ctx->EndRenderPass();
            }
        }
    );

    // Return outputs
    ShadowCascadeOutputs outputs;
    outputs.shadowAtlas = passData.shadowAtlas;
    for (int i = 0; i < 4; i++) {
        outputs.cascadeMatrices[i] = passData.cascadeMatrices[i];
    }
    outputs.cascadeSplits = passData.cascadeSplits;

    return outputs;
}

} // namespace xray::render::passes
```

---

### Task 4.2: Create Shadow Sampling Shader Code

**File:** `res/gamedata/shaders/r5/shared/shadow_common.h`

```hlsl
#ifndef SHADOW_COMMON_H
#define SHADOW_COMMON_H

// Shadow map atlas texture
Texture2D t_ShadowAtlas : register(t10);  // Reserve slot 10 for shadows
SamplerComparisonState s_ShadowSampler : register(s10);

// From ForwardGlobals CB:
// float4x4 shadow_matrices[4];
// float4 cascade_splits;

// Select cascade based on view-space depth
int SelectCascade(float viewDepth) {
    if (viewDepth < cascade_splits.x) return 0;
    if (viewDepth < cascade_splits.y) return 1;
    if (viewDepth < cascade_splits.z) return 2;
    return 3;
}

// Compute shadow atlas UV for a specific cascade
float2 ComputeShadowUV(float3 worldPos, int cascadeIndex) {
    // Transform world position to light clip space
    float4 lightClipPos = mul(shadow_matrices[cascadeIndex], float4(worldPos, 1.0));

    // Project to NDC (-1 to 1)
    float3 lightNDC = lightClipPos.xyz / lightClipPos.w;

    // Convert to UV (0 to 1)
    float2 cascadeUV = lightNDC.xy * 0.5 + 0.5;
    cascadeUV.y = 1.0 - cascadeUV.y;  // Flip Y (D3D convention)

    // Map to cascade region in atlas
    float cascadeOffset = cascadeIndex * 0.25;  // 4 cascades = 0.25 width each
    cascadeUV.x = cascadeUV.x * 0.25 + cascadeOffset;

    return cascadeUV;
}

// Sample shadow map with PCF (Percentage Closer Filtering)
float SampleShadowPCF(float3 worldPos, float3 normal, int cascadeIndex, int filterSize) {
    // Compute UV in shadow atlas
    float2 shadowUV = ComputeShadowUV(worldPos, cascadeIndex);

    // Get depth in light space
    float4 lightClipPos = mul(shadow_matrices[cascadeIndex], float4(worldPos, 1.0));
    float lightDepth = lightClipPos.z / lightClipPos.w;

    // Depth bias (prevent shadow acne)
    float bias = 0.001;  // Tune this value
    float biasedDepth = lightDepth - bias;

    // PCF kernel (2x2, 3x3, or 5x5)
    float shadow = 0.0;
    float samples = 0.0;

    int halfSize = filterSize / 2;
    float texelSize = 1.0 / 4096.0;  // Atlas is 4096x4096

    for (int y = -halfSize; y <= halfSize; y++) {
        for (int x = -halfSize; x <= halfSize; x++) {
            float2 offset = float2(x, y) * texelSize;
            float2 sampleUV = shadowUV + offset;

            // Hardware PCF comparison
            shadow += t_ShadowAtlas.SampleCmpLevelZero(
                s_ShadowSampler,
                sampleUV,
                biasedDepth
            ).r;

            samples += 1.0;
        }
    }

    return shadow / samples;
}

// Main shadow function (call from pixel shader)
float ComputeShadow(float3 worldPos, float3 normal, float viewDepth) {
    // Select cascade
    int cascadeIndex = SelectCascade(viewDepth);

    // Sample shadow map with PCF
    // Filter size: 2 (2x2), 3 (3x3), or 5 (5x5)
    // Larger = softer shadows, more expensive
    return SampleShadowPCF(worldPos, normal, cascadeIndex, 3);  // 3x3 PCF
}

#endif // SHADOW_COMMON_H
```

---

### Task 4.3: Integrate Shadows into Forward Shader

**File:** `res/gamedata/shaders/r5/forward/forward_base.ps`

Update pixel shader to sample shadows:

```hlsl
#include "../shared/shadow_common.h"

PSOutput main(PSInput input) {
    // ... sample textures, prepare surface ...

    // ═══════════════════════════════════════════════════════
    //  SHADOW SAMPLING (NEW)
    // ═══════════════════════════════════════════════════════

    // Compute view-space depth for cascade selection
    float4 viewPos = mul(m_V, float4(input.worldPos, 1.0));
    float viewDepth = -viewPos.z;  // Negated Z in view space

    // Sample shadow map
    float shadow = ComputeShadow(input.worldPos, normal, viewDepth);

    // ═══════════════════════════════════════════════════════
    //  LIGHTING (with shadows)
    // ═══════════════════════════════════════════════════════

    lighting += EvaluateDirectionalLight(
        surface,
        sun_direction.xyz,
        sun_color.rgb * sun_color.a,
        shadow  // NOW USING REAL SHADOWS!
    );

    // ... rest of shader ...
}
```

---

### Task 4.4: Wire Up Shadow Pass in FrameGraph

**File:** `src/Layers/xrRender/r_FrameGraphRenderer.cpp`

Update pipeline:

```cpp
void FrameGraphRenderer::SetupFrameGraphPasses() {
    // ... depth prepass ...

    // ═══════════════════════════════════════════════════════
    //  2. SHADOW MAPS (NEW - Phase 4)
    // ═══════════════════════════════════════════════════════

    // Extract sun direction from environment
    Fvector sunDirection(0.577f, 0.577f, 0.577f);
    if (g_pGamePersistent && g_pGamePersistent->Environment().CurrentEnv) {
        sunDirection = g_pGamePersistent->Environment().CurrentEnv->sun_dir;
    }

    auto shadowOutputs = passes::setupCascadedShadowPass(
        *m_framegraph,
        m_device,
        m_geometryCollector.get(),
        m_materialCache.get(),
        sunDirection,
        4096  // Atlas size
    );

    // ═══════════════════════════════════════════════════════
    //  3. FORWARD COLOR (now reads shadows)
    // ═══════════════════════════════════════════════════════

    auto forwardOutputs = passes::setupForwardColorPass(
        *m_framegraph,
        m_device,
        depthBuffer,
        shadowOutputs,  // PASS SHADOW DATA!
        m_geometryCollector.get(),
        m_materialCache.get(),
        width,
        height
    );

    // ... rest of pipeline ...
}
```

Update `ForwardColorPassSetup` to accept shadow inputs and bind shadow atlas texture.

---

### Success Metrics (Phase 4)

**Validation:**
1. RenderDoc: Check "CascadedShadows" pass
   - 4 viewports in shadow atlas (1024x1024 each)
   - Geometry rendered from light POV
   - Depth values written correctly

2. Check "ForwardColor" pass
   - Shadow atlas bound to t10
   - Pixel shader samples shadow map
   - Shadow term affects lighting (0.0 = shadowed, 1.0 = lit)

3. Visual check:
   - Dynamic shadows appear on ground
   - Shadows follow sun direction
   - No severe shadow acne or peter-panning
   - Cascade transitions smooth (minimal popping)

**Performance:**
- Shadow pass: +2-3ms
- Forward pass: +0.5ms (shadow sampling)
- **Total: ~12-15ms slower than Phase 3**
- Expected FPS: 80-100fps (before optimization)

**Known Issues to Fix:**
- Shadow acne (bias tuning)
- Peter-panning (bias too high)
- Cascade popping (blend between cascades)
- Shimmering (texel snapping needed)

---

## Phase 5: Clustered Light Culling (Weeks 11-13)

**Goal:** Add support for 100+ dynamic point/spot lights efficiently

This phase enables the "plus" in Forward+ - efficient handling of many lights.

### Overview: Clustered Shading

**Problem:** Evaluating N lights for every pixel is expensive
- 100 lights × 2M pixels = 200M light evaluations per frame
- Most lights don't affect most pixels (outside radius)

**Solution:** Pre-compute which lights affect which screen regions
- Divide screen into 3D grid of "clusters" (tiles in X/Y, slices in Z)
- Per cluster: Test which lights overlap (sphere frustum test)
- Per pixel: Only evaluate lights in its cluster

**Example:**
```
Screen: 1920x1080
Cluster grid: 16×16×24 (X, Y, Z)
Total clusters: 6144

Each cluster:
- Screen tile: 120×67 pixels
- Depth slice: View space Z range

Per frame:
1. Compute shader tests 100 lights vs 6144 clusters
2. Builds light index list per cluster
3. Forward shader reads: "Cluster 3421 has lights [5, 17, 42]"
4. Only evaluate those 3 lights instead of all 100!
```

---

### Task 5.1: Define Light Data Structures

**File:** `src/Layers/xrRender/Lighting/LightData.h` (NEW)

```cpp
#pragma once
#include "xrCore/xrCore.h"

namespace xray::render {

// ═══════════════════════════════════════════════════════
//  GPU LIGHT DATA (tightly packed for upload)
// ═══════════════════════════════════════════════════════

enum class LightType : u32 {
    Directional = 0,
    Point = 1,
    Spot = 2
};

// GPU light structure (64 bytes, cache-friendly)
struct alignas(16) GPULight {
    Fvector4 position_and_range;    // xyz = position, w = range
    Fvector4 direction_and_type;    // xyz = direction, w = type
    Fvector4 color_and_intensity;   // xyz = color, w = intensity
    Fvector4 spot_params;           // x = inner_angle, y = outer_angle, z/w = unused

    // Total: 64 bytes
};

static_assert(sizeof(GPULight) == 64, "GPULight must be 64 bytes");

// ═══════════════════════════════════════════════════════
//  CLUSTER DATA
// ═══════════════════════════════════════════════════════

// Cluster grid parameters
struct ClusterGridParams {
    u32 dimX = 16;      // Horizontal tiles
    u32 dimY = 16;      // Vertical tiles
    u32 dimZ = 24;      // Depth slices
    float zNear = 0.1f;
    float zFar = 500.0f;

    u32 GetTotalClusters() const { return dimX * dimY * dimZ; }
};

// Per-cluster light list (variable size)
struct ClusterLightList {
    u32 offset;   // Offset into global light index buffer
    u32 count;    // Number of lights in this cluster
};

// ═══════════════════════════════════════════════════════
//  LIGHT MANAGER
// ═══════════════════════════════════════════════════════

class LightManager {
public:
    LightManager();
    ~LightManager();

    // Add lights to scene
    void AddPointLight(const Fvector& position, const Fvector& color, float range, float intensity);
    void AddSpotLight(const Fvector& position, const Fvector& direction, const Fvector& color,
                      float range, float intensity, float innerAngle, float outerAngle);

    // Frame management
    void BeginFrame();
    void EndFrame();

    // Getters
    const xr_vector<GPULight>& GetLights() const { return m_lights; }
    u32 GetLightCount() const { return (u32)m_lights.size(); }

    // Upload to GPU buffer
    void UploadToGPU(nvrhi::IBuffer* lightBuffer, ng::RenderContext* ctx);

private:
    xr_vector<GPULight> m_lights;
};

} // namespace xray::render
```

**Implementation:** `LightData.cpp`

```cpp
#include "stdafx.h"
#include "LightData.h"

namespace xray::render {

LightManager::LightManager() {
    m_lights.reserve(256);  // Pre-allocate for typical scene
}

LightManager::~LightManager() = default;

void LightManager::BeginFrame() {
    m_lights.clear();
}

void LightManager::EndFrame() {
    // Could sort lights by importance/size here
}

void LightManager::AddPointLight(
    const Fvector& position,
    const Fvector& color,
    float range,
    float intensity)
{
    GPULight light = {};
    light.position_and_range.set(position.x, position.y, position.z, range);
    light.direction_and_type.set(0, 0, 0, (float)LightType::Point);
    light.color_and_intensity.set(color.x, color.y, color.z, intensity);

    m_lights.push_back(light);
}

void LightManager::AddSpotLight(
    const Fvector& position,
    const Fvector& direction,
    const Fvector& color,
    float range,
    float intensity,
    float innerAngle,
    float outerAngle)
{
    GPULight light = {};
    light.position_and_range.set(position.x, position.y, position.z, range);
    light.direction_and_type.set(direction.x, direction.y, direction.z, (float)LightType::Spot);
    light.color_and_intensity.set(color.x, color.y, color.z, intensity);
    light.spot_params.set(innerAngle, outerAngle, 0, 0);

    m_lights.push_back(light);
}

void LightManager::UploadToGPU(nvrhi::IBuffer* lightBuffer, ng::RenderContext* ctx) {
    if (m_lights.empty()) return;

    u32 dataSize = (u32)(m_lights.size() * sizeof(GPULight));
    ctx->WriteBuffer(lightBuffer, m_lights.data(), dataSize);
}

} // namespace xray::render
```

---

### Task 5.2: Create Light Culling Compute Shader

**File:** `res/gamedata/shaders/r5/compute/light_culling.cs`

```hlsl
// Clustered light culling compute shader

// ═══════════════════════════════════════════════════════
//  INPUTS
// ═══════════════════════════════════════════════════════

// Depth buffer (for determining cluster Z bounds)
Texture2D<float> t_Depth : register(t0);

// Light buffer (structured buffer of all lights in scene)
struct GPULight {
    float4 position_and_range;
    float4 direction_and_type;
    float4 color_and_intensity;
    float4 spot_params;
};
StructuredBuffer<GPULight> t_Lights : register(t1);

// ═══════════════════════════════════════════════════════
//  OUTPUTS
// ═══════════════════════════════════════════════════════

// Cluster light list (offset + count per cluster)
struct ClusterLightList {
    uint offset;
    uint count;
};
RWStructuredBuffer<ClusterLightList> u_ClusterLightList : register(u0);

// Global light index buffer (flat list of light indices)
RWStructuredBuffer<uint> u_LightIndices : register(u1);

// Atomic counter for light index allocation
RWStructuredBuffer<uint> u_IndexCounter : register(u2);

// ═══════════════════════════════════════════════════════
//  CONSTANTS
// ═══════════════════════════════════════════════════════

cbuffer ClusterParams : register(b0) {
    uint cluster_dimX;
    uint cluster_dimY;
    uint cluster_dimZ;
    uint light_count;

    float4x4 inv_projection;  // Inverse projection matrix
    float z_near;
    float z_far;
    float screen_width;
    float screen_height;
};

// ═══════════════════════════════════════════════════════
//  HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════

// Compute cluster index from 3D grid position
uint GetClusterIndex(uint3 clusterPos) {
    return clusterPos.x +
           clusterPos.y * cluster_dimX +
           clusterPos.z * cluster_dimX * cluster_dimY;
}

// Compute cluster Z bounds (view space)
float2 GetClusterZBounds(uint clusterZ) {
    // Exponential depth slicing for better near detail
    float slice = float(clusterZ) / float(cluster_dimZ);
    float nextSlice = float(clusterZ + 1) / float(cluster_dimZ);

    float zMin = z_near * pow(z_far / z_near, slice);
    float zMax = z_near * pow(z_far / z_near, nextSlice);

    return float2(zMin, zMax);
}

// Compute cluster AABB in view space
struct AABB {
    float3 minPos;
    float3 maxPos;
};

AABB GetClusterAABB(uint3 clusterPos) {
    AABB aabb;

    // Screen-space tile bounds
    float tileWidth = screen_width / float(cluster_dimX);
    float tileHeight = screen_height / float(cluster_dimY);

    float minX = float(clusterPos.x) * tileWidth;
    float maxX = float(clusterPos.x + 1) * tileWidth;
    float minY = float(clusterPos.y) * tileHeight;
    float maxY = float(clusterPos.y + 1) * tileHeight;

    // Depth bounds
    float2 zBounds = GetClusterZBounds(clusterPos.z);

    // Reconstruct 4 corners at near and far planes
    // This forms a frustum in view space
    float4 corners[8];

    // Near plane corners (Z = zMin)
    corners[0] = float4(minX, minY, zBounds.x, 1.0);
    corners[1] = float4(maxX, minY, zBounds.x, 1.0);
    corners[2] = float4(minX, maxY, zBounds.x, 1.0);
    corners[3] = float4(maxX, maxY, zBounds.x, 1.0);

    // Far plane corners (Z = zMax)
    corners[4] = float4(minX, minY, zBounds.y, 1.0);
    corners[5] = float4(maxX, minY, zBounds.y, 1.0);
    corners[6] = float4(minX, maxY, zBounds.y, 1.0);
    corners[7] = float4(maxX, maxY, zBounds.y, 1.0);

    // Convert screen space to view space
    for (int i = 0; i < 8; i++) {
        corners[i] = mul(inv_projection, corners[i]);
        corners[i].xyz /= corners[i].w;
    }

    // Compute AABB from frustum corners
    aabb.minPos = float3(1e10, 1e10, 1e10);
    aabb.maxPos = float3(-1e10, -1e10, -1e10);

    for (int j = 0; j < 8; j++) {
        aabb.minPos = min(aabb.minPos, corners[j].xyz);
        aabb.maxPos = max(aabb.maxPos, corners[j].xyz);
    }

    return aabb;
}

// Test if sphere intersects AABB
bool SphereAABBIntersection(float3 sphereCenter, float sphereRadius, AABB aabb) {
    // Find closest point on AABB to sphere center
    float3 closest = clamp(sphereCenter, aabb.minPos, aabb.maxPos);

    // Distance from sphere center to closest point
    float3 diff = sphereCenter - closest;
    float distSq = dot(diff, diff);

    return distSq <= (sphereRadius * sphereRadius);
}

// ═══════════════════════════════════════════════════════
//  MAIN COMPUTE SHADER
// ═══════════════════════════════════════════════════════

// Dispatch with (cluster_dimX, cluster_dimY, cluster_dimZ) threads
[numthreads(1, 1, 1)]
void main(uint3 clusterPos : SV_GroupID) {
    // Get cluster AABB in view space
    AABB clusterAABB = GetClusterAABB(clusterPos);

    // Allocate local light index buffer (shared memory)
    // Max 256 lights per cluster
    groupshared uint localLightIndices[256];
    groupshared uint localLightCount;

    if (all(clusterPos == 0)) {
        localLightCount = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    // Test all lights against this cluster
    for (uint lightIdx = 0; lightIdx < light_count; lightIdx++) {
        GPULight light = t_Lights[lightIdx];

        uint lightType = uint(light.direction_and_type.w);

        // Only test point/spot lights (directional handled separately)
        if (lightType == 0) continue;  // Skip directional

        // Get light position and range
        float3 lightPos = light.position_and_range.xyz;
        float lightRange = light.position_and_range.w;

        // TODO: Transform light position to view space
        // For now, assume already in view space

        // Test sphere-AABB intersection
        if (SphereAABBIntersection(lightPos, lightRange, clusterAABB)) {
            // Add light to local list
            uint localIdx;
            InterlockedAdd(localLightCount, 1, localIdx);

            if (localIdx < 256) {
                localLightIndices[localIdx] = lightIdx;
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    // Allocate space in global light index buffer
    uint clusterIdx = GetClusterIndex(clusterPos);
    uint globalOffset;

    if (localLightCount > 0) {
        InterlockedAdd(u_IndexCounter[0], localLightCount, globalOffset);

        // Write light indices to global buffer
        for (uint i = 0; i < localLightCount; i++) {
            u_LightIndices[globalOffset + i] = localLightIndices[i];
        }
    } else {
        globalOffset = 0;
    }

    // Write cluster light list
    u_ClusterLightList[clusterIdx].offset = globalOffset;
    u_ClusterLightList[clusterIdx].count = localLightCount;
}
```

---

### Task 5.3: Create LightCullingPassSetup.cpp

**File:** `src/Layers/xrRender/FrameGraphPasses/LightCullingPassSetup.h`

```cpp
#pragma once
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/Lighting/LightData.h"

namespace xray::render {
    namespace ng { class RenderDevice; }
}

namespace xray::render::passes {

// Light culling outputs
struct LightCullingOutputs {
    framegraph::VirtualResourceHandle clusterLightList;  // Per-cluster light lists
    framegraph::VirtualResourceHandle lightIndices;      // Global light index buffer
    nvrhi::BufferHandle lightBuffer;                     // GPU light data
};

// Setup clustered light culling pass (compute shader)
LightCullingOutputs setupLightCullingPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle depthBuffer,
    LightManager* lightManager,
    const ClusterGridParams& gridParams);

} // namespace xray::render::passes
```

Implementation details similar to shadow pass - create compute pass, dispatch compute shader, output buffers.

---

### Task 5.4: Update Forward Shader to Read Cluster Data

**File:** `res/gamedata/shaders/r5/forward/forward_base.ps`

```hlsl
// ═══════════════════════════════════════════════════════
//  CLUSTERED LIGHTING INPUTS
// ═══════════════════════════════════════════════════════

// Cluster data
StructuredBuffer<ClusterLightList> t_ClusterLightList : register(t11);
StructuredBuffer<uint> t_LightIndices : register(t12);
StructuredBuffer<GPULight> t_Lights : register(t13);

// ... in pixel shader main():

    // ═══════════════════════════════════════════════════════
    //  CLUSTERED LIGHT EVALUATION
    // ═══════════════════════════════════════════════════════

    // Compute cluster index for this pixel
    uint3 clusterPos;
    clusterPos.x = uint(input.position.x / (screen_width / cluster_dimX));
    clusterPos.y = uint(input.position.y / (screen_height / cluster_dimY));

    float viewZ = -mul(m_V, float4(input.worldPos, 1.0)).z;
    float zSlice = log(viewZ / z_near) / log(z_far / z_near);
    clusterPos.z = uint(zSlice * cluster_dimZ);

    uint clusterIdx = clusterPos.x +
                      clusterPos.y * cluster_dimX +
                      clusterPos.z * cluster_dimX * cluster_dimY;

    // Get light list for this cluster
    ClusterLightList lightList = t_ClusterLightList[clusterIdx];

    // Evaluate all lights in cluster
    for (uint i = 0; i < lightList.count; i++) {
        uint lightIdx = t_LightIndices[lightList.offset + i];
        GPULight light = t_Lights[lightIdx];

        uint lightType = uint(light.direction_and_type.w);

        if (lightType == 1) {
            // Point light
            lighting += EvaluatePointLight(
                surface,
                light.position_and_range.xyz,
                light.color_and_intensity.rgb * light.color_and_intensity.a,
                light.position_and_range.w
            );
        }
        else if (lightType == 2) {
            // Spot light
            lighting += EvaluateSpotLight(
                surface,
                light.position_and_range.xyz,
                light.direction_and_type.xyz,
                light.color_and_intensity.rgb * light.color_and_intensity.a,
                light.position_and_range.w,
                light.spot_params.x,
                light.spot_params.y
            );
        }
    }
```

---

### Success Metrics (Phase 5)

**Validation:**
1. Add 100 point lights to scene
2. RenderDoc: Check "LightCulling" compute pass
   - Dispatches with (16, 16, 24) thread groups
   - Outputs cluster light lists
   - Light index buffer populated

3. Forward pass reads cluster data
   - Only evaluates lights in visible clusters
   - Frame time scales with visible lights, not total lights

**Performance:**
- Light culling: +0.3-0.5ms (async, overlaps with other work)
- Forward pass: +0.1ms per 10 visible lights
- 100 total lights, ~10-20 visible per pixel avg: +1-2ms
- **Expected FPS: 70-90fps with 100 lights**

**Scalability Test:**
- 0 lights: Same as Phase 4
- 50 lights: +0.5ms
- 100 lights: +1-2ms
- 200 lights: +3-5ms (still playable!)

Compare to naive approach (no culling):
- 100 lights: +30-50ms (slideshow)

**Gain: 15-25× faster multi-light evaluation**

---

## Phase 6: Froxel-Based Volumetric Lighting (Weeks 14-16)

**Goal:** Implement native volumetric lighting system with froxel infrastructure

This phase adds AAA-quality volumetric fog, light shafts, and atmospheric effects using a unified volumetric architecture. The system is designed with native volumetric support - particles, fog, and light shafts are all treated as volumetric primitives contributing to the same system.

**Why This Architecture?**
- **Native volumetric support:** System understands volumetrics natively, not as "injection" afterthought
- **Clean abstraction:** `IVolumetricSource` interface - fog, particles, light shafts all equal
- **Lighting inheritance:** Add point light support ONCE, all volumetric sources get it automatically
- **Extensibility:** New volumetric effects (cloud shadows, volumetric GI) are trivial to add
- **Matches industry:** Doom Eternal, UE5 use similar unified volumetric systems

### Overview: Volumetric System Architecture

**Froxel Grid:**
```
Frustum divided into 3D grid of voxels (froxels):
- X/Y: Screen tiles (160×90 for 1920×1440, ~12×16 pixels per froxel)
- Z: Exponential slices (16 slices, more detail near camera)
- Total: 160×90×16 = 230,400 froxels
- Storage: 3D texture (RGBA16F) = 18.4 MB
```

**Rendering Pipeline:**
```
Pass 1: Froxel Injection (compute, multi-source)
├── Each volumetric source contributes density/color
├── Sources: WorldFog, ParticleEmitters, LightShafts
└── Output: Froxel volume (density + albedo per froxel)

Pass 2: Volumetric Lighting (compute)
├── Apply sun + clustered lights to entire volume
├── Shadow maps attenuate light through volume
└── Output: Lit froxel volume (inscattering computed)

Pass 3: Temporal Reprojection (compute, optional)
├── Reproject 75-90% of froxels from previous frame
├── Update only 10-25% per frame (checkerboard pattern)
└── Output: Temporally stable froxel volume

Pass 4: Volumetric Ray March (pixel shader)
├── March rays through froxel volume (32-64 steps)
├── Integrate transmittance and inscattering
└── Output: Composite with forward color buffer
```

**Cost:** ~2.5ms total (0.3ms injection, 0.2ms reprojection, 2.0ms ray march)

---

### Task 6.1: Define Volumetric Source Interface

**File:** `src/Layers/xrRender/Volumetrics/IVolumetricSource.h` (NEW)

```cpp
#pragma once
#include "xrCore/xrCore.h"
#include "nvrhi/nvrhi.h"

namespace xray::render {

// Froxel contribution data
struct FroxelContribution {
    float density;       // Scattering coefficient [0, 1]
    float extinction;    // Absorption coefficient [0, 1]
    Fcolor albedo;       // Scattering color (RGB)
    float anisotropy;    // Phase function parameter [-1, 1]
};

// Abstract volumetric source interface
class IVolumetricSource {
public:
    virtual ~IVolumetricSource() = default;

    // Prepare GPU data (upload to structured buffer)
    virtual void PrepareGPUData(nvrhi::BufferHandle& outBuffer) = 0;

    // Shader to use for this source type
    virtual const char* GetShaderName() const = 0;

    // World-space bounds (for culling)
    virtual void GetWorldBounds(Fvector& outMin, Fvector& outMax) const = 0;

    // Source type identifier (for debugging)
    virtual const char* GetTypeName() const = 0;
};

} // namespace xray::render
```

### Task 6.2: Implement Concrete Volumetric Sources

See `VOLUMETRIC_ARCHITECTURE.md` for full implementation details. Key sources:

**WorldFogSource** - Uniform fog with height falloff
**ParticleEmitterSource** - Localized smoke/fire from particle system
**LightShaftSource** - Sun rays through gaps

Each implements `IVolumetricSource` interface (~30-50 lines of code per source).

---

### Task 6.3: Implement VolumetricRenderer

**File:** `src/Layers/xrRender/Volumetrics/VolumetricRenderer.h` (NEW)

```cpp
namespace xray::render {

class VolumetricRenderer {
public:
    VolumetricRenderer();
    ~VolumetricRenderer();

    void Initialize(ng::RenderDevice* device, u32 screenWidth, u32 screenHeight);

    // Register volumetric sources (called by game systems each frame)
    void RegisterSource(IVolumetricSource* source);
    void ClearSources();

    // Setup FrameGraph passes
    framegraph::VirtualResourceHandle SetupVolumetricPasses(
        framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle shadowMap,
        framegraph::VirtualResourceHandle lightClusterData,
        u32 width,
        u32 height);

private:
    nvrhi::TextureHandle m_froxelVolume;      // Current frame
    nvrhi::TextureHandle m_prevFroxelVolume;  // Previous frame (for temporal)
    xr_vector<IVolumetricSource*> m_sources;

    void SetupFroxelInjectionPass(/* ... */);
    void SetupVolumetricLightingPass(/* ... */);
    void SetupTemporalReprojectionPass(/* ... */);
    void SetupRayMarchingPass(/* ... */);
};

} // namespace xray::render
```

**Key Implementation:**
- Multi-pass compute shader system (injection → lighting → reprojection)
- Each source runs its own compute shader to contribute to froxel volume
- Lighting pass applies sun + clustered lights to ENTIRE volume at once
- Ray marching integrates volume in pixel shader

---

### Task 6.4: Create Froxel Compute Shaders

**File:** `res/gamedata/shaders/r5/volumetric/world_fog.cs`

```hlsl
// World fog contribution to froxel volume

RWTexture3D<float4> u_FroxelVolume : register(u0);

cbuffer WorldFogParams : register(b0) {
    float cb_BaseDensity;
    float cb_HeightFalloff;
    float3 cb_FogColor;
    uint3 cb_FroxelDimensions;
};

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID) {
    if (any(dtID >= cb_FroxelDimensions)) return;

    float3 worldPos = FroxelToWorld(dtID);

    // Height-based fog density
    float heightFactor = exp(-worldPos.y * cb_HeightFalloff);
    float density = cb_BaseDensity * heightFactor;

    // ADD to existing density (multiple sources accumulate)
    u_FroxelVolume[dtID] += float4(density, density, 0, 0);
}
```

**File:** `res/gamedata/shaders/r5/volumetric/apply_lighting.cs`

```hlsl
// Apply lights to entire froxel volume (runs AFTER all sources inject)

Texture2D t_ShadowMap : register(t0);
StructuredBuffer<LightData> g_Lights : register(t1);
StructuredBuffer<uint> g_LightIndices : register(t2);
RWTexture3D<float4> u_FroxelVolume : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID) {
    // Read accumulated density from ALL sources
    float4 froxelData = u_FroxelVolume[dtID];
    float density = froxelData.r;

    if (density < 0.0001) return;  // Empty froxel

    float3 worldPos = FroxelToWorld(dtID);

    // Sun lighting
    float shadowFactor = SampleShadowMap(worldPos);
    float3 sunLight = cb_SunColor.rgb * shadowFactor;

    // Clustered point lights
    uint3 cluster = WorldToCluster(worldPos);
    // ... (evaluate lights in cluster)

    float3 totalLight = sunLight + pointLightContribution;
    float inscattering = luminance(totalLight) * density;

    // Write lit volume (R=density, A=inscattering)
    u_FroxelVolume[dtID] = float4(density, density, 0, inscattering);
}
```

---

### Task 6.5: Implement Temporal Reprojection

**File:** `res/gamedata/shaders/r5/volumetric/temporal_reproject.cs`

```hlsl
// Reproject froxels from previous frame (4-8× performance gain)

Texture3D<float4> t_PrevFroxelVolume : register(t0);
Texture3D<float4> t_CurrentFroxelVolume : register(t1);
RWTexture3D<float4> u_OutputFroxelVolume : register(u0);

cbuffer ReprojectionParams : register(b0) {
    float4x4 cb_PrevViewProj;
    float3 cb_CameraVelocity;
    uint cb_FrameIndex;  // For checkerboard pattern
};

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID) {
    // Checkerboard: Update 1/4 froxels per frame
    bool shouldUpdate = ((dtID.x + dtID.y + dtID.z + cb_FrameIndex) % 4) == 0;

    if (shouldUpdate) {
        // This froxel was recomputed - use current data
        u_OutputFroxelVolume[dtID] = t_CurrentFroxelVolume[dtID];
        return;
    }

    // Reproject from previous frame
    float3 worldPos = FroxelToWorld(dtID);
    float3 prevWorldPos = worldPos - cb_CameraVelocity;

    // Convert to previous frame's froxel UVW
    float3 prevUVW = WorldToFroxelUVW(prevWorldPos, cb_PrevViewProj);

    if (all(prevUVW >= 0.0) && all(prevUVW <= 1.0)) {
        // Sample previous frame (trilinear)
        float4 reprojected = t_PrevFroxelVolume.SampleLevel(s_LinearClamp, prevUVW, 0);
        u_OutputFroxelVolume[dtID] = reprojected;
    } else {
        // Out of bounds - use current frame
        u_OutputFroxelVolume[dtID] = t_CurrentFroxelVolume[dtID];
    }
}
```

---

### Task 6.6: Implement Volumetric Ray Marching

**File:** `res/gamedata/shaders/r5/volumetric/ray_march.ps`

```hlsl
// Integrate fog along view rays

Texture3D<float4> t_FroxelVolume : register(t0);
Texture2D<float> t_SceneDepth : register(t1);

PSOutput main(PSInput input) {
    float2 screenUV = input.texcoord;
    float sceneDepth = t_SceneDepth.Sample(s_PointClamp, screenUV);

    // Reconstruct world position
    float3 rayStart = cb_CameraPos;
    float3 rayEnd = ReconstructWorldPos(screenUV, sceneDepth);
    float3 rayDir = normalize(rayEnd - rayStart);
    float rayLength = length(rayEnd - rayStart);

    // Ray march through volume
    const int numSteps = 32;
    float stepSize = rayLength / numSteps;

    float3 transmittance = float3(1, 1, 1);
    float3 scattering = float3(0, 0, 0);

    for (int i = 0; i < numSteps; i++) {
        float t = (i + 0.5) / numSteps;
        float3 samplePos = rayStart + rayDir * (t * rayLength);

        // Sample froxel volume
        float3 froxelUVW = WorldToFroxelUVW(samplePos);
        float4 froxelData = t_FroxelVolume.SampleLevel(s_LinearClamp, froxelUVW, 0);

        float density = froxelData.r;
        float extinction = froxelData.g;
        float inscatter = froxelData.a;

        // Integrate transmittance (Beer's law)
        float opticalDepth = extinction * stepSize;
        float transmittanceStep = exp(-opticalDepth);

        // Accumulate inscattering
        scattering += inscatter * density * stepSize * transmittance;
        transmittance *= transmittanceStep;
    }

    // Output fog color and opacity
    PSOutput output;
    output.color = float4(scattering, 1.0 - luminance(transmittance));
    return output;
}

### Success Metrics (Phase 6)

**Validation:**
1. RenderDoc: Check volumetric passes
   - "FroxelInjection" compute passes (one per source type)
   - "VolumetricLighting" compute pass (applies lights to volume)
   - "TemporalReproject" compute pass (optional)
   - "VolumetricRayMarch" pixel shader pass

2. Visual check:
   - World fog visible, follows height falloff
   - Light shafts visible (god rays through windows/trees)
   - Fog is lit correctly by sun + point lights
   - Shadows attenuate fog (darker in shadow)

3. Performance:
   - Froxel injection: <0.5ms (all sources)
   - Volumetric lighting: <0.3ms (compute)
   - Temporal reprojection: <0.2ms (if enabled)
   - Ray marching: <2.0ms (pixel shader)
   - **Total: ~2.5ms for volumetrics**

**Quality Targets:**
- No visible banding (16 Z-slices sufficient)
- No temporal ghosting (reprojection stable)
- Smooth depth transitions (exponential slicing)
- Correct multi-scattering approximation

**Memory Usage:**
- Froxel volume (current): 18.4 MB (160×90×16 RGBA16F)
- Froxel volume (previous): 18.4 MB (for temporal)
- Source buffers: ~10 KB (small)
- **Total: ~40 MB**

**Performance Impact:**
- Before: 80-100fps (Phase 5 with clustered lights)
- After: 70-90fps (volumetrics add ~2.5ms)
- Still within 100-120fps target for average scenes

---

## Phase 7: Particle System Integration (Weeks 17-21)

**Goal:** Implement four-tier particle rendering system for maximum quality and flexibility

This phase integrates particles with the volumetric system and adds Unity-style 6-way lighting for hero smoke effects.

**Four-Tier Approach:**
1. **Tier 1: Volumetric Particles** (smoke, fog, fire) → Use Phase 6 froxel system
2. **Tier 2: Hero Smoke** (grenade explosions, important effects) → Unity 6-way lightmaps
3. **Tier 3: Lit Particles** (debris, casings, rain) → Clustered forward lighting
4. **Tier 4: Emissive Particles** (muzzle flashes, sparks, UI) → No lighting (50-70% of particles)

---

### Task 7.1: Core ParticlePassSetup Migration

**Follow existing plan structure** but with updated lighting modes:

**File:** `src/Layers/xrRender/ParticleSystem/ParticleDefinition.h`

```cpp
// Add lighting mode to CPEDef
enum class ParticleLightingMode : u32 {
    Emissive = 0,     // No lighting (default for most particles)
    Lit = 1,          // Simple clustered forward lighting
    Volumetric = 2,   // Inject into froxel volume (Phase 6)
    SixWay = 3        // Baked 6-way lightmaps (hero effects)
};

struct CPEDef {
    // ... existing fields ...
    ParticleLightingMode lightingMode = ParticleLightingMode::Emissive;

    // For SixWay mode:
    shared_str lightmapPosXYZ;  // Texture: +X, +Y, +Z in RGB
    shared_str lightmapNegXYZ;  // Texture: -X, -Y, -Z in RGB
};
```

---

### Task 7.2: Tier 1 - Volumetric Particles (Week 17)

**Integration with Phase 6 volumetrics:**

```cpp
// ParticleManager.cpp
void ParticleManager::UpdateVolumetricParticles(VolumetricRenderer* volumetricRenderer) {
    volumetricRenderer->ClearSources();

    // Register active volumetric particle effects
    for (auto& effect : m_activeEffects) {
        if (effect.lightingMode != ParticleLightingMode::Volumetric) continue;

        // Create temporary emitter source
        ParticleEmitterSource emitter(
            effect.GetPosition(),
            effect.GetRadius(),
            effect.GetDensity(),
            effect.GetColor()
        );

        volumetricRenderer->RegisterSource(&emitter);
    }
}
```

**Expected usage:** 10-20% of particles (large smoke, fog, fire effects)

---

### Task 7.3: Tier 2 - Unity 6-Way Lighting (Weeks 18-19)

**Create 6-way lightmap shader:**

**File:** `res/gamedata/shaders/r5/particles/particle_sixway.ps`

```hlsl
// Unity-style 6-way lighting for hero smoke

Texture2D t_BaseColor : register(t0);
Texture2D t_Lightmap_PosXYZ : register(t1);  // +X, +Y, +Z in RGB
Texture2D t_Lightmap_NegXYZ : register(t2);  // -X, -Y, -Z in RGB
StructuredBuffer<LightData> g_Lights : register(t3);

PSOutput main(PSInput input) {
    float4 albedo = t_BaseColor.Sample(s_Linear, input.texcoord);

    // Sample 6 directional lightmaps
    float3 lightmaps[6];
    float3 posXYZ = t_Lightmap_PosXYZ.Sample(s_Linear, input.texcoord).rgb;
    float3 negXYZ = t_Lightmap_NegXYZ.Sample(s_Linear, input.texcoord).rgb;

    lightmaps[0] = posXYZ.r;  // +X
    lightmaps[1] = negXYZ.r;  // -X
    lightmaps[2] = posXYZ.g;  // +Y
    lightmaps[3] = negXYZ.g;  // -Y
    lightmaps[4] = posXYZ.b;  // +Z
    lightmaps[5] = negXYZ.b;  // -Z

    // Evaluate each clustered light
    float3 totalLighting = float3(0, 0, 0);

    uint3 cluster = WorldToCluster(input.worldPos);
    uint clusterIndex = GetClusterIndex(cluster);

    uint lightStart = g_LightIndices[clusterIndex * 2 + 0];
    uint lightCount = g_LightIndices[clusterIndex * 2 + 1];

    for (uint i = 0; i < min(lightCount, 8); i++) {
        uint lightIndex = g_LightIndices[lightStart + i];
        LightData light = g_Lights[lightIndex];

        // Calculate light direction
        float3 L = normalize(light.position - input.worldPos);

        // Compute weights for 6 directions
        float weights[6];
        weights[0] = max(0, L.x);   // +X
        weights[1] = max(0, -L.x);  // -X
        weights[2] = max(0, L.y);   // +Y
        weights[3] = max(0, -L.y);  // -Y
        weights[4] = max(0, L.z);   // +Z
        weights[5] = max(0, -L.z);  // -Z

        // Blend 6 lightmaps for this light direction
        float3 lightResponse = float3(0, 0, 0);
        for (int j = 0; j < 6; j++) {
            lightResponse += lightmaps[j] * weights[j];
        }

        // Attenuation
        float distance = length(light.position - input.worldPos);
        float attenuation = 1.0 / (distance * distance);
        attenuation *= saturate(1.0 - distance / light.radius);

        totalLighting += light.color * lightResponse * attenuation;
    }

    // Add ambient
    totalLighting += cb_AmbientLight.rgb * 0.2;

    PSOutput output;
    output.color = float4(albedo.rgb * totalLighting, albedo.a);
    return output;
}
```

**Content Pipeline:**
- Artist creates smoke in Houdini/EmberGen
- Offline renderer bakes 6-directional light response
- Export as 2 textures (pos_xyz.dds, neg_xyz.dds)
- Pack into CPEDef material definition

**Expected usage:** 3-5 hero effects (important smoke/fire)
**Memory cost:** 20-50 MB for hero effects (flipbook textures)

---

### Task 7.4: Tier 3 - Simple Lit Particles (Week 20)

**Add normal to particle vertex:**

```cpp
struct ParticleVertex {
    Fvector position;
    Fvector2 texcoord;
    u32 color;
    Fvector normal;  // NEW: For lighting
};

// In FillSprite():
Fvector normal;
switch (particle.alignment) {
    case dfCameraFacing:
        normal = camTransform.k;  // Toward camera
        break;
    case dfVelocityAligned:
        normal.crossproduct(particle.velocity, camTransform.j);
        normal.normalize_safe();
        break;
}

vertices[0].normal = normal;
// ... (same normal for all 4 vertices)
```

**Shader:** Simple N·L diffuse with clustered lights (reuse from `forward_base.ps`)

**Expected usage:** 10-20% of particles (debris, casings, rain)

---

### Task 7.5: Tier 4 - Emissive Particles (Week 20)

**Simplest case - texture sample only:**

```hlsl
// particle_emissive.ps
PSOutput main(PSInput input) {
    float4 albedo = t_BaseColor.Sample(s_Linear, input.texcoord);
    albedo *= input.color;  // Vertex color modulation

    PSOutput output;
    output.color = albedo;  // No lighting
    return output;
}
```

**Expected usage:** 50-70% of particles (muzzle flashes, sparks, UI)

---

### Task 7.6: Integrate with FrameGraph (Week 21)

**File:** `src/Layers/xrRender/FrameGraphPasses/ParticlePassSetup.cpp`

```cpp
framegraph::DefaultOutputLayout setupParticlePass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::DefaultOutputLayout inputs,
    ParticleBatches* particles,
    MaterialCache* materialCache,
    VolumetricRenderer* volumetricRenderer,  // NEW: For Tier 1
    u32 width,
    u32 height)
{
    // Tier 1 particles already handled in Phase 6 volumetrics

    // Tier 2-4: Render as billboards
    auto& passData = fg.addCallbackPass<ParticlePassData>(
        "Particles",
        [&](/* ... */) {
            // Setup: Read color/depth from forward pass
            data.color = passBuilder.write(inputs.albedo, ResourceState::RenderTarget);
            data.depth = passBuilder.read(inputs.depth, ResourceState::DepthStencilRead);
        },
        [](const ParticlePassData& data, /* ... */) {
            // Execute: Render particles by tier
            for (auto& batch : particles->GetBatches()) {
                switch (batch.lightingMode) {
                    case ParticleLightingMode::Emissive:
                        // Use emissive PSO
                        break;
                    case ParticleLightingMode::Lit:
                        // Use lit PSO (clustered)
                        break;
                    case ParticleLightingMode::SixWay:
                        // Use 6-way PSO (hero smoke)
                        break;
                    case ParticleLightingMode::Volumetric:
                        // Skip (already rendered in volumetrics)
                        break;
                }
                // Draw batch
            }
        }
    );

    return outputs;
}
```

---

### Success Metrics (Phase 7)

**Validation:**
1. All particle types render correctly:
   - Emissive particles work (muzzle flashes, sparks)
   - Lit particles respond to scene lights (debris)
   - Hero smoke uses 6-way lightmaps (quality check)
   - Volumetric particles visible in froxel volume

2. Performance:
   - Emissive: ~0.1ms (1000 particles)
   - Lit: ~0.3ms (500 particles)
   - 6-way: ~0.15ms (3-5 hero effects)
   - Volumetric: already counted in Phase 6
   - **Total: ~0.5ms for particles**

3. Memory:
   - Vertex data: ~50 KB (1500 particles × 40 bytes/vertex × 4 vertices)
   - Hero smoke lightmaps: ~20-50 MB (3-5 effects)
   - **Total: ~50-100 MB**

**Quality:**
- Hero smoke looks amazing (multi-bounce scattering baked)
- Lit particles respond to scene lights correctly
- Emissive particles bright and punchy
- Volumetric smoke integrates with fog naturally

---

## Phase 8: Optimization & Polish (Weeks 22-24)

**Goal:** Optimize performance, fix visual issues, reach production quality

This final phase takes the renderer from "working" to "shippable".

### Task 8.1-8.5: Performance & Quality Polish

See `PARTICLE_LIGHTING_COMPARISON.md` and `VOLUMETRIC_ARCHITECTURE.md` for detailed optimization strategies:

**Key Optimizations:**
1. **Async Compute:** Overlap shadow rendering with depth prepass (2-3ms gain)
2. **Shadow Quality:** Fix acne, peter-panning, cascade popping, shimmering
3. **Transparency Pass:** Separate pass for correct alpha blending
4. **SSAO:** Optional ambient occlusion (depth-only, ~1ms)
5. **Performance Profiling:** RenderDoc/PIX analysis, bottleneck identification

**Performance Targets:**
- Simple scene: 140-165fps
- Complex scene (100 lights, volumetrics, particles): 100-120fps
- Stress test: 60-80fps

---

| Phase | Tasks | Weeks | Deliverable |
|-------|-------|-------|-------------|
| **Phase 1** | Foundation cleanup | 1-2 | Single-RT forward rendering (unlit) |
| **Phase 2** | Depth prepass | 3-4 | Early-Z optimization (+20-25% FPS) |
| **Phase 2.5** | PBR conversion | 5 | Convert all materials to PBR workflow |
| **Phase 3** | PBR lighting | 6-7 | Physically-based lit scene (sun only, no shadows) |
| **Phase 3.5** | GPU culling | 8 | GPU-driven rendering, DetailManager, grass (+10× geometry) |
| **Phase 4** | Shadow maps | 9-10 | Dynamic shadows (CSM) |
| **Phase 5** | Clustered lights | 11-13 | Multi-light support (100+ lights) |
| **Phase 6** | Volumetrics | 14-16 | Native froxel volumetric system (fog, light shafts) |
| **Phase 7** | Particles | 17-21 | Four-tier particle system (volumetric, 6-way, lit, emissive) |
| **Phase 8** | Optimization | 22-24 | Production quality, target FPS reached |

**Total: 24 weeks** (6 months)

**Key Architectural Decisions:**
- **Native Volumetric System:** Phase 6 implements `IVolumetricSource` interface - particles, fog, light shafts all equal
- **Four-Tier Particles:** Phase 7 uses volumetrics (Tier 1), Unity 6-way (Tier 2), clustered lighting (Tier 3), emissive (Tier 4)
- **Lighting Inheritance:** Add point light support to volumetrics ONCE → all sources get it automatically

---

## Shader File Checklist

### Must Implement (Core):
- ✅ Phase 1: `forward/forward_base.vs/ps` (unlit)
- ✅ Phase 1: `shared/forward_constants.h`
- ✅ Phase 2: `depth/depth_prepass.vs/ps`
- ✅ Phase 2.5: PBR texture converter tool (C++)
- ✅ Phase 3: `shared/lighting_common.h` (PBR - Cook-Torrance)
- ✅ Phase 3: Update `forward_base.ps` (PBR lit, no shadows)
- ✅ Phase 3.5: `compute/detail_cull.cs` (GPU culling - EXISTING, needs update)
- ✅ Phase 3.5: `compute/hiz_build.cs` (Hi-Z pyramid)
- ✅ Phase 4: `shared/shadow_common.h`
- ✅ Phase 4: `shadow/shadow_cascade.vs/ps`
- ✅ Phase 4: Update `forward_base.ps` (PBR + shadows)
- ✅ Phase 5: `compute/light_culling.cs`
- ✅ Phase 5: Update `forward_base.ps` (clustered lights)
- ✅ Phase 6: `volumetric/world_fog.cs` (uniform fog)
- ✅ Phase 6: `volumetric/particle_emitter.cs` (smoke emitters)
- ✅ Phase 6: `volumetric/apply_lighting.cs` (volumetric lighting)
- ✅ Phase 6: `volumetric/temporal_reproject.cs` (temporal stability)
- ✅ Phase 6: `volumetric/ray_march.ps` (volumetric integration)
- ✅ Phase 7: `particles/particle_emissive.ps` (emissive particles)
- ✅ Phase 7: `particles/particle_lit.ps` (clustered lit particles)
- ✅ Phase 7: `particles/particle_sixway.ps` (Unity 6-way hero smoke)

### Optional (Quality):
- ⬜ Phase 8: `forward/forward_transparent.ps` (transparency)
- ⬜ Phase 8: `postprocess/ssao.cs` (ambient occlusion)
- ⬜ Phase 8: `postprocess/bloom.cs` (HDR bloom)
- ⬜ Phase 8: `postprocess/tonemap.ps` (HDR tonemapping)

### Material Variants (As Needed):
- ⬜ `forward/forward_skin.vs` (skinned meshes)
- ⬜ `forward/forward_terrain.ps` (terrain multi-texturing)
- ⬜ `forward/forward_tree.vs/ps` (SpeedTree optimizations)
- ⬜ `forward/forward_grass.vs/ps` (grass wind animation)

**Core shader count: ~20 files**
**Total with variants: ~30 files**

---

## Risk Mitigation

### High Risk Items:

1. **Shadow shimmer/acne**
   - **Mitigation:** Reference NVIDIA/AMD sample code
   - **Fallback:** Use simpler shadow filtering (2×2 instead of 5×5)

2. **Clustered culling bugs**
   - **Mitigation:** Validate with PIX GPU debugger
   - **Fallback:** Simpler tiled forward (2D grid instead of 3D)

3. **Performance below target**
   - **Mitigation:** Aggressive LOD, reduce shadow resolution
   - **Fallback:** Reduce cluster grid size, fewer cascades

4. **Shader compilation issues**
   - **Mitigation:** Test shaders incrementally
   - **Fallback:** Keep legacy shaders working during transition

---

## Success Criteria

### Functional Requirements:
- ✅ Scene renders correctly (all geometry visible)
- ✅ Sun lighting works (matches time-of-day)
- ✅ Shadows cast by sun (4 cascades)
- ✅ Dynamic lights illuminate geometry (100+ lights)
- ✅ Transparency blends correctly
- ✅ HUD renders on top
- ✅ Particles render
- ✅ Day/night cycle functional

### Performance Requirements:
- ✅ Average scene: >120fps @ 1920x1080
- ✅ Complex scene: >80fps @ 1920x1080
- ✅ Better than current renderer (currently ~90-110fps)

### Quality Requirements:
- ✅ No visual regressions vs original renderer
- ✅ Shadows look correct (no severe artifacts)
- ✅ Lighting matches reference screenshots
- ✅ No flickering or shimmering
- ✅ Smooth frame times (no hitching)

---

## Conclusion

This plan provides a **clear, achievable path** from your current state to a production-ready forward+ renderer with AAA-quality volumetrics and particles in 24 weeks (6 months).

**Key Advantages:**
1. Builds incrementally (each phase adds value)
2. Performance improvements every phase
3. Realistic timelines based on actual workload
4. Clear success metrics per phase
5. Fallback options for high-risk items
6. **Modern architecture:** Native volumetric system (not injection afterthought)
7. **Extensible design:** Easy to add new volumetric effects or particle types
8. **Industry-aligned:** Matches Doom Eternal, UE5 volumetric/particle approaches

**Architectural Highlights:**

**Phase 6 (Volumetrics):**
- `IVolumetricSource` interface - fog, particles, light shafts treated equally
- Multi-pass system: injection → lighting → temporal reprojection → ray marching
- Lighting inheritance: Add point lights ONCE, all volumetric sources get it
- Cost: ~2.5ms, Memory: ~40 MB

**Phase 7 (Particles):**
- **Tier 1:** Volumetric (smoke/fog) → Froxel system (free lighting)
- **Tier 2:** Hero smoke → Unity 6-way lightmaps (multi-bounce scattering)
- **Tier 3:** Lit particles → Clustered forward lighting (N·L diffuse)
- **Tier 4:** Emissive → No lighting (50-70% of particles)
- Cost: ~0.5ms, Memory: ~50-100 MB

**Total Performance Target:**
- Simple scene: 140-165fps @ 1920×1080
- Complex scene (100 lights, volumetrics, particles): 100-120fps
- Massive stress test: 60-80fps

**Recommendation:** Start with Phase 1 this week. The foundation cleanup alone will save you 0.5-1.0ms per frame (60% bandwidth reduction from removing wasteful G-buffer writes).

**Reference Documents:**
- `VOLUMETRIC_ARCHITECTURE.md` - Full native volumetric system design
- `PARTICLE_LIGHTING_COMPARISON.md` - Four-tier particle system details

Ready to begin?
