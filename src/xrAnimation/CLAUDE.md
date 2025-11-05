# X-Ray ozz-animation Integration - Agent Documentation

**Document Type:** Agent Session Context & Project Guide
**Last Updated:** 2025-10-12
**Status:** Living Document

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Critical Build Information](#build-info)
3. [Current Status](#current-status)
4. [Key Technical Decisions](#technical-decisions)
5. [Implementation Insights](#implementation-insights)
6. [Session Context & MVP Notes](#session-context)
7. [Resources & Documentation](#resources)

---

## Project Overview {#project-overview}

### Mission
Integrate ozz-animation (modern, SIMD-optimized animation library) into OpenXRay to replace the legacy X-Ray animation system while maintaining full backward compatibility.

### Timeline: MVP Delivered (10-12 weeks completed)

#### Phase 1: Foundation (Weeks 1-2) - ✅ COMPLETED
- ✅ Build system & initial study
- ✅ Existing system analysis
- ✅ xrAnimation module created
- ✅ Dependencies resolved (ozz-animation, imgui, SDL2)
- ✅ IKinematics interface analyzed
- ✅ Usage patterns traced
- ✅ Integration points documented

#### Phase 2: Prototype & Converter (Weeks 3-5) - ✅ COMPLETED
- ✅ Basic ozz integration
- ✅ OGF→ozz skeleton converter
- ✅ OMF→ozz animation converter
- ✅ Asset conversion pipeline working
- ✅ Coordinate system transformation solved
- ✅ Motion marks format fixed
- ✅ Per-bone animation distribution working

#### Phase 3: Core Implementation (Weeks 6-8) - ✅ COMPLETED
- ✅ Full animation system (OzzAnimationSystem)
- ✅ Runtime façade (OzzKinematics, OzzKinematicsAnimated)
- ✅ Three-tier architecture (Core, Static, Animated)
- ✅ Game integration via COzzKinematicsVisual
- ✅ Asset conversion tools production-ready

#### Phase 4: Polish & Optimization (Weeks 9-10) - ✅ MVP COMPLETE
- ✅ Testing & parity validation
- ✅ ozz_animation_viewer tool
- ✅ Comprehensive documentation
- 🔄 Performance optimization (ongoing)

---

## Critical Build Information {#build-info}

### ⚠️ CRITICAL: Build System Requirements

**CMake Build Directory:**
- ✅ ALWAYS use `./build` (NOT `./out/build`)
- ✅ ALWAYS use `-j$(nproc)` flag for parallel builds
- ❌ NEVER use Visual Studio's CMake integration (causes path issues)

**Example Commands:**
```bash
# Configure (first time)
cmake -S xray-16 -B build -DCMAKE_BUILD_TYPE=Debug

# Build (parallel)
cmake --build ./build -j$(nproc)

# Build specific target
cmake --build ./build --target xrAnimation -j$(nproc)

# Run tests
ctest --test-dir ./build --output-on-failure
```

**Important Paths:**
- **Source:** `/mnt/f/modding/claude_sessions/xray-16/`
- **Build:** `/mnt/f/modding/claude_sessions/xray-16/build/`
- **Binaries:** `./build/bin/Debug/` or `./build/bin/Release/`
- **Test Assets:** `./res/gamedata/meshes/actors/`

---

## Current Status {#current-status}

### MVP Status: ✅ Delivered

**What Works:**
- ✅ Asset conversion (OGF/OMF → ozz/ozzx)
- ✅ Runtime loading and playback
- ✅ Full IKinematics/IKinematicsAnimated compatibility
- ✅ Visual integration with COzzKinematicsVisual
- ✅ Parity tests passing
- ✅ Startup conversion system
- ✅ Three-tier architecture (eliminates "$editor" spam)
- ✅ Extended bone metadata (physics, IK constraints)

**What's Next (Post-MVP):**
- 🔄 Parallel animation updates (500+ characters)
- 🔄 Shared motion container (memory optimization)
- 🔄 GPU skinning
- 🔄 Advanced IK/Ragdoll systems (see ECS_IK_REFACTOR_STATUS.md)

### Test Infrastructure
- **Converter Tests:** `xrAnimation_converter_tests`
- **Parity Tests:** `ozz_kinematics_tests`
- **Visualization:** `ozz_animation_viewer` (with profiling)
- **Test Fixtures:** `src/xrAnimation/tests/testdata/`

---

## Key Technical Decisions {#technical-decisions}

### 1. Use X-Ray Types & Conventions
```cpp
// ✅ DO: Use X-Ray types
xr_vector<T>          // Not std::vector
xr_unique_ptr<T>      // Not std::unique_ptr
xr_new<T>() / xr_delete()  // Not new/delete
Fmatrix, Fvector, Fquaternion  // X-Ray math types
Msg() / Msgf()        // Not printf/cout
IReader* / IWriter*   // X-Ray file I/O
FS.r_open() / FS.r_close()  // Virtual file system

// ❌ DON'T: Use STL directly
std::vector<T>        // Use xr_vector
new/delete            // Use xr_new/xr_delete
std::cout / printf    // Use Msg()
std::fstream          // Use IReader/IWriter
```

### 2. Incremental Integration Strategy
- Keep legacy `CKinematics` working alongside `OzzKinematics`
- Developer toggle: `g_use_ozz_visuals` console variable
- Fallback to legacy on missing `.ozzx` bundles
- No gameplay code changes during migration
- Lua scripts remain unchanged

### 3. Full API Compatibility
- `IKinematics` interface fully implemented
- `IKinematicsAnimated` interface fully implemented
- Legacy bone callback system preserved
- Motion partition system maintained
- Blend pool compatible with `CBlend` usage patterns

### 4. Asset Conversion Pipeline
- Startup conversion with digest tracking
- CLI tool for batch conversion
- `.ozzx` bundle format (skeleton + mesh + metadata)
- Extended bone records (physics, IK, collision hints)

---

## Implementation Insights {#implementation-insights}

### Critical Discoveries

#### 1. Coordinate System Conversion ✅ SOLVED
**Problem:** X-Ray uses Y-up, ozz/OpenGL uses Z-up

**Solution:**
```cpp
// Transform: (X, Y, Z) → (X, Z, -Y)
Fmatrix converted;
converted.i.set(xray.i.x, xray.i.z, -xray.i.y);  // X-axis
converted.j.set(xray.j.x, xray.j.z, -xray.j.y);  // Y-axis
converted.k.set(xray.k.x, xray.k.z, -xray.k.y);  // Z-axis
converted.c.set(xray.c.x, xray.c.z, -xray.c.y);  // Position
```

#### 2. OMF Motion Marks Format ✅ FIXED
**Problem:** Motion marks use `\r\n` termination, NOT null termination

**Solution:**
```cpp
// Read until '\n', strip trailing '\r'
xr_string ReadMotionMark(IReader* reader) {
    xr_string result;
    char ch;
    while (reader->r(&ch, sizeof(ch))) {
        if (ch == '\n') break;
        if (ch != '\r') result += ch;
    }
    return result;
}
```

#### 3. Per-Bone Animation Distribution ✅ FIXED
**Problem:** OMF contains motion data for ALL bones, must distribute correctly

**Solution:**
```cpp
// Each OMFBoneMotion contains data for multiple bones
struct OMFBoneMotion {
    xr_string motion_name;
    xr_vector<OMFBoneData> bone_data;  // One entry per bone
};

// Map bone IDs to correct animation tracks during conversion
for (const OMFBoneData& bone : motion.bone_data) {
    u16 bone_idx = skeleton.FindBoneByName(bone.bone_name);
    ozz_animation.tracks[bone_idx] = ConvertKeyframes(bone.keyframes);
}
```

#### 4. IK Data Parsing ✅ FIXED
**Problem:** Not all bones have IK data in OGF (only those with valid shapes)

**Solution:**
```cpp
bool SBoneShape::Valid() const {
    return (type == stBox && !box.IsZero()) ||
           (type == stSphere && sphere.R > 0) ||
           (type == stCylinder && cylinder.IsValid());
}

// Read IK data only if shape is valid
if (shape.Valid()) {
    // Read SJointIKData, mass, center_of_mass
} else {
    // Use OBB transform as fallback
}
```

#### 5. Quaternion Conjugation ✅ FIXED
**Problem:** Left-handed (X-Ray) vs right-handed (ozz) coordinate systems

**Solution:**
```cpp
// Convert from left-handed to right-handed
Fquaternion quat;
quat.set(xray_matrix);
// Conjugate: negate x, y, z components, keep w
result.rotation = ozz::math::Quaternion(-quat.x, -quat.y, -quat.z, quat.w);
```

### ozz-animation SIMD Access Patterns

⚠️ **CRITICAL:** ozz uses SIMD types - cannot access members directly!

**❌ WRONG:**
```cpp
ozz::math::SimdFloat4 vec;
float x = vec.x;  // ERROR: 'x' is not a member of '__m128'

ozz::math::Float4x4 matrix;
float m11 = matrix.cols[0].x;  // ERROR: cols[0] is SimdFloat4
```

**✅ CORRECT:**
```cpp
// For SimdFloat4 - use GetX/Y/Z/W functions
ozz::math::SimdFloat4 vec;
float x = ozz::math::GetX(vec);
float y = ozz::math::GetY(vec);

// For Float4x4 matrices
ozz::math::Float4x4 matrix;
float m11 = ozz::math::GetX(matrix.cols[0]);
float m21 = ozz::math::GetY(matrix.cols[0]);

// For SoaTransform (4 transforms in SoA layout)
ozz::math::SoaTransform transforms;
float tx0 = ozz::math::GetX(transforms.translation.x);  // Transform 0
float tx1 = ozz::math::GetY(transforms.translation.x);  // Transform 1
```

**Key SIMD Types:**
- `SimdFloat4` - 4 floats in SIMD register (__m128)
- `Float4x4` - 4x4 matrix with SimdFloat4 columns
- `SoaTransform` - 4 transforms in Structure-of-Arrays layout
- `SoaFloat3` - 4 Float3 vectors in SoA layout
- `SoaQuaternion` - 4 quaternions in SoA layout

### CKinematicsAnimated Architecture

**Blend Pool:**
```cpp
// Fixed-size blend pool (no dynamic allocation)
svector<CBlend, MAX_BLENDED_POOL> blend_pool;

// Active blends tracked separately
xr_vector<CBlend*> m_Blends;  // Pointers into pool
```

**Motion Storage (Bone-Major Layout):**
```cpp
struct SMotionsSlot {
    shared_motions motions;           // Shared motion data
    BoneMotionsVec bone_motions;      // xr_vector<MotionVec*>
    //             ^^^ Each entry is MotionVec* (pointer to vector of CMotions)
};

// Access pattern (critical for performance)
CMotion* motion = m_Motions[slot].bone_motions[bone_id]->at(motion_idx);
//                          ^^^^ BONE-MAJOR: bone first, then motion
```

**Update Pipeline:**
```cpp
UpdateTracks()
  → LL_UpdateTracks()
    → For each blend: blend->timeCurrent += dt
    → CalculateBones_Invalidate()
      → CalculateBones(TRUE)
        → Bone_Calculate() for each bone
          → CLBone() or BuildBoneMatrix()
            → Apply callbacks, additional transforms
```

**Partition System:**
```cpp
// Allows independent animation of body parts
#define MAX_PARTS 4
CPartition m_Partition[MAX_PARTS];  // e.g., LEGS, TORSO, HEAD, etc.

// Usage
MotionID legs_id = {.slot=0, .idx=walk_motion};
MotionID torso_id = {.slot=1, .idx=reload_motion};
PlayCycle(legs_id, 1.0f, LEGS_PARTITION);
PlayCycle(torso_id, 1.0f, TORSO_PARTITION);
```

### Animation Usage Patterns (from ActorAnimation.cpp)

**Common Pattern:**
```cpp
IKinematicsAnimated* K = smart_cast<IKinematicsAnimated*>(Visual());

// State-based motion selection
if (mstate_rl & mcFwd)
    M_legs = AS->legs_fwd;      // MotionID from state object
else if (mstate_rl & mcBack)
    M_legs = AS->legs_back;

// Play with callback
K->PlayCycle(M_legs, true, legs_play_callback, this);

// Torso override for weapons
STorsoWpn* TW = &ST->m_torso[weapon_slot];
M_torso = W->IsZoomed() ? TW->zoom : TW->moving[moving_idx];
K->PlayCycle(M_torso, true, nullptr, nullptr, TORSO_PARTITION);
```

### File Format Details (from xrSDK)

**OGF Chunk IDs:**
```cpp
#define OGF_HEADER        1   // File header
#define OGF_S_BONE_NAMES  13  // 0x0D - Bone hierarchy
#define OGF_S_MOTIONS     14  // 0x0E - Motion references
#define OGF_S_SMPARAMS    15  // 0x0F - Motion parameters
#define OGF_S_IKDATA      16  // 0x10 - IK constraints, shapes
#define OGF_S_USERDATA    17  // 0x11 - User metadata
#define OGF_S_DESC        18  // 0x12 - Description
```

**OGF_S_BONE_NAMES Structure:**
```cpp
// Chunk 13 (0x0D)
u32 bone_count;
for (u32 i = 0; i < bone_count; ++i) {
    stringZ bone_name;
    stringZ parent_name;  // Empty string if root
    Fobb obb;             // Oriented bounding box (60 bytes)
}
```

**OGF_S_IKDATA Structure (version 0x0001):**
```cpp
// Only present if SBoneShape::Valid() returns true!
for each bone (if valid) {
    SJointIKData ik_data;   // Joint type, limits, spring/damping
    float mass;
    Fvector center_of_mass;
}
```

**SBoneShape Structure (112 bytes total):**
```cpp
struct SBoneShape {
    u16 type;           // 2 bytes: stNone=0, stBox=1, stSphere=2, stCylinder=3
    Flags16 flags;      // 2 bytes
    Fobb box;          // 60 bytes (15 floats)
    Fsphere sphere;    // 16 bytes (4 floats)
    Fcylinder cylinder;// 32 bytes (8 floats)

    bool Valid() const {
        return (type == stBox && !box.IsZero()) ||
               (type == stSphere && sphere.R > 0) ||
               (type == stCylinder && cylinder.IsValid());
    }
};
```

**OMF Compression Format (Version 4):**
```cpp
// Flags
#define flTKeyPresent  (1 << 0)  // Translation keys present
#define flRKeyAbsent   (1 << 1)  // Rotation keys absent
#define flTKey16IsBit  (1 << 2)  // Use 16-bit translation (else 8-bit)

// Quaternion: Always 16-bit per component
struct CKeyQR {
    i16 x, y, z, w;  // Range: -32767..32767 → -1.0..1.0
};

// Translation: 8-bit or 16-bit based on flTKey16IsBit
struct CKeyQT8 {
    u8 x, y, z;  // Range: 0..255 → compressed range
};

struct CKeyQT16 {
    u16 x, y, z;  // Range: 0..65535 → compressed range
};
```

---

## Session Context & MVP Notes {#session-context}

### Project Milestones

**September 2025 - Project Start**
- Initial ozz-animation research
- Build system setup
- Module creation

**Week 1-2: Foundation**
- X-Ray animation system analysis
- Interface documentation created
- Integration strategy defined

**Week 3: Converter Breakthrough**
- OGF parser working
- OMF parser working
- First successful skeleton conversion

**Week 4: Critical Bug Fixes**
- Coordinate system solved
- Motion marks format fixed
- Per-bone animation distribution
- IK data parsing fixed
- Quaternion conjugation applied

**Week 5: Blender Validation**
- Compared with blender-xray addon
- Verified file format specifications
- Confirmed coordinate transformations
- All conversions validated

**Week 6-8: Runtime Implementation**
- OzzAnimationSystem core
- OzzKinematicsAnimated wrapper
- Channel and partition support
- Callback system integration

**Week 9: Three-Tier Architecture**
- Split into OzzKinematicsCore, OzzKinematics, OzzKinematicsAnimated
- Eliminated "$editor" spam for static props
- Memory optimization for non-animated models

**Week 10: Polish & Testing**
- Parity tests created
- ozz_animation_viewer tool
- Documentation comprehensive
- MVP delivered

**October 2025 - Post-MVP Planning**
- ECS IK/Ragdoll system design
- Parallel animation optimization roadmap
- Shared motion container architecture
- Startup conversion system

### Development Environment

**Platform:** WSL2 (Ubuntu on Windows)
**Compiler:** GCC 11.4+
**Build System:** CMake 3.20+
**IDE:** VS Code with Remote-WSL

**Key Directories:**
```
/mnt/f/modding/claude_sessions/xray-16/
├── build/                              # CMake build output
├── src/
│   ├── xrAnimation/                    # Animation module
│   │   ├── docs/                       # Documentation
│   │   │   ├── OZZ_KINEMATICS_INTEGRATION_STATUS.md
│   │   │   ├── OZZ_KINEMATICS_INTEGRATION_DETAILS.md
│   │   │   ├── ECS_IK_REFACTOR_STATUS.md
│   │   │   └── ECS_IK_REFACTOR_DETAILS.md
│   │   ├── tools/                      # Converter & viewer
│   │   │   ├── xray_to_ozz_converter/  # CLI converter
│   │   │   ├── ozz_animation_viewer/   # Visualization tool
│   │   │   └── CLAUDE.md               # Tool documentation
│   │   ├── tests/                      # Test suite
│   │   └── CLAUDE.md                   # This file
│   └── Layers/xrRender/
│       └── OzzKinematicsVisual.h/cpp   # Visual integration
└── res/gamedata/meshes/actors/         # Test assets
```

### Converter Tool Usage

**⚠️ IMPORTANT:** Output parameter is a DIRECTORY, not a filename!

```bash
# Convert skeleton
xray_to_ozz_converter skeleton stalker_hero_1.ogf .
# → Creates ./stalker_hero_1.ozz

xray_to_ozz_converter skeleton stalker_hero_1.ogf /output/path/
# → Creates /output/path/stalker_hero_1.ozz

# Convert animation (skeleton.ogf comes FIRST!)
xray_to_ozz_converter animation stalker_hero_1.ogf critical_hit.omf .
# → Creates ./critical_hit.ozz

xray_to_ozz_converter animation stalker_hero_1.ogf walk.omf /output/path/
# → Creates /output/path/walk.ozz

# Batch convert
xray_to_ozz_converter batch animations/ ozz_out/ stalker_hero_1.ogf [-optimize]
```

### Test Assets

**Location:** `res/gamedata/meshes/actors/`

**Available Assets:**
- `stalker_hero_1.ogf` - Player skeleton (47 bones)
- `critical_hit_grup_1.omf` - Death animation
- `stalker_animation.omf` - Various player animations
- `stalker_bandit/*.ogf` - Bandit NPC models

### Console Commands

**Runtime Testing:**
```
# Enable ozz runtime
g_use_ozz_visuals 1

# Debug commands
debug_dump_ozz_palette          # Snapshot bone matrices
debug_dump_ozz_palette_toggle   # Auto-dump comparison
g_dev_ozz_actor                 # Hot-swap player model
g_dev_ozz_animation_list        # List available motions
g_dev_ozz_animation <name>      # Play specific animation
```

### Performance Targets

**Current (MVP):**
- 50-100 animated characters functional
- Animation update not yet optimized
- Per-instance motion libraries

**Target (Post-Optimization):**
- 500-1000 characters @ 60 FPS
- <4ms animation update budget
- Shared motion libraries (10-50x memory reduction)
- 70%+ CPU utilization (parallel updates)

---

## Resources & Documentation {#resources}

### Project Documentation

**Status & Planning:**
- `docs/OZZ_KINEMATICS_INTEGRATION_STATUS.md` - Current status, roadmap
- `docs/OZZ_KINEMATICS_INTEGRATION_DETAILS.md` - Technical deep dive, extended metadata, shared motion container
- `docs/ECS_IK_REFACTOR_STATUS.md` - Advanced IK/ragdoll system plan (6 phases, 12-16 weeks)
- `docs/ECS_IK_REFACTOR_DETAILS.md` - ECS architecture, migration strategy

**Agent Documentation:**
- `CLAUDE.md` (this file) - Session context, key decisions, implementation insights
- `CLAUDE_COMMANDS.md` - Quick reference commands
- `CLAUDE_CONTEXT.md` - Code patterns, technical context

**Tool Documentation:**
- `tools/CLAUDE.md` - Converter tool usage
- `tools/VULKAN_RENDERER_DESIGN.md` - Vulkan renderer design (ozz_animation_viewer)

### External Resources

**ozz-animation:**
- [GitHub Repository](https://github.com/guillaumeblanc/ozz-animation)
- [Documentation](http://guillaumeblanc.github.io/ozz-animation/)
- [Samples](https://github.com/guillaumeblanc/ozz-animation/tree/master/samples)

**X-Ray Engine:**
- [OpenXRay GitHub](https://github.com/OpenXRay/xray-16)
- [blender-xray Addon](https://github.com/PavelBlend/blender-xray) - Reference for file formats
- [xrSDK](https://github.com/OpenXRay/xray-16/tree/xd_dev/SDK) - Legacy SDK with format specs

**EnTT (for ECS):**
- [GitHub Repository](https://github.com/skypjack/entt)
- [Documentation](https://github.com/skypjack/entt/wiki)

### Common Issues & Solutions

**Issue: CMake can't find ozz-animation**
```bash
# Solution: Ensure submodule is initialized
git submodule update --init --recursive Externals/ozz-animation
```

**Issue: Build fails with "undefined reference to ozz::*"**
```bash
# Solution: Clean build directory
rm -rf build/
cmake -S xray-16 -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build ./build -j$(nproc)
```

**Issue: Converter crashes on loading OGF**
```bash
# Solution: Verify file path and X-Ray FS initialization
Msg("Loading OGF: %s", path.c_str());
IReader* reader = FS.r_open(path.c_str());
if (!reader) {
    Msg("! Failed to open OGF: %s", path.c_str());
    return false;
}
```

**Issue: Skeleton collapses to (0,0,0)**
```bash
# Solution: Check IK data parsing - not all bones have valid shapes
if (bone_shape.Valid()) {
    // Read IK data
} else {
    // Use OBB transform fallback
}
```

**Issue: Animation looks wrong (twisted/mirrored)**
```bash
# Solution: Apply quaternion conjugation for handedness conversion
Fquaternion quat;
quat.set(xray_matrix);
result.rotation = ozz::math::Quaternion(-quat.x, -quat.y, -quat.z, quat.w);
```

---

## Next Steps (Post-MVP)

### Immediate (Weeks 11-12)
1. ✅ Complete documentation consolidation
2. 🔄 Expand test coverage (weapon animations, monster animations)
3. 🔄 Profile performance in full game scenarios
4. 🔄 Begin ECS IK Phase 1.1 (Ground Probing)

### Short-term (Weeks 13-16)
1. 🎯 Implement parallel animation updates
2. 🎯 Test with 500+ animated characters
3. 🎯 Begin shared motion container implementation (if needed)
4. 🎯 Continue ECS IK Phase 1 (Enhanced Foot IK)

### Long-term (Months 5-6)
1. 🎯 GPU skinning implementation
2. 🎯 Complete ECS IK/Ragdoll systems (Phases 2-6)
3. 🎯 Hot-reload support for animations
4. 🎯 Advanced modding API

---

**Document Version:** 2.0
**Maintainer:** OpenXRay Animation Team
**Last Review:** 2025-10-12
