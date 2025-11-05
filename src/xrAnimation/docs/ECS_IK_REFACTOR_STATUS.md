# XRay Engine - Advanced IK & Ragdoll Implementation Plan

## 📋 Table of Contents
1. [Current Status](#current-status)
2. [Euphoria System Analysis](#euphoria-system-analysis)
3. [Implementation Roadmap](#implementation-roadmap)
4. [Phase 1: Enhanced Foot IK](#phase-1-enhanced-foot-ik)
5. [Phase 2: Spine/Torso IK](#phase-2-spinetorso-ik)
6. [Phase 3: Head/Body Look-At](#phase-3-headbody-look-at)
7. [Phase 4: Hand/Arm IK](#phase-4-handarm-ik)
8. [Phase 5: Ragdoll Integration](#phase-5-ragdoll-integration)
9. [Phase 6: Live/Active IK](#phase-6-liveactive-ik)
10. [Technical Architecture](#technical-architecture)

---

## Current Status

### ✅ Completed - Basic ECS IK Foundation + Interactive Testing UI

**AnimationECS_IK Module:**
- `LimbIKChain` component (data-only, two-bone IK)
- `IKConfiguration` component (container for all chains)
- `IKInitializationSystem` - Auto-detects and initializes IK chains with left/right detection
- `IKSolverSystem` - Solves basic two-bone IK with per-limb rebuild strategy
- `IKDebugSystem` - Placeholder for debug visualization
- Build system integration with xrAnimation library
- **ozz_animation_viewer full IK panel integration (October 2025)**

**Current Capabilities:**
- Basic two-bone IK for legs and arms
- Pole vector support (fixes first-person arm bending)
- **Multi-limb simultaneous IK (all 4 limbs independently controlled)**
- **Per-limb model-space rebuild (handles skeleton hierarchy dependencies)**
- **Left/right limb auto-detection (prevents label swapping)**
- **ImGui IK panel with real-time parameter adjustment**
- ECS-based architecture ready for expansion
- Single source of truth between viewer and game

**ozz_animation_viewer IK Panel (NEW - October 2025):**
- Individual limb enable/disable controls (Left Leg, Right Leg, Left Arm, Right Arm)
- Enable All / Disable All buttons for legs and arms
- Real-time target offset adjustment (X, Y, Z sliders)
- IK parameter tuning (weight, soften, twist angle)
- Status feedback (target reached / out of reach)
- Panel visibility toggle via View menu

### 🔧 Technical Implementation Notes

**Per-Limb Rebuild Strategy (October 2025):**
The current IK solver uses a per-limb rebuild approach (4 LocalToModelJob calls per frame) to handle skeleton hierarchy dependencies:

**Why Not Single Rebuild?**
- Skeleton hierarchy: Right leg (joints 3-5) shares pelvis parent with left leg (joints 7-9)
- If we solve all limbs then rebuild once, later limbs see ANIMATED parent transforms, not IK-modified ones
- Example: Right leg IK moves pelvis → Left leg's IK solution is now wrong (assumed old pelvis)

**Per-Limb Solution:**
1. Left leg solves → Rebuild from joint 7 → models[] updated
2. Right leg solves (sees IK-modified pelvis) → Rebuild from joint 3 → models[] updated
3. Left arm solves (sees IK-modified spine) → Rebuild from joint 34 → models[] updated
4. Right arm solves (sees all IK) → Rebuild from joint 21 → models[] updated

**Cost:** 4 LocalToModelJob calls per entity per frame (~0.02ms each = ~0.08ms total per character)
**Benefit:** Correct multi-limb IK with proper hierarchy propagation

**Future Optimization:** Could reduce to 2 rebuilds (legs group, arms group) if limbs within groups are truly independent (requires skeleton structure analysis).

### 🚧 Limitations of Current Implementation

**What's Missing:**
1. **Foot IK:** No ground probing, pelvis adjustment, foot locking, stair detection
2. **Spine IK:** No torso rotation for aiming/looking
3. **Head IK:** No look-at system
4. **Hand IK:** No target tracking, no interaction system
5. **Ragdoll:** No ragdoll-to-animation transitions, no getup system
6. **Active IK:** No IK during physics simulation
7. **Environmental Awareness:** No dynamic reactions to terrain, moving platforms

---

## Euphoria System Analysis

### Key Learnings from RAGE/Euphoria Documentation

**Effector System (Low-Level Joint Control):**
- **1DOF Effectors:** Hinge joints (elbows, knees) with angle control
- **3DOF Effectors:** Ball joints (shoulders, hips, spine) with twist-swing representation
- **Muscle Parameters:** Strength, damping, stiffness, gravity opposition
- **Runtime Limits:** Dynamic joint limit modification

**Limb System (Mid-Level Grouping):**
- Hierarchical grouping of effectors (arm = clavicle + shoulder + elbow + wrist)
- Priority-based message queue for behavior coordination
- IK solving at limb level (two-bone IK, wrist orientation)
- Memory pooling for per-frame allocations

**Behavior System (High-Level Procedural Animation):**
- 47+ distinct behaviors (balance, shot, grab, fall, etc.)
- Message-based parameter passing (100+ message types)
- State machines for complex behavior sequences
- Priority and sub-priority for layered control

**IK Systems from RAGE Engine:**
- **Leg IK:** Async ground probing (heel + ball of foot), pelvis adjustment, foot locking, stair detection
- **Torso IK:** Distributed spine rotation (4 bones), yaw/pitch limits, weapon aiming
- **Head IK:** Multi-priority look-at, torso integration, FOV awareness
- **Arm IK:** Target tracking (absolute/relative), distance-based blending, recoil integration

**Ragdoll Transitions:**
- 70+ getup animations with pose matching
- Drive-to-getup (NM actively moves toward animation pose)
- Breakout phases (early exit based on player input)
- Blend-from-ragdoll with smooth interpolation

---

## Implementation Roadmap

### Philosophy: Incremental, Tested, Integrated

Each phase builds on the previous, with working features at every step.

```
Current Foundation (Two-Bone IK)
    ↓
Phase 1: Enhanced Foot IK (Ground Contact + Pelvis)
    ↓
Phase 2: Spine/Torso IK (Aiming System)
    ↓
Phase 3: Head/Body Look-At (Target Tracking)
    ↓
Phase 4: Hand/Arm IK (Interaction System)
    ↓
Phase 5: Ragdoll Integration (Transitions)
    ↓
Phase 6: Live/Active IK (Physics Integration)
```

### Timeline Estimate
- **Phase 1:** 2-3 weeks
- **Phase 2:** 2 weeks
- **Phase 3:** 1-2 weeks
- **Phase 4:** 2 weeks
- **Phase 5:** 3-4 weeks
- **Phase 6:** 2-3 weeks
- **Total:** ~12-16 weeks for complete system

---

## Phase 1: Enhanced Foot IK

**Goal:** Production-quality foot IK with ground probing, pelvis adjustment, and environmental awareness

### 1.1 Ground Probing System

**Component Extensions:**
```cpp
// Add to AnimationECS_IK.h

struct FootProbe {
    enum FootPart { HEEL, BALL, NUM_PARTS };

    // World-space probe positions
    ozz::math::Float3 probe_positions[NUM_PARTS];
    ozz::math::Float3 probe_velocities[NUM_PARTS];  // For prediction

    // Raycast results
    ozz::math::Float3 ground_positions[NUM_PARTS];
    ozz::math::Float3 ground_normals[NUM_PARTS];
    bool probe_valid[NUM_PARTS]{false, false};

    // Interpolation state
    ozz::math::Float3 ground_pos_smoothed;
    ozz::math::Float3 ground_normal_smoothed;
};

struct LegIKEnhanced : LimbIKChain {
    FootProbe foot_probe;

    // Foot locking (prevents sliding when planted)
    bool foot_locked{false};
    float locked_height{0.f};
    ozz::math::Float3 locked_position;
    float lock_blend{0.f};

    // Interpolation parameters
    struct InterpolationSettings {
        float position_rate{10.f};
        float normal_rate{15.f};
        float stair_position_rate{5.f};
        float stair_normal_rate{8.f};
        bool use_acceleration{true};
    } interp;

    // Environmental state
    bool on_stairs{false};
    bool on_slope{false};
    bool on_moving_platform{false};
    float terrain_slope_angle{0.f};

    // Tuning
    float stair_height_tolerance{0.15f};  // 15cm step detection
    float slope_angle_threshold{15.f};     // 15 degrees
    float probe_hysteresis{0.02f};         // 2cm to prevent jitter
};
```

**System Implementation:**
```cpp
class GroundProbeSystem {
public:
    // Synchronous probing (Phase 1.1)
    static void UpdateFootProbes(
        entt::registry& registry,
        float delta_time);

    // Async probing (Phase 1.2 - optional optimization)
    static void SubmitAsyncProbes(entt::registry& registry);
    static void ProcessAsyncResults(entt::registry& registry);

private:
    static void CalculateProbePositions(
        const LegIKEnhanced& leg,
        const ozz::math::Float4x4& ankle_transform);

    static void PerformRaycast(
        FootProbe& probe,
        int foot_part_index,
        const PhysicsWorld& physics);

    static void DetectStairs(LegIKEnhanced& leg);
    static void DetectSlope(LegIKEnhanced& leg);

    static void InterpolateGroundContact(
        FootProbe& probe,
        const InterpolationSettings& settings,
        float delta_time);
};
```

**Integration:**
- Raycast using existing XRay physics system
- Submit from `IKSolverSystem::Update()` before IK solving
- Support both synchronous and async (job system) raycasts

### 1.2 Pelvis Adjustment System

**Component Extensions:**
```cpp
struct PelvisIK {
    bool enabled{true};

    // Height adjustment
    float height_offset{0.f};           // Current offset (applied)
    float target_height_offset{0.f};    // Target offset (goal)
    float height_velocity{0.f};         // For smooth damping

    // Interpolation
    float interp_rate{8.f};
    float interp_rate_moving{12.f};
    float interp_rate_stairs{6.f};
    float interp_rate_platform{15.f};

    // Limits (context-dependent)
    struct Limits {
        float max_raise_standing{0.15f};
        float max_lower_standing{0.20f};
        float max_raise_moving{0.12f};
        float max_lower_moving{0.18f};
        float max_raise_stairs{0.25f};
        float max_lower_stairs{0.10f};
    } limits;

    // Supporting foot mode
    enum SupportMode {
        LOWEST_FOOT,
        HIGHEST_FOOT,
        BACK_FOOT,
        FRONT_FOOT,
        MOST_RECENT_LOCKED
    } support_mode{LOWEST_FOOT};

    // State
    bool character_moving{false};
    bool on_stairs{false};
    bool on_moving_platform{false};
};

// Add to IKConfiguration
struct IKConfiguration {
    // ... existing members ...
    PelvisIK pelvis;
    bool pelvis_ik_available{false};
};
```

**System Implementation:**
```cpp
class PelvisAdjustmentSystem {
public:
    static void UpdatePelvis(
        entt::registry& registry,
        float delta_time);

private:
    static float CalculateTargetOffset(
        const PelvisIK& pelvis,
        const LegIKEnhanced& left_leg,
        const LegIKEnhanced& right_leg);

    static void ApplyLimits(
        PelvisIK& pelvis,
        float& target_offset);

    static void SmoothToTarget(
        PelvisIK& pelvis,
        float delta_time);

    static void ApplyToPelvisBone(
        ozz::span<ozz::math::SoaTransform> locals,
        int pelvis_bone_idx,
        float height_offset);
};
```

### 1.3 Foot Locking System

**Purpose:** Prevent foot sliding when planted on ground

**Implementation:**
```cpp
class FootLockingSystem {
public:
    static void UpdateFootLocks(
        entt::registry& registry,
        ozz::span<const ozz::math::Float4x4> current_models,
        float delta_time);

private:
    static bool IsFootPlanted(
        const LegIKEnhanced& leg,
        const ozz::animation::Animation& anim,
        float playback_time);

    static void LockFoot(LegIKEnhanced& leg, ozz::math::Float3 world_pos);
    static void UnlockFoot(LegIKEnhanced& leg);

    static void BlendToLockedPosition(
        LegIKEnhanced& leg,
        ozz::math::Float3& ik_target,
        float delta_time);
};
```

**Plant Detection:**
- Check animation velocity at foot bone
- Check foot height relative to ground
- Use animation tags if available (preferred)

### Phase 1 Deliverables

**Features:**
- ✅ Dual-probe per foot (heel + ball)
- ✅ Ground raycast integration with XRay physics
- ✅ Pelvis height adjustment based on supporting foot
- ✅ Foot locking to prevent sliding
- ✅ Stair detection and special handling
- ✅ Slope detection and adaptation
- ✅ Smooth interpolation with context-aware rates

**Testing:**
- Walk on flat ground (no floating/sinking)
- Walk up/down stairs (smooth transitions)
- Walk on slopes (proper foot orientation)
- Stand on moving platforms
- Transition between idle/walk/run (no pops)

---

## Phase 2: Spine/Torso IK

**Goal:** Distributed spine rotation for weapon aiming and torso tracking

### 2.1 Spine Chain Component

```cpp
struct SpineIKChain {
    // Spine bone indices (typically 3-5 bones)
    static constexpr int MAX_SPINE_BONES = 5;
    s16 spine_bones[MAX_SPINE_BONES]{-1, -1, -1, -1, -1};
    int num_spine_bones{0};

    // Weight distribution (must sum to 1.0)
    // Example: [0.15, 0.20, 0.30, 0.25, 0.10] for 5-bone spine
    float bone_weights[MAX_SPINE_BONES]{0.2f, 0.2f, 0.2f, 0.2f, 0.2f};

    // Target (world space)
    ozz::math::Float3 aim_target_position;
    ozz::math::Float3 aim_direction;
    bool use_position{true};  // false = use direction

    // Rotation limits (radians, in spine-local space)
    float min_yaw{-1.047f};    // -60 degrees
    float max_yaw{1.047f};     // +60 degrees
    float min_pitch{-0.524f};  // -30 degrees
    float max_pitch{0.524f};   // +30 degrees

    // Current state (for smoothing)
    float current_yaw{0.f};
    float current_pitch{0.f};
    float yaw_velocity{0.f};
    float pitch_velocity{0.f};

    // Blending
    float blend{0.f};
    float blend_in_rate{2.f};
    float blend_out_rate{3.f};

    // Status
    bool enabled{false};
    bool target_reachable{true};
    bool yaw_reached{false};
    bool pitch_reached{false};

    // Tuning
    float max_rotation_speed{3.0f};  // rad/s
    float smooth_time{0.15f};
};

// Add to IKConfiguration
struct IKConfiguration {
    // ... existing ...
    SpineIKChain spine;
    bool spine_ik_available{false};
};
```

### 2.2 Spine IK Solver

```cpp
class SpineIKSystem {
public:
    // Auto-detect spine bones
    static void InitializeSpine(
        SpineIKChain& spine,
        const ozz::animation::Skeleton& skeleton);

    // Set target (high-level API)
    static void SetAimTarget(
        SpineIKChain& spine,
        ozz::math::Float3 target_position);

    static void SetAimDirection(
        SpineIKChain& spine,
        ozz::math::Float3 direction);

    static void Enable(SpineIKChain& spine, bool enable);

    // Solve (called during IK update)
    static void SolveSpine(
        SpineIKChain& spine,
        ozz::span<const ozz::math::Float4x4> models,
        ozz::span<ozz::math::SoaTransform> locals,
        float delta_time);

private:
    static void CalculateTargetAngles(
        const SpineIKChain& spine,
        const ozz::math::Float4x4& spine_root_transform,
        float& out_yaw,
        float& out_pitch);

    static void ClampAndSmooth(
        SpineIKChain& spine,
        float target_yaw,
        float target_pitch,
        float delta_time);

    static void DistributeRotation(
        const SpineIKChain& spine,
        float yaw,
        float pitch,
        ozz::span<ozz::math::SoaTransform> locals);

    static ozz::math::Quaternion CreateRotationForBone(
        int bone_index,
        float yaw,
        float pitch,
        float weight,
        float blend);
};
```

### 2.3 Weapon Aiming Integration

```cpp
// High-level API for game code
class AimIKController {
public:
    void PointWeaponAt(
        entt::entity character,
        ozz::math::Float3 target_position,
        float blend_speed = 1.0f);

    void StopAiming(
        entt::entity character,
        float blend_speed = 1.0f);

    bool IsAiming(entt::entity character) const;
    bool IsOnTarget(entt::entity character) const;

    void SetLimits(
        entt::entity character,
        float yaw_range,
        float pitch_range);
};
```

### Phase 2 Deliverables

**Features:**
- ✅ Auto-detection of spine bone chain
- ✅ Distributed rotation across 3-5 spine bones
- ✅ Yaw/pitch angle limits
- ✅ Smooth interpolation to targets
- ✅ Blend in/out control
- ✅ Status feedback (reached, reachable, etc.)

**Testing:**
- Aim weapon at moving target
- Smooth transitions when target moves
- Proper limit clamping (can't rotate 180°)
- Blend in/out when entering/exiting aim mode
- Work with existing animation (additive on top)

---

## Phase 3: Head/Body Look-At

**Goal:** Multi-priority look-at system with head/neck/torso integration

### 3.1 Look-At Component

```cpp
struct LookAtTarget {
    // Target
    ozz::math::Float3 position;
    entt::entity target_entity{entt::null};  // Track entity (optional)
    s16 target_bone_idx{-1};                 // Track bone (optional)
    ozz::math::Float3 offset{0.f, 0.f, 0.f}; // Offset from target

    // Priority system
    enum Priority {
        VERY_LOW = 0,
        LOW = 1,
        MEDIUM = 2,
        HIGH = 3,
        VERY_HIGH = 4
    } priority{LOW};

    // Timing
    float duration{-1.f};        // -1 = infinite
    float elapsed_time{0.f};
    float blend_in_time{0.5f};
    float blend_out_time{0.5f};

    // Flags
    bool use_torso{false};       // Also rotate torso
    bool eyes_only{false};       // Only eyes, no head turn
    bool track_outside_fov{false}; // Track even if not visible

    // State
    bool active{false};
    float current_blend{0.f};
    uint32_t unique_id{0};       // For identification
};

struct HeadIKChain {
    // Bone indices
    s16 head_bone{-1};
    s16 neck_lower_bone{-1};
    s16 neck_upper_bone{-1};

    // Weight distribution
    float head_weight{0.6f};
    float neck_lower_weight{0.25f};
    float neck_upper_weight{0.15f};

    // Limits
    float max_yaw{1.396f};     // 80 degrees
    float max_pitch{0.785f};   // 45 degrees
    float max_roll{0.349f};    // 20 degrees

    // Current rotation
    float current_yaw{0.f};
    float current_pitch{0.f};

    // Smoothing
    float rotation_speed{4.0f}; // rad/s

    // Active targets (priority-sorted)
    static constexpr int MAX_LOOK_AT_TARGETS = 4;
    LookAtTarget targets[MAX_LOOK_AT_TARGETS];
    int num_active_targets{0};
};

// Add to IKConfiguration
struct IKConfiguration {
    // ... existing ...
    HeadIKChain head;
    bool head_ik_available{false};
};
```

### 3.2 Look-At System

```cpp
class LookAtSystem {
public:
    // Target management
    static uint32_t AddLookAtTarget(
        entt::registry& registry,
        entt::entity character,
        const LookAtTarget& target);

    static void RemoveLookAtTarget(
        entt::registry& registry,
        entt::entity character,
        uint32_t target_id);

    static void UpdateLookAtTargets(
        entt::registry& registry,
        float delta_time);

    // Solve
    static void SolveHeadIK(
        HeadIKChain& head,
        ozz::span<const ozz::math::Float4x4> models,
        ozz::span<ozz::math::SoaTransform> locals,
        float delta_time);

private:
    static void UpdateTargetPositions(
        entt::registry& registry,
        HeadIKChain& head);

    static void SortTargetsByPriority(HeadIKChain& head);
    static void BlendTargets(HeadIKChain& head, float delta_time);

    static void CalculateLookDirection(
        const HeadIKChain& head,
        const ozz::math::Float4x4& head_transform,
        ozz::math::Float3& out_direction);

    static void ApplyHeadRotation(
        const HeadIKChain& head,
        float yaw,
        float pitch,
        ozz::span<ozz::math::SoaTransform> locals);
};
```

### 3.3 High-Level API

```cpp
// Convenient API for game code
class LookAtController {
public:
    // Look at entity
    uint32_t LookAt(
        entt::entity character,
        entt::entity target,
        float duration = -1.f,
        LookAtTarget::Priority priority = LookAtTarget::MEDIUM);

    // Look at position
    uint32_t LookAt(
        entt::entity character,
        ozz::math::Float3 position,
        float duration = -1.f,
        LookAtTarget::Priority priority = LookAtTarget::MEDIUM);

    // Look at bone on entity
    uint32_t LookAtBone(
        entt::entity character,
        entt::entity target,
        const char* bone_name,
        ozz::math::Float3 offset = {0,0,0},
        float duration = -1.f,
        LookAtTarget::Priority priority = LookAtTarget::MEDIUM);

    void StopLookAt(entt::entity character, uint32_t look_id);
    void StopAllLookAt(entt::entity character);

    bool IsLookingAt(entt::entity character, uint32_t look_id) const;
};
```

### Phase 3 Deliverables

**Features:**
- ✅ Multi-priority look-at system
- ✅ Entity tracking (auto-updates position)
- ✅ Bone tracking (track specific bone on entity)
- ✅ Duration control (timed look-ats)
- ✅ Blend in/out
- ✅ Head + neck distribution
- ✅ Optional torso integration
- ✅ Rotation limits

**Testing:**
- Look at moving NPC
- Multiple look-at priorities (higher priority takes over)
- Smooth transitions between targets
- Proper limits (can't look behind)
- Timed look-ats expire correctly

---

## Phase 4: Hand/Arm IK

**Goal:** Hand interaction and tracking system for doors, objects, weapons

### 4.1 Hand IK Component

```cpp
struct HandIKTarget {
    // Target (world space)
    ozz::math::Float3 position;
    ozz::math::Quaternion rotation;  // Hand orientation

    // Target type
    enum TargetType {
        ABSOLUTE,    // World space position/rotation
        RELATIVE     // Relative to entity/bone
    } type{ABSOLUTE};

    // For relative targets
    entt::entity target_entity{entt::null};
    s16 target_bone{-1};
    ozz::math::Float3 offset{0.f, 0.f, 0.f};
    ozz::math::Quaternion rotation_offset;

    // Distance-based blending
    float blend_in_distance{0.5f};   // Start blending at 0.5m
    float blend_out_distance{1.0f};  // Full blend when > 1.0m
    float max_reach{0.8f};           // Max reach distance

    // Blending
    float current_blend{0.f};
    float blend_in_rate{2.0f};
    float blend_out_rate{3.0f};

    // Status
    bool enabled{false};
    bool reachable{true};
    float distance_to_target{0.f};
};

struct ArmIKChain : LimbIKChain {
    // Extends base two-bone IK with hand-specific features

    // Hand target
    HandIKTarget target;

    // Wrist orientation control
    bool control_wrist_rotation{true};
    float wrist_rotation_weight{1.0f};

    // Elbow hint (pole vector alternative)
    ozz::math::Float3 elbow_hint_position;
    bool use_elbow_hint{false};

    // Recoil (for weapon firing)
    struct Recoil {
        bool active{false};
        ozz::math::Float3 offset{0.f, 0.f, 0.f};
        float decay_rate{5.0f};
    } recoil;
};

// Update IKConfiguration
struct IKConfiguration {
    // ... existing ...
    ArmIKChain left_arm_enhanced;
    ArmIKChain right_arm_enhanced;
    bool arm_ik_enhanced_available{false};
};
```

### 4.2 Hand IK System

```cpp
class HandIKSystem {
public:
    // Target setting
    static void SetHandTarget(
        ArmIKChain& arm,
        ozz::math::Float3 position,
        ozz::math::Quaternion rotation = ozz::math::Quaternion::identity());

    static void SetHandTargetRelative(
        ArmIKChain& arm,
        entt::entity target_entity,
        int bone_idx,
        ozz::math::Float3 offset,
        ozz::math::Quaternion rotation_offset);

    static void ClearHandTarget(ArmIKChain& arm);

    // Recoil
    static void ApplyRecoil(
        ArmIKChain& arm,
        ozz::math::Float3 recoil_offset);

    // Solve
    static void SolveHandIK(
        ArmIKChain& arm,
        const ozz::animation::Skeleton& skeleton,
        ozz::span<const ozz::math::Float4x4> models,
        ozz::span<ozz::math::SoaTransform> locals,
        float delta_time);

private:
    static void UpdateTargetPosition(
        entt::registry& registry,
        HandIKTarget& target);

    static void CalculateDistanceBlend(
        HandIKTarget& target,
        ozz::math::Float3 hand_position,
        float delta_time);

    static void SolveWristOrientation(
        const ArmIKChain& arm,
        const ozz::math::Quaternion& target_rotation,
        ozz::span<ozz::math::SoaTransform> locals);

    static void ApplyRecoilOffset(
        ArmIKChain::Recoil& recoil,
        ozz::math::Float3& hand_target,
        float delta_time);
};
```

### 4.3 Interaction System

```cpp
// High-level API for game interactions
class InteractionIKController {
public:
    // Door interaction
    void ReachForDoorHandle(
        entt::entity character,
        entt::entity door,
        bool left_hand = true);

    // Object interaction
    void ReachForObject(
        entt::entity character,
        entt::entity object,
        ozz::math::Float3 grab_point,
        bool left_hand = true);

    // Weapon holding
    void HoldWeapon(
        entt::entity character,
        entt::entity weapon,
        bool two_handed = false);

    void ReleaseWeapon(entt::entity character);

    // Mounted weapon
    void GrabMountedWeapon(
        entt::entity character,
        entt::entity weapon,
        ozz::math::Float3 left_grip,
        ozz::math::Float3 right_grip);

    void ReleaseMountedWeapon(entt::entity character);
};
```

### Phase 4 Deliverables

**Features:**
- ✅ Absolute and relative hand targets
- ✅ Wrist orientation control
- ✅ Distance-based auto-blending
- ✅ Reachability checking
- ✅ Recoil system for weapons
- ✅ Two-handed interaction support

**Testing:**
- Reach for door handle
- Hold rifle with both hands
- Grab mounted weapon
- Apply weapon recoil
- Out-of-reach targets blend smoothly

---

## Phase 5: Ragdoll Integration

**Goal:** Smooth transitions between animation and physics, with intelligent getup system

### 5.1 Ragdoll Transition Component

```cpp
struct RagdollTransition {
    // State
    enum State {
        IN_ANIMATION,      // Normal animation
        BLENDING_TO_RAGDOLL,
        IN_RAGDOLL,        // Full physics
        BLENDING_FROM_RAGDOLL,
        COMPLETE
    } state{IN_ANIMATION};

    // Blend control
    float ragdoll_blend{0.f};  // 0 = animation, 1 = ragdoll
    float blend_rate{5.0f};

    // Saved ragdoll pose (for blend-from)
    ozz::vector<ozz::math::SoaTransform> ragdoll_locals;
    bool ragdoll_pose_captured{false};

    // Getup system
    std::string selected_getup_anim;
    float getup_playback_time{0.f};
    float getup_duration{0.f};

    // Velocity tracking (for settling detection)
    float linear_velocity{0.f};
    float angular_velocity{0.f};
    float settled_time{0.f};
    float required_settled_time{1.0f};

    // Thresholds
    float max_linear_velocity{0.5f};   // m/s
    float max_angular_velocity{0.3f};  // rad/s
};

struct GetupAnimationSet {
    std::string name;
    std::vector<std::string> animation_names;

    // Selection criteria
    enum Orientation {
        ON_BACK,
        ON_FRONT,
        ON_LEFT_SIDE,
        ON_RIGHT_SIDE,
        SITTING
    } required_orientation;

    // Playback control
    float min_playback_rate{0.9f};
    float max_playback_rate{1.1f};

    // Early exit phases
    float early_out_phase{0.8f};           // Can exit at 80%
    float movement_breakout_phase{0.5f};   // Exit if player moves
    float aim_breakout_phase{0.6f};        // Exit if player aims

    // Blending
    float ragdoll_blend_duration{0.4f};

    // Flags
    bool allow_during_combat{false};
    bool require_grounded{true};
};
```

### 5.2 Ragdoll Transition System

```cpp
class RagdollTransitionSystem {
public:
    // Transition control
    static void StartRagdoll(
        entt::registry& registry,
        entt::entity character,
        ozz::math::Float3 impulse = {0,0,0});

    static void UpdateRagdollTransitions(
        entt::registry& registry,
        float delta_time);

    // Getup system
    static void SelectGetupAnimation(
        RagdollTransition& transition,
        const ozz::animation::Skeleton& skeleton,
        ozz::span<const ozz::math::Float4x4> ragdoll_pose);

    static void StartGetup(
        entt::registry& registry,
        entt::entity character);

private:
    // Orientation detection
    static GetupAnimationSet::Orientation DetermineOrientation(
        const ozz::animation::Skeleton& skeleton,
        ozz::span<const ozz::math::Float4x4> pose);

    // Settling detection
    static bool CheckIfSettled(
        RagdollTransition& transition,
        float delta_time);

    // Blending
    static void BlendAnimationToRagdoll(
        RagdollTransition& transition,
        ozz::span<ozz::math::SoaTransform> output_locals,
        ozz::span<const ozz::math::SoaTransform> anim_locals,
        ozz::span<const ozz::math::SoaTransform> ragdoll_locals,
        float delta_time);

    static void BlendRagdollToAnimation(
        RagdollTransition& transition,
        ozz::span<ozz::math::SoaTransform> output_locals,
        ozz::span<const ozz::math::SoaTransform> anim_locals,
        float delta_time);
};
```

### 5.3 Getup Animation Manager

```cpp
class GetupAnimationManager {
public:
    void RegisterGetupSet(const GetupAnimationSet& set);

    const GetupAnimationSet* FindBestSet(
        const ozz::animation::Skeleton& skeleton,
        ozz::span<const ozz::math::Float4x4> ragdoll_pose,
        bool in_combat = false) const;

    void LoadDefaultSets();  // Load standard getup animations

private:
    std::vector<GetupAnimationSet> sets_;
};
```

### 5.4 Physics Integration

```cpp
// Interface between IK and XRay physics
class RagdollPhysicsInterface {
public:
    // Get ragdoll pose from physics
    static void ExtractPhysicsPose(
        const PhysicsRagdoll& ragdoll,
        const ozz::animation::Skeleton& skeleton,
        ozz::span<ozz::math::SoaTransform> out_locals);

    // Apply animation pose to physics
    static void ApplyAnimationToPhysics(
        PhysicsRagdoll& ragdoll,
        const ozz::animation::Skeleton& skeleton,
        ozz::span<const ozz::math::Float4x4> models,
        float blend_factor);

    // Velocity queries
    static float GetLinearVelocity(const PhysicsRagdoll& ragdoll);
    static float GetAngularVelocity(const PhysicsRagdoll& ragdoll);

    // Impulse application
    static void ApplyImpulse(
        PhysicsRagdoll& ragdoll,
        ozz::math::Float3 impulse,
        ozz::math::Float3 position);
};
```

### Phase 5 Deliverables

**Features:**
- ✅ Smooth blend from animation to ragdoll
- ✅ Automatic settling detection (velocity-based)
- ✅ Orientation detection (back/front/side/sitting)
- ✅ Getup animation selection
- ✅ Smooth blend from ragdoll to animation
- ✅ Early breakout system (player can interrupt)
- ✅ Multiple getup animation sets

**Testing:**
- Push character → ragdoll → auto-getup
- Different orientations trigger different getups
- Player can interrupt getup with movement
- Smooth blends with no popping
- Works with physics impulses

**Getup Animation Sets to Create:**
- Standard getups (back, front, sides)
- Armed getups (holding weapon)
- Injured getups (low health)
- Combat getups (fast, defensive)
- Player-specific getups

---

## Phase 6: Live/Active IK

**Goal:** IK system that works during physics simulation, enabling dynamic reactions

### 6.1 Active IK Component

```cpp
struct ActiveIK {
    bool enabled{false};

    // IK during ragdoll
    bool maintain_hand_grip{false};      // Keep holding object during ragdoll
    entt::entity gripped_entity{entt::null};
    ozz::math::Float3 grip_position;

    bool protect_head{false};            // Hands move to protect head
    float protection_blend{0.f};

    bool balance_assistance{false};      // Legs attempt to regain balance
    float balance_strength{0.5f};

    // Reaction system
    struct Reaction {
        enum Type {
            NONE,
            STUMBLE,         // Light impact, try to recover
            BRACE_FALL,      // Falling, brace for impact
            PROTECT_HEAD,    // Incoming threat, protect head
            CATCH_SELF       // Try to break fall with hands
        } type{NONE};

        float intensity{0.f};
        float duration{0.f};
        float elapsed{0.f};

        // Reaction-specific data
        ozz::math::Float3 threat_direction;
        ozz::math::Float3 ground_contact_point;
    } reaction;

    // Drive-to-pose (similar to Euphoria)
    bool driving_to_pose{false};
    ozz::vector<ozz::math::SoaTransform> target_pose;
    float drive_strength{0.3f};
    float drive_duration{0.5f};
    float drive_elapsed{0.f};
};
```

### 6.2 Active IK System

```cpp
class ActiveIKSystem {
public:
    // Update during physics
    static void UpdateActiveDuringPhysics(
        entt::registry& registry,
        float delta_time);

    // Reaction triggering
    static void TriggerStumbleReaction(
        entt::entity character,
        ozz::math::Float3 push_direction,
        float intensity);

    static void TriggerBraceFallReaction(
        entt::entity character,
        ozz::math::Float3 ground_normal);

    static void TriggerProtectHeadReaction(
        entt::entity character,
        ozz::math::Float3 threat_direction);

    // Drive to pose
    static void StartDriveToPose(
        entt::entity character,
        ozz::span<const ozz::math::SoaTransform> target_pose,
        float strength,
        float duration);

private:
    static void ApplyProtectHead(
        ActiveIK& active_ik,
        IKConfiguration& ik_config,
        const ozz::animation::Skeleton& skeleton,
        ozz::span<ozz::math::SoaTransform> locals,
        float delta_time);

    static void ApplyBalanceAssistance(
        ActiveIK& active_ik,
        IKConfiguration& ik_config,
        const ozz::animation::Skeleton& skeleton,
        ozz::span<ozz::math::SoaTransform> locals,
        float delta_time);

    static void ApplyDriveToPose(
        ActiveIK& active_ik,
        ozz::span<ozz::math::SoaTransform> current_locals,
        float delta_time);

    static void ApplyMaintainGrip(
        ActiveIK& active_ik,
        ArmIKChain& arm,
        const ozz::animation::Skeleton& skeleton,
        ozz::span<const ozz::math::Float4x4> models,
        ozz::span<ozz::math::SoaTransform> locals);
};
```

### 6.3 Reaction System

```cpp
class ReactionSystem {
public:
    // Physics-based reactions
    static void OnPhysicsImpact(
        entt::registry& registry,
        entt::entity character,
        const PhysicsImpactInfo& impact);

    static void OnFalling(
        entt::registry& registry,
        entt::entity character,
        ozz::math::Float3 velocity);

    static void OnBalanceLoss(
        entt::registry& registry,
        entt::entity character,
        ozz::math::Float3 push_direction);

private:
    static void AnalyzeImpact(
        const PhysicsImpactInfo& impact,
        ActiveIK::Reaction& out_reaction);

    static void CalculateReactionStrength(
        const PhysicsImpactInfo& impact,
        float& out_strength);
};
```

### Phase 6 Deliverables

**Features:**
- ✅ Hand grip maintenance during ragdoll
- ✅ Head protection reactions
- ✅ Balance assistance (legs try to catch)
- ✅ Brace-for-fall reactions
- ✅ Drive-to-pose system
- ✅ Physics impact analysis
- ✅ Graduated response based on intensity

**Testing:**
- Push character while holding object (maintains grip)
- Falling triggers arm brace
- Impact from front triggers head protection
- Stumble attempts to regain balance
- Drive-to-pose guides toward getup animation

---

## Technical Architecture

### Integration with ozz-animation Pipeline

```cpp
// Complete update loop
void UpdateCharacterAnimation(
    entt::registry& registry,
    entt::entity entity,
    float delta_time)
{
    // Get components
    auto& skeleton = registry.get<Skeleton>(entity);
    auto& anim_controller = registry.get<AnimationController>(entity);
    auto& ik_config = registry.get<IKConfiguration>(entity);
    auto* ragdoll_transition = registry.try_get<RagdollTransition>(entity);
    auto* active_ik = registry.try_get<ActiveIK>(entity);

    // 1. Sample animation
    ozz::vector<ozz::math::SoaTransform> anim_locals;
    SampleAnimation(anim_controller, anim_locals, delta_time);

    // 2. Handle ragdoll state
    ozz::vector<ozz::math::SoaTransform> blended_locals = anim_locals;
    if (ragdoll_transition && ragdoll_transition->state != RagdollTransition::IN_ANIMATION) {
        RagdollTransitionSystem::UpdateRagdollTransitions(registry, delta_time);

        if (ragdoll_transition->state == RagdollTransition::IN_RAGDOLL) {
            // Extract physics pose
            RagdollPhysicsInterface::ExtractPhysicsPose(
                physics_ragdoll, skeleton, blended_locals);

            // Apply active IK during ragdoll (Phase 6)
            if (active_ik && active_ik->enabled) {
                ActiveIKSystem::UpdateActiveDuringPhysics(registry, delta_time);
            }
        }
        else if (ragdoll_transition->state == RagdollTransition::BLENDING_FROM_RAGDOLL) {
            // Blend from ragdoll to getup animation
            RagdollTransitionSystem::BlendRagdollToAnimation(
                *ragdoll_transition, blended_locals, anim_locals, delta_time);
        }
    }

    // 3. Convert to model space (for IK calculations)
    ozz::vector<ozz::math::Float4x4> models;
    ComputeModelSpace(blended_locals, skeleton, models);

    // 4. Apply IK systems (Phase 1-4)
    if (ik_config.IsInitialized() && ragdoll_transition->state != RagdollTransition::IN_RAGDOLL) {
        // Phase 1: Ground probing and foot IK
        if (ik_config.HasLegIK()) {
            GroundProbeSystem::UpdateFootProbes(registry, delta_time);
            FootLockingSystem::UpdateFootLocks(registry, models, delta_time);
            PelvisAdjustmentSystem::UpdatePelvis(registry, delta_time);

            // Solve leg IK
            IKSolverSystem::SolveLegIK(
                ik_config.left_leg, models, blended_locals, delta_time);
            IKSolverSystem::SolveLegIK(
                ik_config.right_leg, models, blended_locals, delta_time);
        }

        // Phase 2: Spine IK
        if (ik_config.spine_ik_available && ik_config.spine.enabled) {
            SpineIKSystem::SolveSpine(
                ik_config.spine, models, blended_locals, delta_time);
        }

        // Phase 3: Head look-at
        if (ik_config.head_ik_available) {
            LookAtSystem::UpdateLookAtTargets(registry, delta_time);
            LookAtSystem::SolveHeadIK(
                ik_config.head, models, blended_locals, delta_time);
        }

        // Phase 4: Hand IK
        if (ik_config.arm_ik_enhanced_available) {
            HandIKSystem::SolveHandIK(
                ik_config.left_arm_enhanced, skeleton, models, blended_locals, delta_time);
            HandIKSystem::SolveHandIK(
                ik_config.right_arm_enhanced, skeleton, models, blended_locals, delta_time);
        }
    }

    // 5. Final local-to-model conversion
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.input = ozz::make_span(blended_locals);
    ltm_job.output = ozz::make_span(models);
    ltm_job.skeleton = &skeleton.ozz_skeleton;
    ltm_job.Run();

    // 6. Apply to physics if blending
    if (ragdoll_transition &&
        ragdoll_transition->state == RagdollTransition::BLENDING_FROM_RAGDOLL) {
        RagdollPhysicsInterface::ApplyAnimationToPhysics(
            physics_ragdoll, skeleton, models, ragdoll_transition->ragdoll_blend);
    }

    // 7. Upload to renderer
    UpdateSkinningMatrices(models, skeleton);
}
```

### ECS Component Hierarchy

```
Entity (Character)
  ├─ Skeleton (ozz skeleton + joint names)
  ├─ AnimationController (playback state)
  │
  ├─ IKConfiguration (master container)
  │   ├─ LegIKEnhanced (left)
  │   │   └─ FootProbe
  │   ├─ LegIKEnhanced (right)
  │   │   └─ FootProbe
  │   ├─ PelvisIK
  │   ├─ SpineIKChain
  │   ├─ HeadIKChain
  │   │   └─ LookAtTarget[4]
  │   ├─ ArmIKChain (left)
  │   │   └─ HandIKTarget
  │   └─ ArmIKChain (right)
  │       └─ HandIKTarget
  │
  ├─ RagdollTransition (optional)
  │   └─ GetupAnimationSet (selected)
  │
  └─ ActiveIK (optional)
      └─ Reaction (current)
```

---

## Files Modified

**New Files:**
- `src/xrAnimation/AnimationECS_GroundProbe.h/cpp` (Phase 1.1)
- `src/xrAnimation/AnimationECS_PelvisIK.h/cpp` (Phase 1.2)
- `src/xrAnimation/AnimationECS_FootLocking.h/cpp` (Phase 1.3)
- `src/xrAnimation/AnimationECS_SpineIK.h/cpp` (Phase 2)
- `src/xrAnimation/AnimationECS_LookAt.h/cpp` (Phase 3)
- `src/xrAnimation/AnimationECS_HandIK.h/cpp` (Phase 4)
- `src/xrAnimation/AnimationECS_Ragdoll.h/cpp` (Phase 5)
- `src/xrAnimation/AnimationECS_ActiveIK.h/cpp` (Phase 6)
- `src/xrAnimation/AnimationECS_Controllers.h/cpp` (High-level APIs)

**Modified Files:**
- `src/xrAnimation/AnimationECS_IK.h` (extend existing components)
- `src/xrAnimation/AnimationECS_IK.cpp` (integrate new systems)
- `src/xrAnimation/AnimationECS_Systems.h` (add new system declarations)
- `src/xrAnimation/tools/ozz_animation_viewer.cpp` (test all features)
- `src/xrAnimation/CMakeLists.txt` (add new files)

---

## Next Immediate Steps

1. **Complete Current ECS IK Refactor:**
   - Finish ozz_animation_viewer.cpp refactoring
   - Test basic two-bone IK with ECS
   - Verify pole vector fixes

2. **Phase 1.1 - Ground Probing:**
   - Implement FootProbe component
   - Integrate with XRay physics raycasts
   - Test on flat ground and slopes

3. **Phase 1.2 - Pelvis Adjustment:**
   - Implement PelvisIK component
   - Add supporting foot logic
   - Test on stairs

4. **Expand incrementally through phases**

---

## References

**Euphoria Documentation:**
- `docs/rage_euphoria_port/EUPHORIA_ARCHITECTURE.md` - System overview
- `docs/rage_euphoria_port/EUPHORIA_BEHAVIORS.md` - 47+ behaviors
- `docs/rage_euphoria_port/EUPHORIA_MESSAGE_SYSTEM.md` - Message catalog
- `docs/rage_euphoria_port/EUPHORIA_LIMB_EFFECTORS.md` - Effector system
- `docs/rage_euphoria_port/EUPHORIA_GAME_INTEGRATION.md` - Integration patterns
- `docs/rage_euphoria_port/IK_RAGDOLL_ANIMATION_DEEP_DIVE.md` - **PRIMARY REFERENCE** for XRay porting

**Existing Code:**
- `src/xrAnimation/AnimationECS_IK.h` - Current IK foundation
- `src/xrAnimation/tools/ozz_animation_viewer.cpp` - Test application

---

**Document Version:** 2.0
**Last Updated:** 2025-10-12
**Status:** Implementation roadmap defined, ready to begin Phase 1

## Files Modified

- `src/xrAnimation/AnimationECS_IK.h` (NEW)
- `src/xrAnimation/AnimationECS_IK.cpp` (NEW)
- `src/xrAnimation/AnimationECS_Components.h` (added MotionID include)
- `src/xrAnimation/CMakeLists.txt` (added new files)
- `src/xrAnimation/tools/ozz_animation_viewer.cpp` (partially refactored)
