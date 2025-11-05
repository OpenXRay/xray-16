# Ozz-Animation Integration - Technical Details

**Document Type:** Technical Deep Dive
**Last Updated:** 2025-10-12
**Status:** Implementation guide for advanced features

---

## Table of Contents

1. [Extended Bone Records & Physics Metadata](#extended-bone-records)
2. [Shared Motion Container Architecture](#shared-motion-container)
3. [Implementation Patterns](#implementation-patterns)
4. [Performance Optimizations](#performance-optimizations)

---

## Extended Bone Records & Physics Metadata {#extended-bone-records}

### Overview

The `.ozzx` bundle format version 2 includes extended bone metadata beyond basic skeleton hierarchy, enabling advanced physics, IK, and gameplay systems without runtime recomputation.

### Current Status (2025-10-02)

✅ **Implemented:**
- `.ozzx` bundle header targets version 2
- Tagged metadata block carries extended bone payload
- Converter emits rest-length, dominant axis, collision hints, physics descriptors
- Runtime hydrates `CBoneData` via `ApplyExtendedBoneMetadata`
- Parity automation covers metadata hydration (`OzzKinematicsAppliesBoneMetadata`)

### Extended BoneRecord Structure

```cpp
struct BoneRecord {
    // === Existing Fields (Legacy Parity) ===
    std::string name;
    std::string parent_name;
    int parent_index{-1};

    // Rest pose transforms
    Fmatrix local_transform{};        // Local space (relative to parent)
    Fmatrix global_transform{};       // World space (accumulated)

    // Physics properties (from IK chunk)
    float mass{0.f};
    Fvector center_of_mass{};
    SBoneShape shape{};               // Box/sphere/cylinder collision
    SJointIKData joint_ik{};          // Joint limits, spring/damping
    shared_str game_mtl;              // Material name ("flesh", "bone_hand", etc.)

    // === Newly Synthesized Data (Version 2) ===

    // Geometric properties
    float rest_length{0.f};                      // Distance from parent to child origin
    Fvector dominant_axis{};                     // Normalized axis of max extent in local space
    Fvector local_aabb_min{}, local_aabb_max{};  // AABB enclosing the bone in parent space

    // Cached transforms
    Fmatrix inverse_global_transform{};          // Cached inverse for world→bone projections

    // Physics scaffolding
    Fvector inertia_tensor{};                    // Diagonal inertia approximation (shape + mass)
    float volume{0.f};                           // Derived from shape geometry

    // Collision layer hints
    Flags32 collision_layers;                    // Synthesized group bits (soft tissue, rigid, weapon)

    // Attachment hints
    bool ground_contact_candidate{false};        // Heuristics (feet, claws)
    bool weapon_anchor_candidate{false};         // Heuristics (hands, weapon bones)
};
```

### Computation Details

#### 1. Rest Length
**Purpose:** Limb reach calculations, IK constraints, footstep placement

**Computation:**
```cpp
float ComputeRestLength(const BoneRecord& bone, const BoneRecord& parent) {
    Fvector child_pos = bone.local_transform.c;
    return child_pos.magnitude();  // Distance from parent origin to child origin
}
```

#### 2. Dominant Axis
**Purpose:** Limb orientation hints, procedural animation, alignment checks

**Computation:**
```cpp
Fvector ComputeDominantAxis(const BoneRecord& bone) {
    // Transform shape basis vectors into parent space
    Fvector axes[3] = {
        bone.local_transform.i,  // X-axis
        bone.local_transform.j,  // Y-axis
        bone.local_transform.k   // Z-axis
    };

    // Find axis with largest extent based on shape type
    if (bone.shape.type == SBoneShape::stBox) {
        // Use box half-extents to weight axes
        float extents[3] = {
            bone.shape.box.m_halfsize.x,
            bone.shape.box.m_halfsize.y,
            bone.shape.box.m_halfsize.z
        };
        int max_idx = std::max_element(extents, extents + 3) - extents;
        return axes[max_idx].normalize();
    }
    else if (bone.shape.type == SBoneShape::stCylinder) {
        // Cylinder axis is Y in local space
        return axes[1].normalize();
    }

    // Fallback: direction from parent to child
    return bone.local_transform.c.normalize();
}
```

#### 3. Local AABB
**Purpose:** Quick volume queries, penetration testing, footstep contact

**Computation:**
```cpp
void ComputeLocalAABB(BoneRecord& bone) {
    // Sample collision shape corners/hull
    xr_vector<Fvector> sample_points;

    switch (bone.shape.type) {
        case SBoneShape::stBox: {
            // 8 box corners
            Fvector half = bone.shape.box.m_halfsize;
            for (int x = -1; x <= 1; x += 2)
                for (int y = -1; y <= 1; y += 2)
                    for (int z = -1; z <= 1; z += 2)
                        sample_points.push_back(Fvector{x*half.x, y*half.y, z*half.z});
            break;
        }
        case SBoneShape::stSphere: {
            // 6 sphere extremes (±X, ±Y, ±Z)
            float r = bone.shape.sphere.R;
            sample_points = {
                {r, 0, 0}, {-r, 0, 0},
                {0, r, 0}, {0, -r, 0},
                {0, 0, r}, {0, 0, -r}
            };
            break;
        }
        case SBoneShape::stCylinder: {
            // Cylinder hull (top/bottom circles)
            float r = bone.shape.cylinder.m_radius;
            float h = bone.shape.cylinder.m_height;
            for (int i = 0; i < 8; ++i) {
                float angle = i * (PI_MUL_2 / 8.f);
                float x = r * cos(angle);
                float z = r * sin(angle);
                sample_points.push_back(Fvector{x, h/2, z});
                sample_points.push_back(Fvector{x, -h/2, z});
            }
            break;
        }
    }

    // Transform by local_transform and compute bounds
    bone.local_aabb_min = Fvector{FLT_MAX, FLT_MAX, FLT_MAX};
    bone.local_aabb_max = Fvector{-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (const Fvector& p : sample_points) {
        Fvector transformed = bone.local_transform.transform_tiny(p);
        bone.local_aabb_min.min(transformed);
        bone.local_aabb_max.max(transformed);
    }
}
```

#### 4. Inertia Tensor & Volume
**Purpose:** Ragdoll tuning, physics simulation, mass distribution

**Computation:**
```cpp
void ComputePhysicsProperties(BoneRecord& bone) {
    float mass = bone.mass;

    switch (bone.shape.type) {
        case SBoneShape::stBox: {
            Fvector half = bone.shape.box.m_halfsize;
            bone.volume = 8.f * half.x * half.y * half.z;

            // Box inertia tensor (diagonal): I = (m/12) * (dy² + dz², dx² + dz², dx² + dy²)
            float dx = 2 * half.x;
            float dy = 2 * half.y;
            float dz = 2 * half.z;
            bone.inertia_tensor = Fvector{
                (mass / 12.f) * (dy*dy + dz*dz),
                (mass / 12.f) * (dx*dx + dz*dz),
                (mass / 12.f) * (dx*dx + dy*dy)
            };
            break;
        }
        case SBoneShape::stSphere: {
            float r = bone.shape.sphere.R;
            bone.volume = (4.f / 3.f) * PI * r * r * r;

            // Sphere inertia: I = (2/5) * m * r²
            float I = (2.f / 5.f) * mass * r * r;
            bone.inertia_tensor = Fvector{I, I, I};
            break;
        }
        case SBoneShape::stCylinder: {
            float r = bone.shape.cylinder.m_radius;
            float h = bone.shape.cylinder.m_height;
            bone.volume = PI * r * r * h;

            // Cylinder inertia (Y-axis): Ix = Iz = (m/12) * (3r² + h²), Iy = (m/2) * r²
            bone.inertia_tensor = Fvector{
                (mass / 12.f) * (3*r*r + h*h),
                (mass / 2.f) * r * r,
                (mass / 12.f) * (3*r*r + h*h)
            };
            break;
        }
    }
}
```

#### 5. Collision Layers
**Purpose:** Layer-based collision filtering, material responses

**Heuristics:**
```cpp
Flags32 SynthesizeCollisionLayers(const BoneRecord& bone) {
    Flags32 layers{};

    // Material-based classification
    if (bone.game_mtl.contains("flesh") || bone.game_mtl.contains("body"))
        layers.set(COLLISION_LAYER_SOFT_TISSUE, TRUE);
    else if (bone.game_mtl.contains("bone") || bone.game_mtl.contains("hard"))
        layers.set(COLLISION_LAYER_RIGID, TRUE);

    if (bone.game_mtl.contains("weapon") || bone.game_mtl.contains("wpn"))
        layers.set(COLLISION_LAYER_WEAPON, TRUE);

    // Name-based classification
    if (bone.name.contains("head") || bone.name.contains("skull"))
        layers.set(COLLISION_LAYER_HEAD, TRUE);

    if (bone.name.contains("hand") || bone.name.contains("palm"))
        layers.set(COLLISION_LAYER_HAND, TRUE);

    return layers;
}
```

#### 6. Ground Contact Candidates
**Purpose:** Foot IK, footstep sounds, climbing

**Heuristics:**
```cpp
bool IsGroundContactCandidate(const BoneRecord& bone, const xr_vector<BoneRecord>& skeleton) {
    // Check bone name
    if (bone.name.contains("foot") || bone.name.contains("toe") || bone.name.contains("claw"))
        return true;

    // Check if dominant axis aligns with -Y (down)
    if (bone.dominant_axis.dotproduct(Fvector{0, -1, 0}) > 0.8f) {
        // Check if near ground (global Y position near min)
        float min_y = FLT_MAX;
        for (const BoneRecord& b : skeleton)
            min_y = std::min(min_y, b.global_transform.c.y);

        if (bone.global_transform.c.y - min_y < 0.1f)  // Within 10cm of lowest point
            return true;
    }

    return false;
}
```

#### 7. Weapon Anchor Candidates
**Purpose:** Weapon attachment, item interactions, two-handed IK

**Heuristics:**
```cpp
bool IsWeaponAnchorCandidate(const BoneRecord& bone) {
    // Name-based detection
    if (bone.name.contains("weapon") || bone.name.contains("wpn"))
        return true;

    if (bone.name.contains("hand") && (bone.name.contains("r_") || bone.name.contains("right")))
        return true;  // Right hand is primary weapon grip

    // Check for weapon-tagged child meshes (would require mesh data)
    // TODO: Integrate with mesh parsing if needed

    return false;
}
```

### Bundle Format Update

**Version 2 Layout:**
```
.ozzx Bundle
├── Header (version=2, chunk count, metadata)
├── Skeleton Chunk (ozz binary format)
├── Mesh Chunk (vertex/index buffers)
└── Bone Metadata Chunk (NEW in v2)
    ├── count: u32
    └── records[count]:
        ├── name: string
        ├── rest_length: float
        ├── dominant_axis: vec3
        ├── local_aabb_min: vec3
        ├── local_aabb_max: vec3
        ├── inverse_global_transform: mat4
        ├── inertia_tensor: vec3
        ├── volume: float
        ├── collision_layers: u32
        ├── ground_contact_candidate: bool
        └── weapon_anchor_candidate: bool
```

**Serialization:**
```cpp
void WriteExtendedBoneMetadata(IWriter& writer, const xr_vector<BoneRecord>& bones) {
    writer.w_u32(bones.size());

    for (const BoneRecord& bone : bones) {
        writer.w_stringZ(bone.name);
        writer.w_float(bone.rest_length);
        writer.w_fvector3(bone.dominant_axis);
        writer.w_fvector3(bone.local_aabb_min);
        writer.w_fvector3(bone.local_aabb_max);
        writer.w_fmatrix(bone.inverse_global_transform);
        writer.w_fvector3(bone.inertia_tensor);
        writer.w_float(bone.volume);
        writer.w_u32(bone.collision_layers.get());
        writer.w_u8(bone.ground_contact_candidate ? 1 : 0);
        writer.w_u8(bone.weapon_anchor_candidate ? 1 : 0);
    }
}
```

### Runtime Hydration

**Loading from Bundle:**
```cpp
void COzzKinematicsVisual::HydrateExtendedBoneMetadata(IReader& reader) {
    u32 bone_count = reader.r_u32();

    for (u32 i = 0; i < bone_count; ++i) {
        shared_str bone_name = reader.r_stringZ();
        u16 bone_idx = LL_BoneID(bone_name);

        if (bone_idx == BI_NONE) {
            Msg("! [OzzVisual] Unknown bone '%s' in metadata", bone_name.c_str());
            // Skip this bone's data
            reader.advance(sizeof(ExtendedBoneData));
            continue;
        }

        CBoneData* bone_data = bones[bone_idx];

        // Read extended metadata
        bone_data->rest_length = reader.r_float();
        bone_data->dominant_axis = reader.r_fvector3();
        bone_data->local_aabb_min = reader.r_fvector3();
        bone_data->local_aabb_max = reader.r_fvector3();
        bone_data->inverse_global_transform = reader.r_fmatrix();
        bone_data->inertia_tensor = reader.r_fvector3();
        bone_data->volume = reader.r_float();
        bone_data->collision_layers.assign(reader.r_u32());
        bone_data->ground_contact_candidate = reader.r_u8() != 0;
        bone_data->weapon_anchor_candidate = reader.r_u8() != 0;
    }
}
```

### Validation

**Parity Test:**
```cpp
TEST(OzzKinematicsMetadata, AppliesBoneMetadata) {
    // Load .ozzx bundle with extended metadata
    OzzKinematicsAnimated kinematics;
    kinematics.InitializeFromOzzxBundle("stalker_hero.ozzx");

    // Verify extended properties were hydrated
    u16 foot_bone = kinematics.LL_BoneID("bip01_l_foot");
    ASSERT_NE(foot_bone, BI_NONE);

    const CBoneData* foot = kinematics.GetBoneData(foot_bone);
    ASSERT_TRUE(foot->ground_contact_candidate);
    EXPECT_GT(foot->rest_length, 0.f);
    EXPECT_GT(foot->volume, 0.f);
}
```

---

## Shared Motion Container Architecture {#shared-motion-container}

### Problem Statement

**Current Issue:**
```cpp
// Each instance loads its own copy of motion data
OzzKinematicsAnimated instance1;  // Loads stalker_animation.ozz (50MB)
OzzKinematicsAnimated instance2;  // Loads stalker_animation.ozz (50MB) ← DUPLICATE!
OzzKinematicsAnimated instance3;  // Loads stalker_animation.ozz (50MB) ← DUPLICATE!
```

**Impact:**
- 10 NPCs with 50MB motions = 500MB wasted memory
- Slow startup (repeated disk I/O)
- Cache thrashing from duplicate data

### Legacy Reference: `g_pMotionsContainer`

**How Legacy System Works:**
```cpp
// Global singleton (created by CModelPool)
motions_container* g_pMotionsContainer = xr_new<motions_container>();

// Deduplication via dock()
motions_value* motions_container::dock(shared_str key, IReader* data, vecBones* bones) {
    // 1. Check if already loaded
    auto I = container.find(key);
    if (I != container.end())
        return I->second;  // ← REUSE existing!

    // 2. Not found - load it
    result = xr_new<motions_value>();
    result->load(key.c_str(), data, bones);
    container.insert(std::make_pair(key, result));
    return result;
}
```

**Key Benefits:**
- First request loads from disk
- Subsequent requests return cached pointer (instant!)
- Reference counting for lifetime management

### Proposed Architecture

**Memory Layout:**
```
┌─────────────────────────────────────────────┐
│ CModelPool (owns g_pOzzMotionsContainer)    │
└─────────────────────────────────────────────┘
                    │
                    ▼
┌──────────────────────────────────────────────────────────────┐
│ g_pOzzMotionsContainer (global singleton)                    │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ "stalker_animation.ozz" → OzzMotionsValue*             │  │
│  │   - Handle ID: 0x1234ABCD                              │  │
│  │   - Skeleton fingerprint: 0xDEADBEEF                   │  │
│  │   - Ref count: 3 (atomic)                              │  │
│  │   - Bone-major layout:                                 │  │
│  │     bone_motions["bip01_spine"] → [walk, run, idle]    │  │
│  │     bone_motions["bip01_head"]  → [walk, run, idle]    │  │
│  └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
           ▲              ▲              ▲
           │              │              │
    ┌──────┴───┐   ┌──────┴───┐   ┌──────┴───┐
    │Instance 1│   │Instance 2│   │Instance 3│
    │          │   │          │   │          │
    │SharedOzzMotions (RAII handle)         │
    │ bone_motions[bone_id]->at(motion_id)  │
    └──────────┘                   └──────────┘
```

### Core Data Structures

**Handle-Based API:**
```cpp
struct MotionLibraryHandle {
    u64 id{0};  // Unique ID (could be GUID in future)

    bool IsValid() const { return id != 0; }
    void Invalidate() { id = 0; }

    bool operator==(const MotionLibraryHandle& other) const { return id == other.id; }
};
```

**Skeleton Fingerprinting:**
```cpp
struct SkeletonFingerprint {
    u32 hash{0};        // CRC32 of joint names
    u16 jointCount{0};  // Number of joints
    u16 version{0};     // Format version

    static SkeletonFingerprint Compute(const ozz::animation::Skeleton& skeleton) {
        SkeletonFingerprint fp;
        fp.jointCount = static_cast<u16>(skeleton.num_joints());
        fp.version = 1;

        // Compute CRC32 of joint names
        const char** names = skeleton.joint_names();
        u32 hash = 0;
        for (int i = 0; i < skeleton.num_joints(); ++i) {
            hash = crc32(names[i], xr_strlen(names[i]), hash);
        }
        fp.hash = hash;
        return fp;
    }

    bool Matches(const SkeletonFingerprint& other) const {
        return (hash == other.hash) && (jointCount == other.jointCount);
    }
};
```

**Shared Motion Data (Bone-Major Layout):**
```cpp
struct XRANIMATION_API OzzMotionsValue {
    // Motion metadata (lightweight)
    struct MotionRecord {
        xr_string name;
        std::shared_ptr<ozz::animation::Animation> animation;
        CMotionDef definition;      // Marks, speed, power, etc (immutable)
        MotionID id;
        u32 frameCount{0};
    };

    xr_vector<MotionRecord> records;          // Metadata only
    xr_unordered_map<xr_string, u16> lookup;  // Name → index

    // Heavy motion data (BONE-MAJOR LAYOUT - CRITICAL!)
    BoneMotionMap bone_motions;  // bone_name → [motion0, motion1, ...]
    //                              ^^^ Organized by BONE first, not motion!

    // Handle & fingerprinting
    MotionLibraryHandle handle;
    SkeletonFingerprint skelFingerprint;

    // Reference counting (atomic for thread safety)
    std::atomic<u32> refCount{0};

    // Load state tracking
    std::atomic<MotionLoadState> loadState{MotionLoadState::Unloaded};

    // Metadata
    shared_str sourceFile;
    u64 lastAccessTime{0};        // For LRU eviction
    u32 totalMemoryBytes{0};      // Cached memory usage
};
```

**Global Container:**
```cpp
class XRANIMATION_API OzzMotionsContainer {
public:
    struct Config {
        u64 maxMemoryBytes = 512 * 1024 * 1024;  // 512MB default
        bool enableLRUEviction = true;
        bool enablePrefetching = false;
        bool enforceSkeletonCompatibility = true;
    };

    // Primary API
    MotionLibraryHandle Dock(shared_str key, const ozz::animation::Skeleton& skeleton);
    OzzMotionsValue* Resolve(MotionLibraryHandle handle);
    void Release(MotionLibraryHandle handle);

    // Batch operations
    xr_vector<MotionLibraryHandle> LoadBatch(const xr_vector<LoadRequest>& requests);

    // Memory management
    void EvictLRU(u64 target_memory_bytes);
    bool EvictMotion(MotionLibraryHandle handle);
    u64 GetMemoryUsage() const { return stats.currentMemoryBytes.load(); }

private:
    using HandleToValueMap = xr_unordered_map<u64, OzzMotionsValue*>;
    using KeyToHandleMap = xr_unordered_map<shared_str, u64>;

    HandleToValueMap values;
    KeyToHandleMap keyToHandle;
    Lock containerLock;

    std::atomic<u64> nextHandleID{1};
};
```

**RAII Wrapper:**
```cpp
class XRANIMATION_API SharedOzzMotions {
    MotionLibraryHandle handle_;

    void Destroy() {
        if (handle_.IsValid() && g_pOzzMotionsContainer) {
            g_pOzzMotionsContainer->Release(handle_);
            handle_.Invalidate();
        }
    }

public:
    SharedOzzMotions() = default;
    ~SharedOzzMotions() { Destroy(); }

    // Move semantics
    SharedOzzMotions(SharedOzzMotions&& other) noexcept : handle_(other.handle_) {
        other.handle_.Invalidate();
    }

    // Copy semantics (increments ref count)
    SharedOzzMotions(const SharedOzzMotions& other);

    // Creation from global container
    bool Create(shared_str key, const ozz::animation::Skeleton& skeleton) {
        Destroy();
        handle_ = g_pOzzMotionsContainer->Dock(key, skeleton);
        return handle_.IsValid();
    }

    // Accessors (bone-major layout)
    MotionVec* GetBoneMotions(const shared_str& bone_name) const;
    OzzMotionsValue::MotionRecord* FindMotion(const xr_string& name) const;
};
```

### Implementation in OzzKinematicsAnimated

**Updated Structure:**
```cpp
class OzzKinematicsAnimated : public OzzKinematics {
private:
    // Motion library management (CORRECTED - matches legacy layout)
    struct SMotionsSlot {
        SharedOzzMotions motions;         // Shared handle (not raw pointer!)
        BoneMotionsVec bone_motions;      // xr_vector<MotionVec*>
        //             ^^^ Each entry is MotionVec* (pointer to vector of CMotions)
    };
    using MotionsSlotVec = xr_vector<SMotionsSlot>;
    MotionsSlotVec m_Motions;

    xr_vector<xr_string> motionReferences;  // File paths to load
};
```

**Motion Loading:**
```cpp
void OzzKinematicsAnimated::EnsureMotionLibraryLoaded() {
    m_Motions.clear();

    // Compute skeleton fingerprint once
    SkeletonFingerprint skelFP = SkeletonFingerprint::Compute(core.Skeleton());

    for (const auto& reference : motionReferences) {
        // Create motion slot
        m_Motions.push_back(SMotionsSlot());
        SMotionsSlot& slot = m_Motions.back();

        // Dock to global container (deduplicates automatically!)
        if (!slot.motions.Create(shared_str(reference.c_str()), core.Skeleton())) {
            Msg("[OzzKinematicsAnimated] Failed to load motion ref '%s'", reference.c_str());
            m_Motions.pop_back();
            continue;
        }

        // Build per-bone motion cache
        BuildBoneMotionCache(slot);
    }
}

void OzzKinematicsAnimated::BuildBoneMotionCache(SMotionsSlot& slot) {
    const u32 bone_count = core.Skeleton().num_joints();
    slot.bone_motions.resize(bone_count);

    // Get MotionVec* for each bone from shared data
    for (u32 bone_idx = 0; bone_idx < bone_count; ++bone_idx) {
        const char* bone_name = core.Skeleton().joint_names()[bone_idx];
        slot.bone_motions[bone_idx] = slot.motions.GetBoneMotions(bone_name);
        //                             ^^^ Returns MotionVec* from shared container
    }
}
```

**Motion Access:**
```cpp
CMotion* OzzKinematicsAnimated::LL_GetMotion(MotionID id, u16 bone_id) {
    // Validate
    if (!id.valid() || id.slot >= m_Motions.size())
        return nullptr;
    if (!core.IsInitialized() || bone_id == BI_NONE)
        return nullptr;
    if (bone_id >= core.Skeleton().num_joints())
        return nullptr;

    // Get bone's motion vector from cache
    MotionVec* bone_motions = m_Motions[id.slot].bone_motions[bone_id];
    //           ^^^ This is a POINTER to a vector of CMotions for this bone

    if (!bone_motions || id.idx >= bone_motions->size())
        return nullptr;

    return &bone_motions->at(id.idx);
    //      ^^^ Returns CMotion* for this specific bone + motion combo
}
```

### Integration with CModelPool

```cpp
CModelPool::CModelPool() {
    bLogging = TRUE;
    bForceDiscard = FALSE;
    bAllowChildrenDuplicate = TRUE;
    g_pMotionsContainer = xr_new<motions_container>();
    g_pOzzMotionsContainer = xr_new<OzzMotionsContainer>();  // NEW!
}

CModelPool::~CModelPool() {
    Destroy();
    xr_delete(g_pMotionsContainer);
    xr_delete(g_pOzzMotionsContainer);  // NEW!
}

void CModelPool::Destroy() {
    // ... existing code ...

    // Cleanup ozz motions
    if (g_pOzzMotionsContainer)
        g_pOzzMotionsContainer->Clean(false);
}
```

### Performance Benefits

| Metric | Before (Per-Instance) | After (Shared) | Improvement |
|--------|----------------------|----------------|-------------|
| **Memory (10 NPCs)** | 500MB | 50MB | **10x reduction** |
| **Load Time (First)** | 100ms | 100ms | Same |
| **Load Time (Subsequent)** | 100ms | <1ms | **100x faster** |
| **Cache Hit Rate** | 0% | 90%+ | Massive improvement |

### Implementation Status

🚧 **Currently Deferred**

**Decision:** Defer to ECS migration

**Rationale:**
- Current per-instance motion libraries work acceptably
- Memory usage tolerable for current character counts
- ECS will change memory layout requirements
- Avoid premature optimization

**When to Implement:**
- During ECS migration (Phase 8)
- When memory profiling shows duplication issues
- When supporting 500+ unique character types

**Design Readiness:** ✅ Complete specification available for implementation

---

## Implementation Patterns {#implementation-patterns}

### Pattern 1: Bone-Major Data Layout

**Legacy Motivation:**
```cpp
// Why bone-major? Because gameplay queries are PER-BONE
CMotion* motion = character->LL_GetMotion(walk_motion_id, spine_bone_id);
//                                         ^^^^^^^^^^^^^^  ^^^^^^^^^^^
//                                         Motion ID       Bone ID
```

**Correct Layout:**
```cpp
// BONE-MAJOR: bone_motions[bone_name][motion_id]
xr_map<shared_str, MotionVec> bone_motions;
bone_motions["bip01_spine"][walk_id];  // CMotion for spine during walk
bone_motions["bip01_head"][walk_id];   // CMotion for head during walk
```

**Wrong Layout (Don't Use):**
```cpp
// MOTION-MAJOR: motions[motion_id].bone_data[bone_id]  ← WRONG!
xr_vector<MotionWithAllBones> motions;
motions[walk_id].bone_data[spine_bone_id];  // Inefficient lookup
```

### Pattern 2: Handle-Based Resources

**Why Handles?**
- Type-safe (can't dereference invalid handle)
- Prevents dangling pointers
- Compatible with async loading
- Enables hot-reloading
- Allows resource versioning

**Implementation:**
```cpp
// DON'T: Store raw pointers
motions_value* motions;  // ← Dangerous! Can become invalid

// DO: Store handles
MotionLibraryHandle handle;  // ← Safe, validated on access
OzzMotionsValue* data = container->Resolve(handle);
```

### Pattern 3: Skeleton Fingerprinting

**Purpose:** Prevent loading incompatible motions onto wrong skeleton

**Example Failure Without Fingerprinting:**
```cpp
// Mutant skeleton (42 bones)
OzzKinematicsAnimated mutant;
mutant.InitializeFromOzz("mutant.ozz");

// Try to load stalker motions (47 bones) ← MISMATCH!
mutant.LoadMotionLibrary("stalker_animation.ozz");  // ← CRASH or corruption!
```

**Solution:**
```cpp
MotionLibraryHandle handle = container->Dock("stalker_animation.ozz", mutant.Skeleton());
if (!handle.IsValid()) {
    Msg("[ERROR] Skeleton mismatch!");
    Msg("  Expected: hash=0x%08X, joints=%u", mutant_fp.hash, mutant_fp.jointCount);
    Msg("  Actual:   hash=0x%08X, joints=%u", stalker_fp.hash, stalker_fp.jointCount);
}
```

### Pattern 4: Atomic Reference Counting

**Why Atomic?**
- Thread-safe without locks
- Faster than mutex-based counting
- Compatible with parallel systems

**Implementation:**
```cpp
// Thread-safe increment/decrement
std::atomic<u32> refCount{0};

// Acquire reference
value->refCount.fetch_add(1, std::memory_order_relaxed);

// Release reference
const u32 prevRef = value->refCount.fetch_sub(1, std::memory_order_release);
if (prevRef == 1) {
    // Was last reference - safe to delete
    xr_delete(value);
}
```

---

## Performance Optimizations {#performance-optimizations}

### Optimization 1: Memory Pooling

**Problem:** Frequent small allocations (bone transforms, motion data)

**Solution:** Pre-allocate pools
```cpp
class BoneTransformPool {
    static constexpr size_t POOL_SIZE = 1024;
    ozz::vector<ozz::math::Float4x4> pool_[POOL_SIZE];
    size_t next_free_{0};

public:
    ozz::span<ozz::math::Float4x4> Allocate(size_t count) {
        if (next_free_ + count > POOL_SIZE)
            return {};  // Pool exhausted

        size_t start = next_free_;
        next_free_ += count;
        return ozz::make_span(&pool_[start], count);
    }

    void Reset() { next_free_ = 0; }  // Reset per-frame
};
```

### Optimization 2: LRU Eviction

**Problem:** Memory pressure from too many loaded motions

**Solution:** Automatic eviction of least-recently-used motions
```cpp
void OzzMotionsContainer::EvictLRU(u64 target_memory_bytes) {
    if (GetMemoryUsage() <= target_memory_bytes)
        return;

    // Gather eviction candidates (ref count == 0)
    xr_vector<EvictionCandidate> candidates;
    for (auto& [handle_id, value] : values) {
        if (value->refCount.load() == 0) {
            candidates.push_back({
                .handle = {handle_id},
                .lastAccessTime = value->lastAccessTime,
                .memoryBytes = value->totalMemoryBytes
            });
        }
    }

    // Sort by last access time (oldest first)
    std::sort(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) {
            return a.lastAccessTime < b.lastAccessTime;
        });

    // Evict until under target
    u64 freed = 0;
    for (const auto& candidate : candidates) {
        if (GetMemoryUsage() - freed <= target_memory_bytes)
            break;

        if (EvictMotion(candidate.handle)) {
            freed += candidate.memoryBytes;
            stats.totalEvictions++;
        }
    }
}
```

### Optimization 3: Batch Loading

**Problem:** Loading many motions sequentially is slow

**Solution:** Parallel batch loading
```cpp
xr_vector<MotionLibraryHandle> LoadBatch(const xr_vector<LoadRequest>& requests) {
    xr_vector<MotionLibraryHandle> handles(requests.size());

    // Submit async loads
    xr_parallel_for(TaskRange<size_t>(0, requests.size()),
        [&](const TaskRange<size_t>& range) {
            for (size_t i = range.begin(); i != range.end(); ++i) {
                handles[i] = DockInternal(requests[i]);
            }
        });

    return handles;
}
```

### Optimization 4: Cache-Friendly Memory Layout

**Problem:** Poor cache locality with scattered data

**Solution:** Structure-of-Arrays (SoA) for hot data
```cpp
// BAD: Array-of-Structures (AoS)
struct BoneData_AoS {
    ozz::math::Float4x4 transform;
    ozz::math::Float3 position;
    ozz::math::Quaternion rotation;
    // ... more data
};
xr_vector<BoneData_AoS> bones;  // ← Poor cache locality

// GOOD: Structure-of-Arrays (SoA)
struct BoneData_SoA {
    ozz::vector<ozz::math::Float4x4> transforms;    // All transforms together
    ozz::vector<ozz::math::Float3> positions;       // All positions together
    ozz::vector<ozz::math::Quaternion> rotations;   // All rotations together
};
BoneData_SoA bones;  // ← Excellent cache locality
```

### Optimization 5: SIMD Utilization

**Problem:** Scalar operations on bone transforms

**Solution:** Use ozz's SIMD types
```cpp
// BAD: Scalar operations
for (int i = 0; i < num_bones; ++i) {
    ozz::math::Float4x4 local = locals[i];
    ozz::math::Float4x4 parent = models[parent_idx[i]];
    models[i] = parent * local;  // Scalar matrix multiply
}

// GOOD: SIMD operations (4 bones at a time)
for (int i = 0; i < num_soa_bones; ++i) {
    ozz::math::SoaTransform local = soa_locals[i];  // 4 transforms in parallel
    // ... SIMD operations on 4 bones simultaneously
}
```

---

## Testing Strategy

### Unit Tests

**Extended Bone Metadata:**
```cpp
TEST(ExtendedBoneMetadata, ComputesRestLength) {
    BoneRecord parent, child;
    parent.local_transform = Fmatrix::identity();
    child.local_transform.c = Fvector{0, 0.5f, 0};  // 0.5m from parent

    float length = ComputeRestLength(child, parent);
    EXPECT_NEAR(length, 0.5f, 0.001f);
}

TEST(ExtendedBoneMetadata, ClassifiesGroundContact) {
    BoneRecord foot;
    foot.name = "bip01_l_foot";
    foot.dominant_axis = Fvector{0, -1, 0};  // Points down
    foot.global_transform.c = Fvector{0, 0.05f, 0};  // Near ground

    xr_vector<BoneRecord> skeleton = {/* ... */};
    EXPECT_TRUE(IsGroundContactCandidate(foot, skeleton));
}
```

**Shared Motion Container:**
```cpp
TEST(SharedMotions, Deduplicates) {
    OzzMotionsContainer container;

    // Load motion 3 times
    auto handle1 = container.Dock("walk.ozz", skeleton);
    auto handle2 = container.Dock("walk.ozz", skeleton);
    auto handle3 = container.Dock("walk.ozz", skeleton);

    // All should return same handle
    EXPECT_EQ(handle1, handle2);
    EXPECT_EQ(handle2, handle3);

    // Ref count should be 3
    OzzMotionsValue* value = container.Resolve(handle1);
    EXPECT_EQ(value->refCount.load(), 3);
}

TEST(SharedMotions, EvictsLRU) {
    OzzMotionsContainer container;
    container.SetMemoryLimit(100 * 1024 * 1024);  // 100MB limit

    // Load many motions (exceed limit)
    xr_vector<MotionLibraryHandle> handles;
    for (int i = 0; i < 20; ++i) {
        auto handle = container.Dock(fmt::format("motion_{}.ozz", i), skeleton);
        handles.push_back(handle);
        container.Release(handle);  // Release immediately (ref count = 0)
    }

    // Should have evicted some
    EXPECT_LT(container.GetMemoryUsage(), 100 * 1024 * 1024);

    // Oldest motions should be evicted
    EXPECT_FALSE(container.Resolve(handles[0]));  // Evicted
    EXPECT_TRUE(container.Resolve(handles[19]));  // Recent, still loaded
}
```

### Integration Tests

```cpp
TEST(Integration, SharedMotionsWithMultipleCharacters) {
    // Create 10 NPCs with same motion reference
    xr_vector<OzzKinematicsAnimated*> npcs;
    for (int i = 0; i < 10; ++i) {
        auto npc = xr_new<OzzKinematicsAnimated>();
        npc->InitializeFromOzz("stalker.ozz");
        npc->LoadMotionLibrary("stalker_animation.ozz");
        npcs.push_back(npc);
    }

    // Check memory usage (should be ~1x, not 10x)
    u64 memory = g_pOzzMotionsContainer->GetMemoryUsage();
    EXPECT_LT(memory, 100 * 1024 * 1024);  // Less than 100MB for 10 instances

    // Check cache hit rate
    auto stats = g_pOzzMotionsContainer->GetStatistics();
    EXPECT_GT(stats.GetHitRate(), 0.9f);  // 90%+ cache hits

    // Cleanup
    for (auto npc : npcs)
        xr_delete(npc);
}
```

---

## Migration Checklist

### Extended Bone Metadata
- [x] Design `BoneRecord` structure with extended fields
- [x] Implement computation functions (rest length, inertia, etc.)
- [x] Update `.ozzx` bundle format to version 2
- [x] Implement serialization/deserialization
- [x] Update converter to emit extended metadata
- [x] Update runtime to hydrate `CBoneData`
- [x] Add parity tests for metadata loading
- [ ] Validate in-game with physics systems
- [ ] Document metadata usage patterns

### Shared Motion Container
- [ ] Implement `OzzMotionsContainer` class
- [ ] Implement handle-based API
- [ ] Implement skeleton fingerprinting
- [ ] Integrate with `CModelPool` lifecycle
- [ ] Update `OzzKinematicsAnimated` to use shared container
- [ ] Implement LRU eviction
- [ ] Add batch loading support
- [ ] Write unit tests for deduplication
- [ ] Write integration tests with multiple characters
- [ ] Profile memory usage before/after
- [ ] **DEFER:** Until ECS migration phase

---

## Performance Targets

### Extended Bone Metadata
- **Bundle Size:** +5-10KB per skeleton (negligible)
- **Load Time:** +1-2ms per skeleton (acceptable)
- **Runtime Cost:** Zero (pre-computed, cached)

### Shared Motion Container
- **Memory Reduction:** 10-50x for scenes with many NPCs
- **Load Time (First):** Same as current (~100ms)
- **Load Time (Subsequent):** <1ms (instant from cache)
- **Cache Hit Rate:** 90%+ in typical gameplay

---

**Document Version:** 1.0
**Maintainer:** OpenXRay Animation Team
**Last Review:** 2025-10-12
