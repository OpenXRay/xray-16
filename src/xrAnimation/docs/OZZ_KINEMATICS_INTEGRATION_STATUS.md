# Ozz-Animation Integration Status

**Last Updated:** 2025-10-12
**Status:** MVP delivered - Production-ready converters, runtime façade, and visual integration complete

---

## Executive Summary

The X-Ray Engine animation system has been successfully integrated with [ozz-animation](https://github.com/guillaumeblanc/ozz-animation), a modern SIMD-optimized animation library. This integration provides a production-ready animation pipeline with full backward compatibility while enabling future performance optimizations.

### Key Achievements
- ✅ Asset conversion pipeline (OGF/OMF → ozz/ozzx)
- ✅ Runtime façade with full IKinematics/IKinematicsAnimated compatibility
- ✅ Three-tier architecture (Core, Static, Animated)
- ✅ Visual integration with `.ozzx` bundle support
- ✅ Parity testing framework
- ✅ Startup conversion system with digest tracking

---

## Current State

### 1. Asset Conversion Pipeline

**Converter Tool:** `xray_to_ozz_converter`
- Converts `.ogf` skeletons to `.ozz` format
- Converts `.omf` animations to `.ozz` format
- Bundles skeleton + mesh data into `.ozzx` format
- Preserves physics metadata, IK data, and motion parameters
- Supports batch conversion of entire asset directories

**Conversion Quality:**
- ✅ Skeleton hierarchy correctly parsed
- ✅ Bind pose transforms validated against Blender
- ✅ Animation keyframes match legacy runtime
- ✅ Physics shapes and IK constraints preserved
- ✅ Extended bone metadata (mass, inertia, collision layers)

**Conversion Scripts:**
- `run_stalker_hero_conversion.sh` - NPC skeleton/animation
- `run_arms_conversion.sh` - First-person arms
- `run_weapon_conversion.sh` - Weapon models
- `run_monster_conversion.sh` - Monster characters
- `convert_assets.sh` - Batch test fixture generation

### 2. Runtime Integration

**Three-Tier Architecture** (2025-10-07 Refactoring):

```
┌─────────────────────────────────────┐
│      COzzKinematicsVisual           │
│  (Visual wrapper, composition)      │
└──────────────┬──────────────────────┘
               │
               ├─────────────────────┬──────────────────────┐
               ▼                     ▼                      ▼
         ┌──────────┐        ┌──────────┐         ┌──────────────┐
         │   Core   │        │  Static  │         │  Animated    │
         │          │◄───────┤          │◄────────┤              │
         │ Shared   │        │IKinematics│        │IKinematicsA..│
         │ State    │        │          │         │              │
         └──────────┘        └──────────┘         └──────────────┘
```

**OzzKinematicsCore:**
- Shared skeleton and bone state management
- No interface dependencies
- Provides common functionality for both static and animated

**OzzKinematics:**
- Implements `IKinematics` for static models
- Returns `nullptr` from `dcast_PKinematicsAnimated()`
- Eliminates "$editor" console spam for static props
- Memory efficient (no animation buffers)

**OzzKinematicsAnimated:**
- Extends `OzzKinematics` and implements `IKinematicsAnimated`
- Full animation playback support
- Motion library management
- Blend pool and partition system
- Legacy `CBlend` compatibility

**Benefits:**
- Static models don't allocate unnecessary animation data
- Clean separation matches legacy CKinematics/CKinematicsAnimated pattern
- Proper polymorphic behavior (static props return null for animated interface)
- Reduces memory footprint and console spam

### 3. Visual Integration

**COzzKinematicsVisual:**
- Loads `.ozzx` bundles from virtual filesystem
- Conditionally creates static or animated kinematics based on motion references
- Uses composition pattern (owns kinematics instance)
- Hydrates mesh data for CPU skinning
- Integrates with existing renderer pipeline

**Developer Toggle:**
- `g_use_ozz_visuals 1` - Enable `.ozzx` bundle loading
- Falls back to legacy `.ogf` if bundle not found
- Allows side-by-side testing

**Console Commands:**
- `debug_dump_ozz_palette` - Snapshot bone matrices
- `debug_dump_ozz_palette_toggle` - Auto-dump comparison
- `g_dev_ozz_actor` - Hot-swap player model to `.ozzx`
- `g_dev_ozz_animation_list` - List available motions
- `g_dev_ozz_animation <name>` - Play specific animation

### 4. Startup Conversion System

**Automatic Asset Conversion:**
- Scans `$level$` and `$game_meshes$` for `.ogf/.omf` files
- Builds inventory with digest (CRC32 of all asset metadata)
- Persists digest in `user.ltx` for change detection
- Converts missing/stale assets on startup
- Outputs `.ozzx` bundles to `$game_meshes$`
- Outputs `.ozz` animations to `$game_anims$`

**Inventory System:**
- `StartupConversionInventory` - Scans and catalogs legacy assets
- `ComputeLegacyAssetInventoryDigest` - Deterministic hash
- `Load/StoreInventoryDigestInUserConfig` - Persistence in user.ltx
- Detects changes (new files, edited timestamps, modified sizes)

**Conversion Workflow:**
1. Load cached digest from `user.ltx`
2. Scan current asset inventory
3. Compare digests
4. Convert if mismatch or missing outputs
5. Store new digest on success

**Integration Point:**
- `CGamePersistent::OnGameStart()` triggers conversion stage
- Progress could surface through loading screen (pending localization)

### 5. Testing Infrastructure

**Parity Tests:**
- `OzzKinematicsParity.AnimationPoseMatchesLegacySkeleton` - Validates bone transforms
- `OzzKinematicsBootstrap.InitializesFromOzzxBundleSkeleton` - Bundle hydration
- `OzzKinematicsAppliesBoneMetadata` - Extended metadata
- `ModelNaming.NormalizesModelIdentifiers` - Path normalization

**Converter Tests:**
- `ConverterIntegration.*` - End-to-end conversion validation
- Regression tests for skeleton/animation output
- Automated fixture generation

**Test Fixtures:**
- `src/xrAnimation/tests/testdata/` - Generated test assets
- NPC, arms, weapons, monsters
- Requires `convert_assets.sh` to regenerate

**Testing Workflow:**
```bash
# Build tests
cmake --build ozz_utils --target ozz_kinematics_tests xrAnimation_converter_tests -j

# Run all tests
ctest --test-dir ozz_utils --output-on-failure

# Run specific suite
ozz_utils/bin/Debug/ozz_kinematics_tests --gtest_filter=OzzKinematicsParity.*
```

### 6. Visualization Tools

**ozz_animation_viewer:**
- Loads `.ozzx` bundles directly
- Displays skeleton in bind pose
- Plays animations from `.ozz` files
- Profiling overlays (frame time, bone count)
- Debug outputs (bind pose tables, animation JSON dumps)
- Headless mode for automated testing

**Viewer Features:**
- `--bundle=<path>.ozzx` - Load bundle
- `--animation=<path>.ozz` - Load animation
- `--render=false` - Headless mode
- `--max_idle_loops=1` - Exit after N frames
- `--dump-animation-json=<path>` - Export animation data

**Debug Outputs:**
- Bind pose tables (compare with Blender)
- Animation JSON dumps (frame-by-frame transforms)
- Skeleton visualization
- Console logging

---

## Architecture Details

### Coordinate System Conversion

**X-Ray → ozz:**
- X-Ray: Y-up coordinate system
- ozz: Z-up coordinate system (OpenGL convention)
- Conversion matrix: `diag(-1, 1, -1)` (flip X and Z)
- Applied during asset conversion
- Runtime applies inverse when feeding back to X-Ray systems

**Basis Transform:**
```cpp
// X-Ray vector (X, Y, Z) → ozz vector (-X, Y, -Z)
Fvector xray_vec;
ozz_vec = ozz::math::Float3(-xray_vec.x, xray_vec.y, -xray_vec.z);
```

### Update Loop Integration

**CalculateBones Pipeline:**
1. Check throttle (`UCalc_Time`, `UCalc_Interval`)
2. Run ozz `SamplingJob` (if animated)
3. Run `LocalToModelJob` (local → model space)
4. Convert ozz matrices to X-Ray `Fmatrix`
5. Apply coordinate transformation
6. Apply additional bone transforms (ABT)
7. Run bone callbacks
8. Update `mRenderTransform` for skinning
9. Recompute visibility volumes

**Data Flow:**
```
ozz::animation::Animation (shared, read-only)
          ↓
ozz::animation::SamplingJob
          ↓
ozz::vector<SoaTransform> locals (per-character)
          ↓
ozz::animation::LocalToModelJob
          ↓
ozz::vector<Float4x4> models (per-character)
          ↓
Convert to Fmatrix + apply basis transform
          ↓
CBoneInstance.mTransform (X-Ray bone cache)
          ↓
Renderer / Physics / Gameplay systems
```

### Physics Metadata

**Extended Bone Records:**
- Shape data (box, sphere, cylinder)
- Joint IK constraints
- Mass and center of mass
- Inertia tensor (auto-computed)
- Game material index
- Collision layer hints
- Rest length and dominant axis
- Local AABB for volume queries
- Ground contact candidates (feet)
- Weapon anchor candidates (hands)

**Bundle Format:**
- `.ozzx` version 2 with metadata block
- Serialized `BoneRecord` structures
- Runtime hydrates `CBoneData` on load
- Full parity with legacy IK data

---

## Known Gaps & Future Work

### Phase 6: Parallel Animation Processing
**Status:** Planned
**Goal:** Efficiently update 500-1000 animated characters @ 60 FPS

**Strategy:**
- Parallel sampling using `xr_parallel_for`
- Per-character data isolation (thread-safe buffers)
- Recursive parallel-for pattern from ozz samples
- Grain size tuning (64-128 characters per task)

**Expected Results:**
- 10x character count increase
- Sub-4ms animation update budget
- 70%+ thread utilization on 8-core systems

### Phase 7: Animation Polish
**Status:** Planned
**Goal:** Production-quality blending and partition support

**Features:**
- Per-joint weight masks for partitions
- Smooth transition curves (smoothstep)
- Precise motion mark callbacks (within 1 frame)
- Rapid state change buffering

### Phase 8: Shared Motion Container
**Status:** Deferred to ECS migration
**Goal:** Eliminate memory duplication for motion libraries

**Rationale:**
- Current per-instance motion libraries work acceptably
- ECS will change memory layout requirements
- Avoid premature optimization
- Revisit during ECS migration phase

**Design Available:**
- See `src/xrAnimation/docs/OZZ_KINEMATICS_INTEGRATION_DETAILS.md`
- Handle-based API with reference counting
- Skeleton fingerprinting for safety
- LRU eviction for memory management

### GPU Skinning
**Status:** Planned
**Goal:** Move bone transformation to GPU

**Strategy:**
- Upload bone palettes as uniform buffers
- Compute skinned vertices in vertex shader
- Eliminate CPU skinning pass
- Enable 1000+ characters

### Hot Reload Support
**Status:** Not implemented
**Goal:** Reload animations without restarting engine

**Requirements:**
- File watcher for `.ozz` changes
- Invalidate cached animations
- Recompile motion libraries
- Update active blends

### Async Loading
**Status:** Not implemented
**Goal:** Stream large animation sets

**Requirements:**
- Background loading thread
- Progressive hydration
- LOD system for distant characters

---

## Performance Metrics

### Current Baseline
- **Character Count:** 50-100 animated characters functional
- **Frame Budget:** Animation update not yet optimized
- **Memory:** Per-instance motion libraries (~10-50x duplication)
- **Threading:** Single-threaded animation update

### Target (Post-Optimization)
- **Character Count:** 500-1000 @ 60 FPS
- **Frame Budget:** <4ms animation update
- **Memory:** Shared motion libraries (10-50x reduction)
- **Threading:** Parallel update (70%+ CPU utilization)

### Startup Performance
- **Current:** 2-5s for 100 assets (single-threaded conversion)
- **Target:** <1s for 100 assets (8-core parallel conversion)

---

## File Organization

### Core Runtime Files
```
src/xrAnimation/
├── OzzConversion.h/cpp           # Coordinate system conversion
├── OzzBundle.h/cpp               # .ozzx reader/writer
├── OzzKinematicsCore.h/cpp       # Shared skeleton/bone state
├── OzzKinematics.h/cpp           # Static kinematics (IKinematics)
├── OzzKinematicsAnimated.h/cpp   # Animated kinematics (IKinematicsAnimated)
└── OzzKinematics_legacy.h/cpp    # Original monolithic (reference only)
```

### Visual Integration
```
src/Layers/xrRender/
└── OzzKinematicsVisual.h/cpp     # Visual wrapper (.ozzx loader)
```

### Converter Tools
```
tools/xray_to_ozz_converter/
├── SkeletonConverter.h/cpp       # OGF → ozz skeleton
├── AnimationConverter.h/cpp      # OMF → ozz animation
├── BundleConverter.h/cpp         # Create .ozzx bundles
└── main.cpp                      # CLI entry point
```

### Startup Conversion
```
src/xrAnimation/
├── StartupConversionInventory.h/cpp    # Asset scanning
├── LegacyOgfConverter.h/cpp            # Runtime OGF converter
└── LegacyOmfConverter.h/cpp            # Runtime OMF converter
```

### Testing Infrastructure
```
src/xrAnimation/tests/
├── ozz_kinematics_parity_tests.cpp    # Runtime parity
├── xray_to_ozz_converter_tests.cpp    # Converter validation
└── testdata/                           # Generated fixtures
```

### Documentation
```
src/xrAnimation/docs/
├── OZZ_KINEMATICS_INTEGRATION_STATUS.md    # This file
├── OZZ_KINEMATICS_INTEGRATION_DETAILS.md   # Technical deep dive
├── ECS_IK_REFACTOR_STATUS.md               # IK/Ragdoll roadmap
└── ECS_IK_REFACTOR_DETAILS.md              # ECS architecture
```

---

## Dependencies

### Build Requirements
- ozz-animation library (submodule in `Externals/ozz-animation`)
- X-Ray Engine xrCore (existing)
- C++17 compiler
- CMake 3.15+

### Runtime Requirements
- Vulkan 1.3+ (for ozz_animation_viewer)
- GLFW 3.3+ (for ozz_animation_viewer)
- VMA (Vulkan Memory Allocator, header-only)

### Optional
- Blender + blender-xray addon (for asset comparison)
- RenderDoc / Nsight Graphics (for debugging)

---

## Migration from Legacy System

### Backward Compatibility Strategy

**Phase 1: Parallel Runtime** (Current)
- Keep legacy `CKinematics` working
- Add `OzzKinematics` alongside
- Developer toggle to switch between systems
- No gameplay changes

**Phase 2: Gradual Replacement** (Future)
- Convert assets incrementally
- Maintain fallback to legacy for missing `.ozzx`
- Test each asset type independently

**Phase 3: Full Migration** (Long-term)
- Remove legacy animation code
- Pure ozz-animation runtime
- Cleanup legacy file formats

### API Compatibility

**IKinematics Interface:**
- ✅ Full compatibility maintained
- All methods implemented
- Bone lookup by name/ID
- Transform access
- Visibility masks
- Callbacks

**IKinematicsAnimated Interface:**
- ✅ Full compatibility maintained
- Motion playback
- Blend management
- Partition system
- FX channels
- Motion marks

**Lua Script Interface:**
- ✅ No changes required
- Scripts use same API
- Transparent ozz backend

### Testing Strategy

**Parity Testing:**
1. Load same asset in legacy and ozz runtimes
2. Sample same animation at same time
3. Compare bone transforms (tolerance: 0.001)
4. Verify visual output matches

**Regression Testing:**
1. Run full game scenarios
2. Check for animation glitches
3. Verify physics interactions
4. Test all NPC/weapon types

**Performance Testing:**
1. Profile frame times (legacy vs ozz)
2. Measure memory usage
3. Test with increasing character counts
4. Validate CPU/GPU balance

---

## Success Criteria

### MVP (Completed ✅)
- ✅ Converters produce valid `.ozz/.ozzx` files
- ✅ Runtime loads and plays animations
- ✅ Visual output matches legacy system
- ✅ No crashes or memory leaks
- ✅ Parity tests pass

### Production Ready (In Progress 🔄)
- ✅ Startup conversion working
- 🔄 All asset types supported
- 🔄 Performance equal or better than legacy
- 🔄 Full game scenario testing
- 🔄 Documentation complete

### Optimized (Future 🎯)
- 🎯 500+ characters @ 60 FPS
- 🎯 Shared motion libraries
- 🎯 GPU skinning
- 🎯 Parallel animation updates
- 🎯 Memory usage reduced 10x

---

## Resources

### Documentation
- **This File:** Current status overview
- **DETAILS:** `OZZ_KINEMATICS_INTEGRATION_DETAILS.md` - Technical deep dive
- **IK Status:** `ECS_IK_REFACTOR_STATUS.md` - Advanced IK roadmap
- **Agent Docs:** `../CLAUDE.md` - Session context and workflows
- **Commands:** `../CLAUDE_COMMANDS.md` - Quick reference

### External Resources
- [ozz-animation GitHub](https://github.com/guillaumeblanc/ozz-animation)
- [ozz-animation Documentation](http://guillaumeblanc.github.io/ozz-animation/)
- [OpenXRay Project](https://github.com/OpenXRay/xray-16)
- [blender-xray Addon](https://github.com/PavelBlend/blender-xray)

### Tools
- `xray_to_ozz_converter` - Asset conversion CLI
- `ozz_animation_viewer` - Visualization and debugging
- Conversion scripts in repository root

---

## Next Immediate Steps

1. **Harden MVP:**
   - Keep converter/runtime parity tests green
   - Expand automation around bundle hydration
   - Chase down any gameplay regressions

2. **Performance Telemetry:**
   - Gather frame-cost data (legacy vs ozz)
   - Profile memory usage
   - Publish results with configuration guidance

3. **Capture Follow-up Requirements:**
   - Threading strategy
   - GPU skinning approach
   - Richer metadata needs
   - Turn into prioritized roadmap

4. **UI Integration:**
   - Surface startup conversion stage in loading screen
   - Add localization strings for "converting ozz assets"
   - Progress reporting for user visibility

5. **Stability Testing:**
   - Full game playthroughs with ozz runtime
   - Test all weapon types
   - Test all NPC types
   - Verify ragdolls and physics

---

**Document Version:** 1.0
**Maintainer:** OpenXRay Animation Team
**Last Review:** 2025-10-12
