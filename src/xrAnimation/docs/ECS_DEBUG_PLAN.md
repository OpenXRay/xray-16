# ECS Debug Rendering Refactoring Plan

**Document Type:** Architecture & Implementation Plan
**Last Updated:** 2025-10-13
**Status:** Planning Phase

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Current State Analysis](#current-state-analysis)
3. [Proposed Architecture](#proposed-architecture)
4. [Implementation Phases](#implementation-phases)
5. [Engine Integration Strategy](#engine-integration-strategy)
6. [Testing Strategy](#testing-strategy)
7. [Migration Path](#migration-path)

---

## Executive Summary

### Problem Statement

The current skeleton debug rendering and IK gizmo rendering implementation has architectural issues:

1. **Tight Coupling**: `SkeletonDebugSystem::RenderSkeletons()` casts `void*` to `VulkanRenderer*`, creating a direct dependency from the ECS library to the application-specific renderer
2. **Mixed Responsibilities**: Rendering logic is split between `VulkanRenderer`, `AnimationECS_IK.cpp`, and `ozz_animation_viewer.cpp`
3. **Code Duplication**: Two nearly identical skeleton rendering functions in `VulkanRenderer`
4. **Not Composable**: Difficult to add new debug visualizations or use different rendering backends

### Proposed Solution

Introduce a **3-layer architecture** with clean separation of concerns:

1. **Abstract Interface Layer**: `IDebugDrawContext` - platform-agnostic draw commands
2. **ECS Systems Layer**: Rendering systems that operate on components and issue draw calls
3. **Application Layer**: Platform-specific implementations (Vulkan, OpenGL, DX11)

### Benefits

- ✅ **Backend Agnostic**: Works with Vulkan (viewer), OpenGL/DX11 (engine), or any renderer
- ✅ **Reusable**: Same ECS systems work in ozz_animation_viewer and xrGame
- ✅ **Testable**: Can use mock implementations for unit tests
- ✅ **Composable**: Easy to add new debug visualizations
- ✅ **Maintainable**: Clear separation of data (components), logic (systems), rendering (application)

---

## Current State Analysis

### Existing Code Locations

#### Components (Good - Keep These)
- **File**: `src/xrAnimation/AnimationECS_Components.h`
- **Components**:
  - `SkeletonDebugState` - Visualization settings (show lines, colors, etc.)
  - `AnimationBuffers` - Pose matrices (locals, models)
  - `InstanceTransform` - World transform for multi-instance rendering

#### Components (Good - Keep These)
- **File**: `src/xrAnimation/AnimationECS_IK.h`
- **Components**:
  - `IKGizmoState` - Per-entity gizmo state (position, hover, drag, etc.)
  - `IKConfiguration` - IK chain configuration (left/right leg/arm)
  - `LimbIKChain` - Individual IK chain data

#### Systems (Problematic - Need Refactoring)
- **File**: `src/xrAnimation/AnimationECS_IK.cpp:832-883`
- **Function**: `SkeletonDebugSystem::RenderSkeletons(entt::registry& registry, void* debug_renderer_ptr)`
- **Problem**:
  ```cpp
  // Line 838: Direct cast to VulkanRenderer!
  auto* renderer = static_cast<xray::animation::renderer::VulkanRenderer*>(debug_renderer_ptr);
  ```
  This creates a dependency from xrAnimation → xrAnimation/tools/renderer

#### Rendering Logic (Problematic - Should Be in ECS System)
- **File**: `src/xrAnimation/tools/renderer/VulkanRenderer.cpp:1198-1257`
- **Function**: `VulkanRenderer::PopulateSkeletonDebugShapesFromPose()`
- **Problem**: Rendering logic for skeletons is in application layer, not ECS layer
- **Contains**:
  - Bone hierarchy traversal
  - Parent-child line drawing
  - Joint sphere drawing
  - Bone shape calculation
  - Root axis drawing

#### Rendering Logic (Problematic - Should Be in ECS System)
- **File**: `src/xrAnimation/tools/ozz_animation_viewer.cpp:1642-1700`
- **Function**: `RenderECSIKGizmos(ViewerState& state, VulkanRenderer& renderer)`
- **Problem**: IK gizmo rendering is in application code, not an ECS system
- **Contains**:
  - Entity iteration with IKGizmoState
  - Gizmo color selection based on state
  - Instance transform application
  - Sphere drawing for each gizmo

#### Code Duplication (Problematic)
- **File**: `src/xrAnimation/tools/renderer/VulkanRenderer.cpp:947-1077`
- **Function**: `VulkanRenderer::PopulateSkeletonDebugShapes()`
- **Problem**: Nearly identical to `PopulateSkeletonDebugShapesFromPose()`, just uses internal state

### Current Call Graph

```
ozz_animation_viewer.cpp main loop
  │
  ├─→ AnimationECS::SkeletonDebugSystem::RenderSkeletons(registry, &renderer)
  │     └─→ casts void* → VulkanRenderer*  ❌ BAD
  │     └─→ calls renderer->PopulateSkeletonDebugShapesFromPose()
  │           └─→ debug_renderer_.DrawLine(), DrawSphere(), etc.
  │
  └─→ RenderECSIKGizmos(state, renderer)
        └─→ Manual entity iteration with registry.view<>()
        └─→ debug_renderer.DrawSphere() for each gizmo
```

### What's Good (Keep)

1. **Component Design**: Data-oriented, no logic in components
2. **DebugRenderer API**: Clean immediate-mode API (`DrawLine`, `DrawSphere`, `DrawBoneShape`, `DrawAxes`)
3. **ECS Registry**: Proper use of EnTT for entity management
4. **Instance Transform**: Already set up for multi-instance rendering

### What's Bad (Fix)

1. **Coupling**: ECS library depends on VulkanRenderer (viewer-specific)
2. **Responsibility**: Rendering logic split across 3+ files
3. **Duplication**: Two skeleton rendering functions with same logic
4. **Composability**: Can't easily add new debug visualizations

---

## Proposed Architecture

### Layered Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                  APPLICATION LAYER                              │
│                                                                 │
│  ozz_animation_viewer:                                          │
│    VulkanRenderer : IDebugDrawContext                           │
│      DrawLine() → debug_renderer_.DrawLine()                    │
│                                                                 │
│  xrGame (Engine):                                               │
│    EngineDebugDrawAdapter : IDebugDrawContext                   │
│      DrawLine() → DBG_DrawLine()                                │
│                                                                 │
│  Unit Tests:                                                    │
│    MockDebugDrawContext : IDebugDrawContext                     │
│      DrawLine() → recorded_calls.push_back(...)                 │
└─────────────────┬───────────────────────────────────────────────┘
                  │
                  │ IDebugDrawContext interface
                  │ (abstract draw commands)
                  │
┌─────────────────▼───────────────────────────────────────────────┐
│                    ECS SYSTEMS LAYER                            │
│                                                                 │
│  SkeletonDebugRenderSystem::Render(registry, ctx, metadata)    │
│    - Iterates entities: view<SkeletonDebugState,               │
│                              AnimationBuffers,                  │
│                              InstanceTransform,                 │
│                              SkeletonMetadata>                  │
│    - Extracts bone hierarchy, pose, instance transform         │
│    - Calls ctx.DrawLine(), ctx.DrawSphere(), ctx.DrawBoneShape()│
│                                                                 │
│  IKGizmoRenderSystem::Render(registry, ctx)                    │
│    - Iterates entities: view<IKGizmoState,                     │
│                              IKConfiguration,                   │
│                              InstanceTransform>                 │
│    - Extracts gizmo positions, states                          │
│    - Calls ctx.DrawSphere()                                    │
│                                                                 │
└─────────────────┬───────────────────────────────────────────────┘
                  │
                  │ Reads/writes component data
                  │
┌─────────────────▼───────────────────────────────────────────────┐
│                    COMPONENT LAYER                              │
│                                                                 │
│  SkeletonDebugState - Visualization settings                   │
│  SkeletonMetadata - Skeleton hierarchy, parent indices          │
│  AnimationBuffers - Pose matrices (locals, models)             │
│  InstanceTransform - World transform                           │
│  IKGizmoState - Gizmo visual state                             │
│  IKConfiguration - IK chains                                   │
└─────────────────────────────────────────────────────────────────┘
```

### Interface Definition

**File**: `src/xrAnimation/IDebugDrawContext.h` (NEW)

```cpp
#pragma once

#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/simd_math.h"

namespace AnimationECS {

/// <summary>
/// Abstract interface for debug primitive drawing.
/// Decouples ECS rendering systems from specific rendering implementations.
/// </summary>
/// <remarks>
/// Implementations:
/// - VulkanRenderer (ozz_animation_viewer) - forwards to DebugRenderer
/// - EngineDebugDrawAdapter (xrGame) - forwards to DBG_Draw* functions
/// - MockDebugDrawContext (unit tests) - records calls for verification
/// </remarks>
class IDebugDrawContext {
public:
    virtual ~IDebugDrawContext() = default;

    /// <summary>
    /// Draw a line between two points
    /// </summary>
    virtual void DrawLine(const ozz::math::Float3& start,
                         const ozz::math::Float3& end,
                         const ozz::math::Float4& color) = 0;

    /// <summary>
    /// Draw a sphere at a position
    /// </summary>
    /// <param name="segments">Number of subdivisions (higher = smoother)</param>
    virtual void DrawSphere(const ozz::math::Float3& center,
                           float radius,
                           const ozz::math::Float4& color,
                           int segments = 12) = 0;

    /// <summary>
    /// Draw a bone shape (octahedron/capsule) between two points
    /// </summary>
    /// <remarks>
    /// Used for skeleton bone visualization.
    /// Typically implemented as octahedron or capsule shape.
    /// </remarks>
    virtual void DrawBoneShape(const ozz::math::Float3& head,
                              const ozz::math::Float3& tail,
                              float radius,
                              const ozz::math::Float4& color) = 0;

    /// <summary>
    /// Draw coordinate axes at a transform
    /// </summary>
    /// <param name="scale">Length of each axis line</param>
    virtual void DrawAxes(const ozz::math::Float4x4& transform,
                         float scale,
                         const ozz::math::Float4& color_x,
                         const ozz::math::Float4& color_y,
                         const ozz::math::Float4& color_z) = 0;
};

} // namespace AnimationECS
```

### Component Definition

**File**: `src/xrAnimation/AnimationECS_Components.h` (ADD)

```cpp
//-----------------------------------------------------------------------------
// SkeletonMetadata Component
// Per-entity skeleton hierarchy and metadata
//-----------------------------------------------------------------------------
struct SkeletonMetadata
{
    // Skeleton hierarchy
    std::vector<int> joint_parents;  // Parent indices (-1 for roots)

    // Extended metadata (physics shapes, IK constraints, etc.)
    XRay::Animation::ExtendedBoneMetadataCollection metadata;

    // Skeleton reference (optional - for rest pose access)
    const ozz::animation::Skeleton* skeleton{nullptr};

    bool IsValid() const {
        return !joint_parents.empty();
    }
};
```

**Purpose**:
- Store skeleton hierarchy per-entity (not globally)
- Eliminates need to pass metadata separately to rendering systems
- More composable - each entity can have different skeleton

---

## Implementation Phases

### Phase 1: Create Abstract Interface (30 minutes)

**Goal**: Define the contract between ECS systems and rendering implementations

**Tasks**:
1. Create `src/xrAnimation/IDebugDrawContext.h`
2. Define pure virtual methods:
   - `DrawLine()`
   - `DrawSphere()`
   - `DrawBoneShape()`
   - `DrawAxes()`
3. Add comprehensive documentation
4. No dependencies on VulkanRenderer or any specific renderer

**Validation**:
- File compiles independently
- No circular dependencies
- Interface is minimal and sufficient

---

### Phase 2: Create ECS Rendering Systems (2-3 hours)

**Goal**: Move all rendering logic into proper ECS systems

#### File: `src/xrAnimation/AnimationECS_DebugRender.h` (NEW)

```cpp
#pragma once

#include "xrCore/xrCore.h"
#include "ozz/base/maths/simd_math.h"
#include "entt/entt.hpp"
#include "IDebugDrawContext.h"
#include "ExtendedBoneMetadata.h"

namespace AnimationECS {

//-----------------------------------------------------------------------------
// SkeletonDebugRenderSystem
// Renders skeleton bones as debug primitives
//-----------------------------------------------------------------------------
class SkeletonDebugRenderSystem
{
public:
    /// <summary>
    /// Render skeleton debug visualization for all entities
    /// </summary>
    /// <param name="registry">ECS registry</param>
    /// <param name="ctx">Debug draw context (application-provided)</param>
    /// <remarks>
    /// Iterates entities with:
    ///   - SkeletonDebugState (settings)
    ///   - AnimationBuffers (pose data)
    ///   - InstanceTransform (world transform)
    ///   - SkeletonMetadata (hierarchy)
    /// </remarks>
    static void Render(entt::registry& registry, IDebugDrawContext& ctx);

private:
    /// <summary>
    /// Render a single skeleton instance
    /// </summary>
    static void RenderSkeleton(
        IDebugDrawContext& ctx,
        const std::vector<ozz::math::Float4x4>& pose_models,
        const std::vector<int>& skeleton_parents,
        const ozz::math::Float4x4& instance_transform,
        const SkeletonDebugState& debug_state,
        const XRay::Animation::ExtendedBoneMetadataCollection& bone_metadata);

    /// <summary>
    /// Compute rest length for a bone (distance to parent or child)
    /// </summary>
    static float ComputeRestLength(
        int bone_index,
        const std::vector<ozz::math::Float4x4>& rest_models,
        const std::vector<int>& skeleton_parents);
};

//-----------------------------------------------------------------------------
// IKGizmoRenderSystem
// Renders IK target gizmos as debug primitives
//-----------------------------------------------------------------------------
class IKGizmoRenderSystem
{
public:
    /// <summary>
    /// Render IK gizmos for all entities
    /// </summary>
    /// <param name="registry">ECS registry</param>
    /// <param name="ctx">Debug draw context (application-provided)</param>
    /// <remarks>
    /// Iterates entities with:
    ///   - IKGizmoState (gizmo positions, hover/drag states)
    ///   - IKConfiguration (IK chain data)
    ///   - InstanceTransform (world transform)
    /// </remarks>
    static void Render(entt::registry& registry, IDebugDrawContext& ctx);

private:
    /// <summary>
    /// Render a single gizmo for an IK chain
    /// </summary>
    static void RenderGizmo(
        IDebugDrawContext& ctx,
        const IKGizmoState::ChainGizmo& gizmo,
        const LimbIKChain& chain,
        const ozz::math::Float4x4& instance_transform,
        bool is_leg);
};

} // namespace AnimationECS
```

#### File: `src/xrAnimation/AnimationECS_DebugRender.cpp` (NEW)

**Implementation Steps**:

1. **Move `PopulateSkeletonDebugShapesFromPose` logic** from `VulkanRenderer.cpp:1198-1257`:
   ```cpp
   void SkeletonDebugRenderSystem::Render(entt::registry& registry, IDebugDrawContext& ctx)
   {
       // Iterate entities with all required components
       auto view = registry.view<SkeletonDebugState, AnimationBuffers,
                                  InstanceTransform, SkeletonMetadata>();

       for (auto entity : view) {
           const auto& debug_state = view.get<SkeletonDebugState>(entity);
           const auto& buffers = view.get<AnimationBuffers>(entity);
           const auto& inst_transform = view.get<InstanceTransform>(entity);
           const auto& metadata = view.get<SkeletonMetadata>(entity);

           // Skip if disabled or empty
           if (!debug_state.enabled || !debug_state.show_skeleton_lines)
               continue;
           if (!buffers.IsInitialized() || buffers.models.empty())
               continue;
           if (!metadata.IsValid())
               continue;

           // Convert to std::vector for RenderSkeleton
           std::vector<ozz::math::Float4x4> pose_models(
               buffers.models.begin(), buffers.models.end());

           // Render this skeleton instance
           RenderSkeleton(ctx, pose_models, metadata.joint_parents,
                         inst_transform.world_transform, debug_state,
                         metadata.metadata);
       }
   }
   ```

2. **Move `RenderECSIKGizmos` logic** from `ozz_animation_viewer.cpp:1642-1700`:
   ```cpp
   void IKGizmoRenderSystem::Render(entt::registry& registry, IDebugDrawContext& ctx)
   {
       // Iterate entities with IK gizmo components
       auto view = registry.view<IKGizmoState, IKConfiguration, InstanceTransform>();

       for (auto entity : view) {
           const auto& gizmo_state = view.get<IKGizmoState>(entity);
           const auto& ik_config = view.get<IKConfiguration>(entity);
           const auto& inst_transform = view.get<InstanceTransform>(entity);

           if (!gizmo_state.enabled || !ik_config.IsInitialized())
               continue;

           // Render each gizmo
           RenderGizmo(ctx, gizmo_state.left_leg_gizmo, ik_config.left_leg,
                      inst_transform.world_transform, true);
           RenderGizmo(ctx, gizmo_state.right_leg_gizmo, ik_config.right_leg,
                      inst_transform.world_transform, true);
           RenderGizmo(ctx, gizmo_state.left_arm_gizmo, ik_config.left_arm,
                      inst_transform.world_transform, false);
           RenderGizmo(ctx, gizmo_state.right_arm_gizmo, ik_config.right_arm,
                      inst_transform.world_transform, false);
       }
   }
   ```

**Validation**:
- Systems compile independently
- No dependencies on VulkanRenderer
- Logic is identical to current implementation

---

### Phase 3: Add SkeletonMetadata Component (30 minutes)

**Goal**: Store skeleton hierarchy per-entity instead of globally

**Tasks**:
1. Add `SkeletonMetadata` struct to `AnimationECS_Components.h`
2. Update `InitializeECSInstances()` in `ozz_animation_viewer.cpp` to add component:
   ```cpp
   // For each entity
   auto& skel_metadata = registry.emplace<AnimationECS::SkeletonMetadata>(entity);
   skel_metadata.joint_parents.assign(parents.begin(), parents.end());
   skel_metadata.metadata = state.bone_metadata;
   skel_metadata.skeleton = state.skeleton.get();
   ```
3. Update systems to read from `SkeletonMetadata` component

**Validation**:
- Component stores hierarchy correctly
- Systems can access per-entity metadata
- No global state needed

---

### Phase 4: Implement IDebugDrawContext in VulkanRenderer (30 minutes)

**Goal**: Make VulkanRenderer satisfy the interface

**File**: `src/xrAnimation/tools/renderer/VulkanRenderer.h` (MODIFY)

```cpp
#include "xrAnimation/IDebugDrawContext.h"

class VulkanRenderer : public AnimationECS::IDebugDrawContext {
public:
    // ... existing methods ...

    // IDebugDrawContext implementation (forward to DebugRenderer)
    void DrawLine(const ozz::math::Float3& start,
                  const ozz::math::Float3& end,
                  const ozz::math::Float4& color) override;

    void DrawSphere(const ozz::math::Float3& center,
                    float radius,
                    const ozz::math::Float4& color,
                    int segments = 12) override;

    void DrawBoneShape(const ozz::math::Float3& head,
                       const ozz::math::Float3& tail,
                       float radius,
                       const ozz::math::Float4& color) override;

    void DrawAxes(const ozz::math::Float4x4& transform,
                  float scale,
                  const ozz::math::Float4& color_x,
                  const ozz::math::Float4& color_y,
                  const ozz::math::Float4& color_z) override;
};
```

**File**: `src/xrAnimation/tools/renderer/VulkanRenderer.cpp` (MODIFY)

```cpp
void VulkanRenderer::DrawLine(const ozz::math::Float3& start,
                              const ozz::math::Float3& end,
                              const ozz::math::Float4& color) {
    debug_renderer_.DrawLine(start, end, color);
}

void VulkanRenderer::DrawSphere(const ozz::math::Float3& center,
                                float radius,
                                const ozz::math::Float4& color,
                                int segments) {
    debug_renderer_.DrawSphere(center, radius, color, segments);
}

void VulkanRenderer::DrawBoneShape(const ozz::math::Float3& head,
                                   const ozz::math::Float3& tail,
                                   float radius,
                                   const ozz::math::Float4& color) {
    debug_renderer_.DrawBoneShape(head, tail, radius, color);
}

void VulkanRenderer::DrawAxes(const ozz::math::Float4x4& transform,
                              float scale,
                              const ozz::math::Float4& color_x,
                              const ozz::math::Float4& color_y,
                              const ozz::math::Float4& color_z) {
    debug_renderer_.DrawAxes(transform, scale, color_x, color_y, color_z);
}
```

**Purpose**: Simple adapter/forwarding pattern. No logic, just delegation.

**Validation**:
- VulkanRenderer compiles with interface inheritance
- All methods forward correctly to DebugRenderer

---

### Phase 5: Update Call Sites (30 minutes)

**Goal**: Use new ECS systems at application level

**File**: `src/xrAnimation/tools/ozz_animation_viewer.cpp` (MODIFY)

**Before** (line 2738):
```cpp
// Old: Direct call with void* cast
AnimationECS::SkeletonDebugSystem::RenderSkeletons(registry, &renderer);
```

**After**:
```cpp
// New: Clean interface, no casting
AnimationECS::SkeletonDebugRenderSystem::Render(registry, renderer);
```

**Before** (line 2782):
```cpp
// Old: Manual function in viewer
RenderECSIKGizmos(state, renderer);
```

**After**:
```cpp
// New: ECS system
AnimationECS::IKGizmoRenderSystem::Render(registry, renderer);
```

**Delete** (lines 1642-1700):
```cpp
// Delete entire RenderECSIKGizmos() function - now in ECS system
```

**Validation**:
- ozz_animation_viewer compiles
- Call sites are cleaner
- No manual rendering logic in application

---

### Phase 6: Remove Old Code (30 minutes)

**Goal**: Clean up deprecated implementations

**Files to Modify**:

1. **src/xrAnimation/AnimationECS_IK.h**
   - Remove `SkeletonDebugSystem` class declaration (lines 327-351)
   - Or update to just call new system

2. **src/xrAnimation/AnimationECS_IK.cpp**
   - Delete `SkeletonDebugSystem::RenderSkeletons()` (lines 832-895)

3. **src/xrAnimation/tools/renderer/VulkanRenderer.h**
   - Remove `PopulateSkeletonDebugShapes()` declaration
   - Remove `PopulateSkeletonDebugShapesFromPose()` declaration

4. **src/xrAnimation/tools/renderer/VulkanRenderer.cpp**
   - Delete `PopulateSkeletonDebugShapes()` (lines 947-1077)
   - Delete `PopulateSkeletonDebugShapesFromPose()` (lines 1198-1257)

5. **src/xrAnimation/tools/ozz_animation_viewer.cpp**
   - Delete `RenderECSIKGizmos()` function (lines 1642-1700)
   - Delete `RenderIKGizmos()` function if still present (lines 1435-1467)
   - Delete `HandleIKGizmoMouseInteraction()` if obsolete

**Validation**:
- All files compile after deletions
- No "unused function" warnings
- Code is cleaner and more maintainable

---

## Engine Integration Strategy

### Overview

The architecture is designed to work with **any** rendering backend:
- **ozz_animation_viewer**: Vulkan via VulkanRenderer
- **xrGame Engine**: OpenGL/DX11 via DBG_DRAW functions
- **Unit Tests**: Mock implementation for verification

### Engine Adapter Pattern

**File**: `src/xrGame/EngineDebugDrawAdapter.h` (FUTURE)

```cpp
#include "xrAnimation/IDebugDrawContext.h"

/// <summary>
/// Adapter that bridges ECS rendering systems to X-Ray Engine's DBG_DRAW functions
/// </summary>
class EngineDebugDrawAdapter : public AnimationECS::IDebugDrawContext
{
public:
    void DrawLine(const ozz::math::Float3& start,
                  const ozz::math::Float3& end,
                  const ozz::math::Float4& color) override;

    void DrawSphere(const ozz::math::Float3& center,
                    float radius,
                    const ozz::math::Float4& color,
                    int segments) override;

    void DrawBoneShape(const ozz::math::Float3& head,
                       const ozz::math::Float3& tail,
                       float radius,
                       const ozz::math::Float4& color) override;

    void DrawAxes(const ozz::math::Float4x4& transform,
                  float scale,
                  const ozz::math::Float4& color_x,
                  const ozz::math::Float4& color_y,
                  const ozz::math::Float4& color_z) override;
};
```

**File**: `src/xrGame/EngineDebugDrawAdapter.cpp` (FUTURE)

```cpp
#include "stdafx.h"
#include "EngineDebugDrawAdapter.h"
#include "../Include/xrRender/DebugRender.h"  // DBG_Draw* functions

void EngineDebugDrawAdapter::DrawLine(
    const ozz::math::Float3& start,
    const ozz::math::Float3& end,
    const ozz::math::Float4& color)
{
    // Convert ozz types → X-Ray types
    Fvector p1(start.x, start.y, start.z);
    Fvector p2(end.x, end.y, end.z);
    u32 xray_color = color_rgba_f(color.x, color.y, color.z, color.w);

    // Use engine's debug draw function
    DRender->dbg_DrawLine(p1, p2, xray_color);
}

void EngineDebugDrawAdapter::DrawSphere(
    const ozz::math::Float3& center,
    float radius,
    const ozz::math::Float4& color,
    int segments)
{
    Fvector c(center.x, center.y, center.z);
    u32 xray_color = color_rgba_f(color.x, color.y, color.z, color.w);

    // If engine has DBG_DrawSphere, use it
    // Otherwise, implement with lat/long lines
    DRender->dbg_DrawEllipse(Fidentity.transform(c), xray_color);
}

void EngineDebugDrawAdapter::DrawBoneShape(
    const ozz::math::Float3& head,
    const ozz::math::Float3& tail,
    float radius,
    const ozz::math::Float4& color)
{
    // Implement bone shape using primitives
    Fvector h(head.x, head.y, head.z);
    Fvector t(tail.x, tail.y, tail.z);
    u32 xray_color = color_rgba_f(color.x, color.y, color.z, color.w);

    // Draw bone as line + spheres at joints
    DRender->dbg_DrawLine(h, t, xray_color);

    Fmatrix head_xform;
    head_xform.identity();
    head_xform.c = h;
    DRender->dbg_DrawEllipse(head_xform, xray_color);

    Fmatrix tail_xform;
    tail_xform.identity();
    tail_xform.c = t;
    DRender->dbg_DrawEllipse(tail_xform, xray_color);
}

void EngineDebugDrawAdapter::DrawAxes(
    const ozz::math::Float4x4& transform,
    float scale,
    const ozz::math::Float4& color_x,
    const ozz::math::Float4& color_y,
    const ozz::math::Float4& color_z)
{
    // Convert ozz::math::Float4x4 → Fmatrix
    Fmatrix xform;
    xform.i.set(ozz::math::GetX(transform.cols[0]),
                ozz::math::GetY(transform.cols[0]),
                ozz::math::GetZ(transform.cols[0]));
    xform.j.set(ozz::math::GetX(transform.cols[1]),
                ozz::math::GetY(transform.cols[1]),
                ozz::math::GetZ(transform.cols[1]));
    xform.k.set(ozz::math::GetX(transform.cols[2]),
                ozz::math::GetY(transform.cols[2]),
                ozz::math::GetZ(transform.cols[2]));
    xform.c.set(ozz::math::GetX(transform.cols[3]),
                ozz::math::GetY(transform.cols[3]),
                ozz::math::GetZ(transform.cols[3]));

    // Draw axes
    Fvector origin = xform.c;
    Fvector x_axis = origin + xform.i * scale;
    Fvector y_axis = origin + xform.j * scale;
    Fvector z_axis = origin + xform.k * scale;

    DRender->dbg_DrawLine(origin, x_axis, color_rgba_f(color_x.x, color_x.y, color_x.z, color_x.w));
    DRender->dbg_DrawLine(origin, y_axis, color_rgba_f(color_y.x, color_y.y, color_y.z, color_y.w));
    DRender->dbg_DrawLine(origin, z_axis, color_rgba_f(color_z.x, color_z.y, color_z.z, color_z.w));
}
```

### Engine Usage Example

**File**: `src/xrGame/Actor.cpp` (FUTURE)

```cpp
void CActor::UpdateAnimation(float dt)
{
    // Get animation registry (ECS)
    AnimationECS::AnimationRegistry& anim_registry = GetAnimationRegistry();
    auto& registry = anim_registry.GetRegistry();

    // Update animations, IK, blending, etc.
    anim_registry.Update(dt);

    // Render debug visualization (if enabled)
    if (g_pGameLevel && g_pGameLevel->bDebug) {
        // Create adapter to engine's debug draw system
        EngineDebugDrawAdapter debug_ctx;

        // ECS systems render via engine's DBG_DRAW functions!
        AnimationECS::SkeletonDebugRenderSystem::Render(registry, debug_ctx);
        AnimationECS::IKGizmoRenderSystem::Render(registry, debug_ctx);
    }
}
```

### Key Points

1. **Zero Changes to ECS Systems**: The systems don't know or care what renderer they're using
2. **Reuse Existing Code**: Just wrap engine's `DBG_DRAW` functions with 30-line adapter
3. **Same Code, Different Backends**:
   ```cpp
   // ozz_animation_viewer (Vulkan)
   VulkanRenderer renderer;
   SkeletonDebugRenderSystem::Render(registry, renderer);

   // xrGame (OpenGL/DX11)
   EngineDebugDrawAdapter adapter;
   SkeletonDebugRenderSystem::Render(registry, adapter);
   ```

---

## Testing Strategy

### Unit Testing (Optional but Recommended)

**File**: `src/xrAnimation/tests/test_debug_render.cpp` (NEW)

```cpp
#include "AnimationECS_DebugRender.h"
#include <gtest/gtest.h>

// Mock implementation for testing
class MockDebugDrawContext : public AnimationECS::IDebugDrawContext {
public:
    struct Call {
        std::string method;
        std::vector<float> params;
    };

    std::vector<Call> calls;

    void DrawLine(const ozz::math::Float3& start,
                  const ozz::math::Float3& end,
                  const ozz::math::Float4& color) override {
        calls.push_back({"DrawLine", {start.x, start.y, start.z, end.x, end.y, end.z}});
    }

    void DrawSphere(const ozz::math::Float3& center,
                    float radius,
                    const ozz::math::Float4& color,
                    int segments) override {
        calls.push_back({"DrawSphere", {center.x, center.y, center.z, radius}});
    }

    // ... other methods
};

TEST(SkeletonDebugRenderSystem, CallsDrawLineForBones) {
    entt::registry registry;
    auto entity = registry.create();

    // Setup entity with required components
    auto& debug_state = registry.emplace<AnimationECS::SkeletonDebugState>(entity);
    debug_state.enabled = true;
    debug_state.show_skeleton_lines = true;

    auto& buffers = registry.emplace<AnimationECS::AnimationBuffers>(entity);
    // ... initialize buffers with test data

    auto& inst_transform = registry.emplace<AnimationECS::InstanceTransform>(entity);
    inst_transform.world_transform = ozz::math::Float4x4::identity();

    auto& metadata = registry.emplace<AnimationECS::SkeletonMetadata>(entity);
    metadata.joint_parents = {-1, 0};  // Root + 1 child

    // Render with mock
    MockDebugDrawContext mock;
    AnimationECS::SkeletonDebugRenderSystem::Render(registry, mock);

    // Verify calls
    ASSERT_GT(mock.calls.size(), 0);
    EXPECT_EQ(mock.calls[0].method, "DrawLine");  // Parent-child line
}
```

### Visual Testing Checklist

**After each phase, verify**:

1. ✅ **Skeleton Lines Render**: Bones show as lines connecting parent → child
2. ✅ **Joint Spheres Render**: Small spheres at each joint position
3. ✅ **Root Axes Render**: X/Y/Z axes at root bone
4. ✅ **IK Gizmos Render**: Colored spheres at IK targets
5. ✅ **Gizmo Hover State**: Gizmo changes color when mouse hovers
6. ✅ **Gizmo Drag State**: Gizmo turns yellow when dragging
7. ✅ **Multi-Instance**: All instances render correctly in grid
8. ✅ **Toggle Visibility**: Skeleton visibility toggle works
9. ✅ **IK Enable/Disable**: Disabled chains show as gray/faded gizmos

### Performance Testing

**Metrics to monitor**:
- Frame time with 100+ instances
- Number of draw calls issued
- Memory usage (should be unchanged)

**Expected**: No performance regression compared to current implementation.

---

## Migration Path

### Backward Compatibility

During refactoring, we can maintain both old and new systems temporarily:

```cpp
// In ozz_animation_viewer.cpp
if (g_use_new_debug_render) {
    // New ECS systems
    AnimationECS::SkeletonDebugRenderSystem::Render(registry, renderer);
    AnimationECS::IKGizmoRenderSystem::Render(registry, renderer);
} else {
    // Old implementation
    AnimationECS::SkeletonDebugSystem::RenderSkeletons(registry, &renderer);
    RenderECSIKGizmos(state, renderer);
}
```

Once validated, remove old code paths.

### Risk Mitigation

1. **Incremental Implementation**: Each phase can be tested independently
2. **Visual Parity**: Compare screenshots between old/new implementations
3. **Fallback Path**: Keep old code until new is fully validated
4. **Unit Tests**: Catch regressions early

---

## Benefits Summary

### For ozz_animation_viewer

- ✅ Cleaner architecture
- ✅ Easier to add new visualizations
- ✅ Testable with mocks
- ✅ No performance impact

### For xrGame Engine

- ✅ Same ECS systems work out-of-the-box
- ✅ Just provide `EngineDebugDrawAdapter`
- ✅ Uses existing `DBG_DRAW` functions
- ✅ No duplicate rendering logic

### For Development

- ✅ Clear separation of concerns
- ✅ No tight coupling
- ✅ Composable systems
- ✅ Easy to extend

---

## Estimated Timeline

| Phase | Time | Cumulative |
|-------|------|------------|
| 1. Abstract Interface | 30 min | 30 min |
| 2. ECS Systems | 2-3 hours | 3 hours |
| 3. Metadata Component | 30 min | 3.5 hours |
| 4. VulkanRenderer Adapter | 30 min | 4 hours |
| 5. Update Call Sites | 30 min | 4.5 hours |
| 6. Remove Old Code | 30 min | 5 hours |
| 7. Testing & Validation | 1 hour | 6 hours |

**Total**: ~5-6 hours for complete refactoring

---

## Next Steps

1. **Review & Approval**: Get feedback on architecture
2. **Phase 1 Implementation**: Create `IDebugDrawContext.h`
3. **Phase 2 Implementation**: Move rendering logic to ECS systems
4. **Incremental Testing**: Validate each phase before proceeding
5. **Documentation**: Update code comments and architecture docs
6. **Engine Integration Planning**: Design `EngineDebugDrawAdapter` for xrGame

---

**Document Status**: Ready for Implementation
**Author**: Claude + Yohji
**Last Updated**: 2025-10-13
