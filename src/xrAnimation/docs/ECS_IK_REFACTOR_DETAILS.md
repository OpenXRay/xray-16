# ECS Architecture & Implementation Details

**Document Type:** Technical Deep Dive - ECS Migration & Architecture
**Last Updated:** 2025-10-12
**Status:** Planning & Reference Document

---

## Table of Contents

1. [Current Architecture Analysis](#current-architecture)
2. [ECS Migration Strategy](#ecs-migration)
3. [EnTT Integration](#entt-integration)
4. [Component & System Design Patterns](#design-patterns)
5. [Performance Considerations](#performance)

---

## Current Architecture Analysis {#current-architecture}

### OOP Structure Overview

**Deep Inheritance Hierarchies:**
```
GameObject (base class, ~400 lines)
  ├─ CEntity (adds networking)
  │   └─ CEntityAlive (adds health/conditions)
  │       ├─ CActor (player, 800+ lines)
  │       └─ CCustomMonster (AI NPCs, 600+ lines)
  ├─ CPhysicObject
  └─ CWeapon (weapons, 500+ lines)
```

**Multiple Inheritance for Features:**
```cpp
class CActor : public CEntityAlive,
               public IInputReceiver,
               public Feel::Touch,
               public CInventoryOwner,
               public CPhraseDialogManager,
               public CStepManager,
               public Feel::Sound
{
    // 7 base classes! Classic "god object" anti-pattern
};
```

**Pain Points:**
- Hard to parallelize (shared mutable state, virtual calls)
- Difficult to test (tight coupling)
- Poor cache locality (scattered data, pointer chasing)
- Rigid structure (hard to add cross-cutting features)
- Deep call stacks (performance overhead)

### Two-Phase Update System

#### Phase 1: `shedule_Update(u32 dt)` - Gameplay Logic
**Thread:** Game logic thread
**Frequency:** Variable (based on distance/priority)

**Responsibilities:**
- AI thinking and decision making
- Physics simulation (`g_Physics()`)
- Movement control
- State machines
- Network updates
- Script execution

#### Phase 2: `UpdateCL()` - Client Update
**Thread:** Main/render thread
**Frequency:** Every frame (60 FPS)

**Responsibilities:**
- Spatial updates (`spatial_update()`)
- Animation updates
- Visual effects (particles, lights)
- HUD updates
- Network interpolation/extrapolation
- Camera updates

### Actor Update Loop Analysis

**UpdateCL() Hot Path Operations:**
```cpp
void CActor::UpdateCL() {
    // 1. Input processing
    m_bPickupMode = true;

    // 2. Inventory update
    UpdateInventoryOwner(Device.dwTimeDelta);

    // 3. Touch physics (iterate all touched objects)
    for (auto& it : feel_touch)
        sh->character_physics_support()->movement()->UpdateObjectBox();

    // 4. Holder (vehicle) update
    if (m_holder)
        m_holder->UpdateEx(currentFOV());

    // 5. Physics support
    inherited::UpdateCL();  // CEntityAlive::UpdateCL()
    m_pPhysics_support->in_UpdateCL();

    // 6. Pickup mode
    PickupModeUpdate();
    PickupModeUpdate_COD();

    // 7. Weapon updates
    CWeapon* pWeapon = smart_cast<CWeapon*>(inventory().ActiveItem());
    // ... weapon zoom, recoil, etc.
}
```

**shedule_Update() Gameplay Operations:**
```cpp
void CActor::shedule_Update(u32 dT) {
    // 1. HUD attachment management
    g_player_hud->attach_item(pHudItem);
    g_player_hud->detach_item(pHudItem);

    // 2. Input → Physics pipeline
    g_cl_CheckControls(mstate_wishful, NET_SavedAccel, NET_Jump, dt);
    g_cl_Orientate(mstate_real, dt);  // Camera orientation
    g_Orientate(mstate_real, dt);     // Character orientation
    g_Physics(NET_SavedAccel, NET_Jump, dt);  // Physics simulation

    // 3. Animation selection
    g_SetAnimation(mstate_real);

    // 4. Feel systems
    feel_touch_update(C, R);           // Touch detection
    Feel_Grenade_Update(m_fFeelGrenadeRadius);

    // 5. Deferred messages
    UpdateDefferedMessages();

    // 6. Camera update
    cam_Update(dt, currentFOV());

    // 7. Network interpolation (for remote players)
    make_Interpolation();
}
```

### Monster AI Update Loop Analysis

**shedule_Update() AI Operations:**
```cpp
void CCustomMonster::shedule_Update(u32 dT) {
    // 1. Network queue management
    while ((NET.size() > 2) && (NET[1].dwTimeStamp < dwTimeCL))
        NET.pop_front();

    // 2. Vision system (CAN BE PARALLEL if mtAiVision enabled)
    Exec_Visibility();

    // 3. Memory system
    memory().update(dt);  // CMemoryManager update

    // 4. AI Think (state machine)
    if (GetScriptControl())
        ProcessScripts();
    else
        Think();  // Pure virtual - implemented by derived classes

    // 5. Movement manager
    movement().update(...);  // CMovementManager update

    // 6. Sound player
    sound().update();
}
```

**UpdateCL() Client Operations:**
```cpp
void CCustomMonster::UpdateCL() {
    // 1. Client update delta
    m_client_update_delta = (u32)std::min(Device.dwTimeGlobal - m_last_client_update_time, u32(100));

    // 2. Base update
    inherited::UpdateCL();  // CEntityAlive::UpdateCL()

    // 3. Sound callbacks
    CScriptEntity::process_sound_callbacks();

    // 4. Sound player (CAN BE PARALLEL)
    update_sound_player();  // Can push to Device.seqParallel

    // 5. Network interpolation/extrapolation
    if (NET.empty()) return;
    // ... complex network position/rotation interpolation

    // 6. Animation movement controller
    update_animation_movement_controller();
}
```

---

## ECS Migration Strategy {#ecs-migration}

### Core Philosophy

**Don't Refactor Everything At Once**

Start with new systems alongside OOP code:
1. Animation System (current focus)
2. Particle Effects
3. Physics/Movement
4. AI Systems
5. Rendering

Keep GameObject as facade during transition (multi-year process).

### Why EnTT?

**Chosen Library:** [EnTT](https://github.com/skypjack/entt)

**Advantages:**
- ✅ Header-only C++ library - Easy integration
- ✅ Cache-friendly sparse sets - Optimized memory layout
- ✅ Built-in parallelization - `view.each()` and `view.parallel()`
- ✅ Minimal overhead - Zero-cost abstractions
- ✅ Active development - Regular updates, good docs
- ✅ Battle-tested - Used in production games

**Alternatives Considered:**
- **flecs** - More features, slightly heavier, more opinionated
- **DECS** - Simpler but less optimized
- **Roll our own** - ❌ Not recommended (complex, time-consuming)

### Phase-by-Phase Migration

#### Phase 1: Animation System (Weeks 1-3) - **CURRENT**

**Why First:**
- Already data-driven (ozz-animation)
- Naturally parallel
- Clear component/system boundaries
- High performance impact
- Currently being implemented

**Components:**
```cpp
struct AnimationState {
    float current_time;
    float time_ratio;
    u16 current_partition;
    bool is_playing;
};

struct AnimationController {
    const ozz::animation::Skeleton* skeleton;  // Shared read-only
    const ozz::animation::Animation* animation;
    shared_str skeleton_name;
    shared_str animation_name;
};

struct AnimationBuffers {
    ozz::animation::SamplingJob::Context context;
    ozz::vector<ozz::math::SoaTransform> locals;
    ozz::vector<ozz::math::Float4x4> models;

    void Init(const ozz::animation::Skeleton* skeleton) {
        const int num_joints = skeleton->num_joints();
        const int num_soa_joints = skeleton->num_soa_joints();
        locals.resize(num_soa_joints);
        models.resize(num_joints);
        context.Resize(num_joints);
    }
};

struct BlendState {
    ozz::vector<ozz::animation::BlendingJob::Layer> layers;
    float transition_time;
    float transition_duration;
};
```

**Systems:**
```cpp
class AnimationSamplingSystem {
    entt::registry& registry;

public:
    void Update(float dt) {
        auto view = registry.view<AnimationState, AnimationController, AnimationBuffers>();

        // Can be parallelized!
        view.each([dt](auto entity, auto& state, auto& controller, auto& buffers) {
            if (!state.is_playing || !controller.animation)
                return;

            // Update time
            state.current_time += dt;
            state.time_ratio = state.current_time / controller.animation->duration();

            // Handle looping
            if (state.time_ratio > 1.0f) {
                if (controller.animation->looping())
                    state.time_ratio = fmodf(state.time_ratio, 1.0f);
                else {
                    state.time_ratio = 1.0f;
                    state.is_playing = false;
                }
            }

            // Sample animation
            ozz::animation::SamplingJob sampling_job;
            sampling_job.animation = controller.animation;
            sampling_job.context = &buffers.context;
            sampling_job.ratio = state.time_ratio;
            sampling_job.output = ozz::make_span(buffers.locals);
            sampling_job.Run();
        });
    }
};
```

**Integration:**
```cpp
class CKinematicsAnimatedECS : public IKinematicsAnimated {
private:
    entt::registry* m_registry;      // Shared global registry
    entt::entity m_entity;           // This object's entity

    // Fallback to old system if needed
    CKinematicsAnimated* m_legacy_impl;
    bool m_use_ecs;

public:
    void UpdateTracks() override {
        if (!m_use_ecs) {
            m_legacy_impl->UpdateTracks();
            return;
        }

        // ECS systems already updated - just sync results
        SyncBonesFromECS();
    }
};
```

#### Phase 2: Particle Effects (Weeks 4-5)

**Components:**
```cpp
struct ParticleEmitter {
    Fvector position;
    float emission_rate;
    u32 max_particles;
};

struct ParticleLifetime {
    float time_remaining;
    float max_lifetime;
};

struct ParticleVelocity {
    Fvector velocity;
};
```

**System:**
```cpp
class ParticleSystem {
    entt::registry registry;

    void Update(float dt) {
        auto view = registry.view<ParticleLifetime, ParticleVelocity, ParticleEmitter>();

        // Parallel update all particles
        view.each([dt](auto& lifetime, auto& velocity, auto& emitter) {
            lifetime.time_remaining -= dt;

            // Remove dead particles
            if (lifetime.time_remaining <= 0.0f) {
                // Mark for cleanup
            }

            // Update position based on velocity
            emitter.position.mad(velocity.velocity, dt);
        });
    }
};
```

#### Phase 3: Weapon Systems (Weeks 6-8)

**Components:**
```cpp
struct WeaponState {
    u32 current_state;  // eFire, eReload, eMisfire
    u32 next_state;
    bool bMisfire;
};

struct AmmoComponent {
    int iAmmoElapsed;
    int iMagazineSize;
    u8 m_ammoType;
    xr_vector<CCartridge> m_magazine;
};

struct WeaponAddons {
    ALife::EWeaponAddonStatus m_eScopeStatus;
    ALife::EWeaponAddonStatus m_eSilencerStatus;
    shared_str m_sScopeName;
};
```

#### Phase 4: Physics & Movement (Weeks 9-12)

**Components:**
```cpp
struct PhysicsCharacter {
    CCharacterPhysicsSupport* m_pPhysics_support;
    CPHMovementControl* movement_control;
    Fvector collision_box;
};

struct MovementState {
    u32 mstate_wishful;  // Desired state
    u32 mstate_real;     // Current state
    Fvector NET_SavedAccel;
    float NET_Jump;
};
```

#### Phase 5: AI Systems (Weeks 13-16)

**Components:**
```cpp
struct AIMemory {
    CMemoryManager* m_memory_manager;
    // Separate sub-components:
    // - VisualMemory, SoundMemory, HitMemory, etc.
};

struct AIMovement {
    CMovementManager* m_movement_manager;
    float m_fCurSpeed;
    Fvector tWatchDirection;
};

struct AIState {
    u32 current_state_id;
    float state_time;
    IGameObject* target_enemy;
};
```

#### Phase 6: Rendering (Weeks 17-20)

**Components:**
```cpp
struct Transform {
    Fmatrix xform;
    IGameObject* Parent;
};

struct MeshRenderer {
    IRenderVisual* visual;
    bool bVisible;
};

struct Material {
    // Material properties
};
```

---

## EnTT Integration {#entt-integration}

### Setup

**Add Submodule:**
```bash
cd /mnt/f/modding/claude_sessions/xray-16
git submodule add https://github.com/skypjack/entt.git Externals/entt
```

**CMake Integration:**
```cmake
# Externals/entt/CMakeLists.txt
add_library(EnTT INTERFACE)
target_include_directories(EnTT INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/single_include)

# src/xrAnimation/CMakeLists.txt
target_link_libraries(xrAnimation PUBLIC EnTT)
```

### Basic Usage Patterns

**Create Registry:**
```cpp
entt::registry registry;
```

**Create Entity:**
```cpp
entt::entity entity = registry.create();
```

**Add Components:**
```cpp
registry.emplace<AnimationState>(entity);
registry.emplace<AnimationController>(entity, skeleton_ptr, anim_ptr);
registry.emplace<AnimationBuffers>(entity);
```

**Query Components:**
```cpp
// Get single component
auto& state = registry.get<AnimationState>(entity);

// Check if has component
bool has_anim = registry.any_of<AnimationState>(entity);

// Remove component
registry.remove<AnimationState>(entity);
```

**Iterate Entities:**
```cpp
// Simple iteration
auto view = registry.view<AnimationState, Transform>();
for (auto entity : view) {
    auto& state = view.get<AnimationState>(entity);
    auto& transform = view.get<Transform>(entity);
    // ... process
}

// With callback
view.each([](auto entity, auto& state, auto& transform) {
    // ... process
});
```

### Parallelization with EnTT

**Sequential Update:**
```cpp
registry.view<AnimationState>().each([](auto entity, auto& anim) {
    // Update animation
});
```

**Parallel Update (C++17):**
```cpp
auto view = registry.view<AnimationState>();
std::for_each(std::execution::par, view.begin(), view.end(), [&](auto entity) {
    auto& anim = view.get<AnimationState>(entity);
    // Update logic
});
```

**X-Ray Parallel-For Integration:**
```cpp
void UpdateAllAnimations(entt::registry& registry, float dt) {
    // Gather all animated entities
    auto view = registry.view<AnimationState, AnimationController, AnimationBuffers>();

    xr_vector<entt::entity> entities;
    entities.reserve(view.size_hint());
    for (auto entity : view)
        entities.push_back(entity);

    // Parallel update using xr_parallel_for
    xr_parallel_for(TaskRange<size_t>(0, entities.size()),
        [&](const TaskRange<size_t>& range) {
            for (size_t idx = range.begin(); idx != range.end(); ++idx) {
                auto entity = entities[idx];
                auto& state = registry.get<AnimationState>(entity);
                auto& controller = registry.get<AnimationController>(entity);
                auto& buffers = registry.get<AnimationBuffers>(entity);
                UpdateSingleAnimation(state, controller, buffers, dt);
            }
        });
}
```

---

## Component & System Design Patterns {#design-patterns}

### Pattern 1: Data-Oriented Design

**Bad (OOP):**
```cpp
class GameObject {
    Transform transform;
    Health health;
    Inventory inventory;
    AI ai;
    // ... 50 more members

    virtual void Update(float dt) {
        // Mix logic and data
        if (health.current <= 0)
            Die();
    }
};
```

**Good (ECS):**
```cpp
// Pure data
struct Transform { Fmatrix xform; };
struct Health { float current, max; };
struct Inventory { /* ... */ };
struct AI { /* ... */ };

// Pure logic
class HealthSystem {
    void Update(entt::registry& registry) {
        auto view = registry.view<Health>();
        view.each([&](auto entity, auto& health) {
            if (health.current <= 0)
                registry.emplace<DeadTag>(entity);
        });
    }
};
```

### Pattern 2: Component Grouping

**Don't Over-Split:**
```cpp
// BAD: Too granular
struct PositionX { float x; };
struct PositionY { float y; };
struct PositionZ { float z; };

// GOOD: Logical grouping
struct Position { float x, y, z; };
```

**Balance:**
```cpp
// Single component when tightly coupled
struct Transform {
    Fmatrix xform;
    IGameObject* parent;
    svector<GameObjectSavedPosition, 4> position_stack;
};

// Split when independently used
struct PhysicsBody { /* ... */ };  // Only for physics objects
struct Renderable { /* ... */ };    // Only for visible objects
```

### Pattern 3: Tags for State

```cpp
// Use empty structs as tags
struct DeadTag {};
struct InCombatTag {};
struct StealthTag {};

// Query
auto dead_entities = registry.view<Health, DeadTag>();
auto in_combat = registry.view<AIState, InCombatTag>();

// Add/remove tags cheaply
registry.emplace<InCombatTag>(entity);
registry.remove<InCombatTag>(entity);
```

### Pattern 4: System Update Order

```cpp
class GameLoop {
    entt::registry registry;

    void Update(float dt) {
        // 1. Input System (Actor only)
        InputSystem::Update(registry, dt);

        // 2. AI Vision System (Monsters) - PARALLEL
        AIVisionSystem::UpdateParallel(registry, dt);

        // 3. AI Memory & Think
        AIMemorySystem::Update(registry, dt);
        AIThinkSystem::Update(registry, dt);

        // 4. Movement & Physics
        MovementSystem::Update(registry, dt);
        PhysicsSystem::Update(registry, dt);

        // 5. Animation Sampling - PARALLEL
        AnimationSamplingSystem::UpdateParallel(registry, dt);
        AnimationBlendingSystem::UpdateParallel(registry, dt);

        // 6. Rendering Prep
        SpatialSystem::Update(registry);
        RenderSystem::Update(registry);
    }
};
```

### Pattern 5: GameObject Facade During Transition

```cpp
class CGameObject {
protected:
    entt::entity m_ecs_entity;  // ECS handle
    entt::registry* m_registry;  // Shared registry

public:
    // Legacy API preserved
    virtual void UpdateCL() {
        // Delegate to ECS systems
        // Or call legacy implementation for non-migrated systems
    }

    // Lua scripts still use this
    Fvector GetPosition() {
        // Read from ECS Transform component
        auto& transform = m_registry->get<Transform>(m_ecs_entity);
        return transform.xform.c;
    }
};
```

---

## Performance Considerations {#performance}

### Expected Improvements

**Animation System:**
- **Current OOP:** 50 characters @ 60 FPS, ~15ms update time
- **Target ECS:** 500+ characters @ 60 FPS, <4ms update time
- **Benefit:** 10x character count, 4x parallelization, 75% time reduction

**AI System:**
- **Current OOP:** 30 AI characters @ 60 FPS, ~20ms AI time
- **Target ECS:** 100+ AI characters @ 60 FPS, <8ms AI time
- **Benefit:** 3x character count, 2.5x faster, parallel vision+sound

**Overall:**
- **Current:** ~50 FPS heavy scenes, ~30ms game logic, 25% CPU util
- **Target:** 60 FPS heavy scenes, <16ms game logic, 70%+ CPU util
- **Benefit:** Stable 60 FPS, 2x faster logic, 3x better CPU usage

### Cache Locality

**OOP (Poor):**
```
Memory Layout:
[Actor1: Transform|Health|Inventory|AI] [Actor2: ...] [Actor3: ...]
         ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
         Walking this wastes cache lines
```

**ECS (Good):**
```
Memory Layout:
Transforms: [Actor1|Actor2|Actor3|Actor4|...]  ← Sequential, cache-friendly
Health:     [Actor1|Actor2|Actor3|Actor4|...]  ← Sequential, cache-friendly
AI:         [Monster1|Monster2|...]            ← Only monsters, packed
```

### SIMD Opportunities

**OOP:**
```cpp
// Scalar operations
for (int i = 0; i < num_actors; ++i) {
    actors[i]->UpdateTransform();  // Virtual call, no SIMD
}
```

**ECS + SIMD:**
```cpp
// Process 4 entities at once
auto view = registry.view<Transform>();
for (int i = 0; i < num_transforms; i += 4) {
    // Load 4 transforms
    __m128 x = _mm_load_ps(&transforms[i].x);
    __m128 y = _mm_load_ps(&transforms[i].y);
    __m128 z = _mm_load_ps(&transforms[i].z);

    // Process in parallel
    // ...

    // Store back
    _mm_store_ps(&transforms[i].x, x);
}
```

### Memory Usage

**OOP Overhead:**
- Vtable pointers (8 bytes per object)
- Virtual inheritance (16+ bytes per object)
- Padding/alignment issues
- Scattered allocations

**ECS Benefits:**
- Packed component arrays
- No vtables for components
- Better alignment control
- Pool allocators

**Example:**
```
10,000 GameObjects (OOP):
- 10,000 × 300 bytes/object ≈ 3MB
- Scattered across heap
- Poor cache usage

10,000 Entities (ECS):
- Transforms: 10,000 × 64 bytes = 625KB
- Health: 10,000 × 8 bytes = 78KB
- Inventory: 1,000 × 512 bytes = 512KB (only actors have inventory)
Total: ~1.2MB, sequential, cache-friendly
```

---

## Migration Risks & Mitigations

### Risk 1: Scope Creep

**Mitigation:**
- Start with ONE system (animation)
- Prove it works before expanding
- Keep legacy version working alongside

### Risk 2: Lua Script Compatibility

**Mitigation:**
- GameObject facade maintains object-oriented API
- Scripts don't see ECS internals
- Comprehensive script testing after each migration

### Risk 3: Performance Regression

**Mitigation:**
- Extensive profiling before/after
- Benchmark suite with automated tests
- Console variable to disable ECS and use legacy path

### Risk 4: Team Learning Curve

**Mitigation:**
- Start simple, document patterns
- Pair programming
- Reference EnTT documentation and examples

---

## Success Metrics

### Phase 1 Success (Animation)
- ✅ EnTT integrated and building
- ✅ Animation system working with ECS components
- ✅ Performance equal or better than OOP
- ✅ Lua scripts work unchanged
- ✅ No crashes, no memory leaks

### Phase 2 Success (2-3 Systems)
- ✅ 2-3 systems migrated to ECS
- ✅ Measurable performance improvements
- ✅ Team comfortable with ECS patterns
- ✅ Documentation and examples created

### Long-term Success (Full Migration)
- ✅ Majority of hot-path systems using ECS
- ✅ 500+ animated characters @ 60 FPS
- ✅ Improved modding API
- ✅ Codebase easier to maintain and extend

---

## Key Resources

### EnTT
- [GitHub](https://github.com/skypjack/entt)
- [Documentation](https://github.com/skypjack/entt/wiki)
- [Examples](https://github.com/skypjack/entt/tree/master/test)

### ECS Theory
- **"Overwatch Gameplay Architecture"** - GDC Talk (Blizzard)
- **"Building a Data-Oriented Entity System"** - Blog series (Bitsquid)
- **"Understanding Component-Entity-Systems"** - Game Programming Patterns

### X-Ray Specific
- **ozz-animation multithread sample** - Proves parallel animation pattern
- **Current CKinematicsAnimated** - Understand existing architecture
- **Actor/NPC update loops** - Find integration points

---

## File Organization

### New ECS Module
```
src/xrECS/
├── Registry.h/cpp              # Global registry management
├── Systems/
│   ├── AnimationSystems.h/cpp  # Animation update systems
│   ├── ParticleSystems.h/cpp   # Particle systems
│   ├── PhysicsSystems.h/cpp    # Physics systems
│   └── AISystems.h/cpp         # AI systems
└── Components/
    ├── AnimationComponents.h   # Animation component definitions
    ├── PhysicsComponents.h     # Physics component definitions
    └── AIComponents.h          # AI component definitions
```

### Integration Files
```
src/xrGame/
├── GameObject_ECS.h/cpp        # ECS facade for GameObject
└── GameLoop_ECS.h/cpp          # ECS-enabled game loop
```

---

## Next Immediate Steps

1. **Complete Animation ECS Refactor** (current)
   - Finish ozz_animation_viewer.cpp integration
   - Test basic two-bone IK with ECS
   - Verify pole vector fixes

2. **Prove Performance Benefits**
   - Profile animation update (legacy vs ECS)
   - Measure character count limits
   - Document results

3. **Expand to Particles** (Phase 2)
   - Design particle components
   - Implement particle systems
   - Test with 1000+ particles

4. **Continue Incremental Migration**
   - Follow phase plan strictly
   - Don't rush to Phase 3 before Phase 2 complete
   - Maintain backward compatibility at all times

---

**Document Version:** 1.0
**Maintainer:** OpenXRay Animation Team
**Last Review:** 2025-10-12
