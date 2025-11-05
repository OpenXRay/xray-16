# Technical Context & Code Patterns

**Document Type:** Code Context & Implementation Patterns
**Last Updated:** 2025-10-12
**Status:** Reference Document

---

## Table of Contents

1. [Performance Optimization Patterns](#performance)
2. [Threading & Parallelization](#threading)
3. [Memory Management](#memory)
4. [Code Organization](#organization)
5. [Common Patterns](#patterns)

---

## Performance Optimization Patterns {#performance}

### Current Status
- ✅ MVP delivered: 50-100 animated characters functional
- ✅ Animation update working, not yet optimized
- 🎯 Target: 500-1000 characters @ 60 FPS, <4ms animation budget

### Parallel Animation Processing (ozz-animation Pattern)

**Key Insight:** All ozz jobs are thread-safe by design
- `ozz::animation::SamplingJob`
- `ozz::animation::BlendingJob`
- `ozz::animation::LocalToModelJob`

**Why?** Data-driven architecture - clear separation between data (buffers) and processes (jobs)

#### Data Isolation Pattern (Per-Character)

```cpp
struct OzzCharacterData {
    // Thread-local mutable data
    ozz::animation::SamplingJob::Context context;
    ozz::vector<ozz::math::SoaTransform> locals;
    ozz::vector<ozz::math::Float4x4> models;

    // Shared read-only data (safe for concurrent access)
    const ozz::animation::Skeleton* skeleton;
    const ozz::animation::Animation* animation;

    // Character-specific state
    float animation_time;
    float weight;
};
```

#### Recursive Parallel-For Pattern

```cpp
// Based on ozz-animation multithread sample
bool ParallelUpdateCharacters(Args args, OzzCharacterData* chars, int num) {
    // Base case: sequential processing for small batches
    if (num <= grain_size) {
        for (int i = 0; i < num; ++i) {
            UpdateSingleCharacter(&chars[i]);
        }
        return true;
    }

    // Recursive case: split work
    int half = num / 2;

    // First half runs async
    auto future = std::async(std::launch::async,
        ParallelUpdateCharacters, args, chars, half);

    // Second half runs on this thread
    bool success = ParallelUpdateCharacters(args, chars + half, num - half);

    // Wait for async half
    success &= future.get();

    return success;
}
```

**Grain Size Tuning:**
```cpp
// Optimal grain size (tune based on profiling)
constexpr int grain_size = 64;  // 64-128 characters per task

// For X-Ray's xr_parallel_for:
xr_parallel_for(TaskRange<size_t>(0, num_characters),
    [&](const TaskRange<size_t>& range) {
        for (size_t i = range.begin(); i != range.end(); ++i) {
            UpdateSingleCharacter(&characters[i]);
        }
    });
```

---

## Threading & Parallelization {#threading}

### Thread Pool for Startup Conversion

```cpp
class OzzConversionThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_{false};

    // Progress tracking
    std::atomic<int> completed_{0};
    std::atomic<int> total_{0};

public:
    OzzConversionThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });

                        if (stop_ && tasks_.empty())
                            return;

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    task();  // Execute
                    ++completed_;
                }
            });
        }
    }

    template<typename F>
    void Enqueue(F&& task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            tasks_.emplace(std::forward<F>(task));
            ++total_;
        }
        condition_.notify_one();
    }

    float GetProgress() const {
        int done = completed_.load();
        int all = total_.load();
        return all > 0 ? (float)done / all : 0.f;
    }

    ~OzzConversionThreadPool() {
        stop_ = true;
        condition_.notify_all();
        for (std::thread& worker : workers_)
            worker.join();
    }
};
```

**Usage Pattern:**
```cpp
// Startup conversion with progress
OzzConversionThreadPool pool(8);  // 8 threads

// Enqueue all conversion tasks
for (const auto& asset : unconverted_assets) {
    pool.Enqueue([&asset]() {
        ConvertAsset(asset);
    });
}

// Update loading screen with progress
while (pool.GetProgress() < 1.0f) {
    LoadTitle("Converting assets", pool.GetProgress());
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}
```

### X-Ray Parallel-For Integration

```cpp
// X-Ray's existing parallel system
void UpdateAllAnimationsParallel(xr_vector<OzzCharacterData>& characters) {
    xr_parallel_for(TaskRange<size_t>(0, characters.size()),
        [&](const TaskRange<size_t>& range) {
            for (size_t idx = range.begin(); idx != range.end(); ++idx) {
                auto& character = characters[idx];

                // 1. Sample animation
                ozz::animation::SamplingJob sampling;
                sampling.animation = character.animation;
                sampling.context = &character.context;
                sampling.ratio = character.animation_time;
                sampling.output = ozz::make_span(character.locals);
                sampling.Run();

                // 2. Convert to model space
                ozz::animation::LocalToModelJob ltm;
                ltm.skeleton = character.skeleton;
                ltm.input = ozz::make_span(character.locals);
                ltm.output = ozz::make_span(character.models);
                ltm.Run();
            }
        });
}
```

---

## Memory Management {#memory}

### Memory Pool for Frequent Allocations

```cpp
class BoneTransformPool {
    static constexpr size_t POOL_SIZE = 1024;
    static constexpr size_t MAX_BONES = 256;

    struct Entry {
        ozz::math::Float4x4 transforms[MAX_BONES];
        bool in_use{false};
    };

    std::array<Entry, POOL_SIZE> pool_;
    std::atomic<size_t> next_free_{0};

public:
    // Allocate transform buffer
    ozz::span<ozz::math::Float4x4> Allocate(size_t count) {
        if (count > MAX_BONES)
            return {};  // Too large

        size_t idx = next_free_.fetch_add(1, std::memory_order_relaxed);
        if (idx >= POOL_SIZE) {
            // Pool exhausted - fall back to heap
            return {};
        }

        Entry& entry = pool_[idx];
        entry.in_use = true;
        return ozz::make_span(entry.transforms, count);
    }

    // Reset pool (per-frame)
    void Reset() {
        next_free_.store(0, std::memory_order_relaxed);
        for (auto& entry : pool_)
            entry.in_use = false;
    }
};
```

### Shared Resource Management

**Pattern:** Reference-counted shared data
```cpp
// Shared skeleton (read-only, many references)
struct SharedSkeleton {
    std::shared_ptr<ozz::animation::Skeleton> skeleton;
    SkeletonFingerprint fingerprint;
    std::atomic<u32> refCount{0};

    static std::shared_ptr<SharedSkeleton> Load(const char* path) {
        // Check cache first
        auto cached = g_skeleton_cache.Find(path);
        if (cached)
            return cached;

        // Load and cache
        auto skel = std::make_shared<SharedSkeleton>();
        skel->skeleton = LoadOzzSkeleton(path);
        skel->fingerprint = ComputeFingerprint(skel->skeleton.get());
        g_skeleton_cache.Insert(path, skel);
        return skel;
    }
};
```

---

## Code Organization {#organization}

### File Structure Best Practices

**Modular Organization:**
```
src/xrAnimation/
├── Core/                              # Core data structures
│   ├── OzzKinematicsCore.h/cpp        # Shared skeleton/bone state
│   ├── OzzConversion.h/cpp            # Coordinate conversion utilities
│   └── OzzBundle.h/cpp                # .ozzx file I/O
│
├── Runtime/                           # Runtime façade
│   ├── OzzKinematics.h/cpp            # Static kinematics (IKinematics)
│   ├── OzzKinematicsAnimated.h/cpp    # Animated (IKinematicsAnimated)
│   └── OzzAnimationSystem.h/cpp       # Animation update logic
│
├── Conversion/                        # Asset conversion
│   ├── SkeletonConverter.h/cpp        # OGF → ozz skeleton
│   ├── AnimationConverter.h/cpp       # OMF → ozz animation
│   └── StartupConversion.h/cpp        # Runtime conversion system
│
├── ECS/                               # ECS components and systems
│   ├── Components/                    # Component definitions
│   │   ├── AnimationComponents.h
│   │   └── IKComponents.h
│   └── Systems/                       # System implementations
│       ├── AnimationSystems.h/cpp
│       └── IKSystems.h/cpp
│
└── tools/                             # Tools and utilities
    ├── xray_to_ozz_converter/         # CLI converter
    └── ozz_animation_viewer/          # Visualization tool
```

### Header Organization

**Good Pattern:**
```cpp
// MyClass.h
#pragma once

// X-Ray includes (alphabetical within groups)
#include "xrCore/xrCore.h"
#include "xrEngine/IGame_Level.h"

// Third-party includes
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

// Forward declarations (avoid includes when possible)
class IReader;
class IWriter;

namespace XRay::Animation {

class MyClass {
    // Public interface first
public:
    MyClass();
    ~MyClass();

    void Initialize();
    void Update(float dt);

    // Implementation details last
private:
    struct Impl;  // PIMPL pattern for large private sections
    xr_unique_ptr<Impl> impl_;
};

} // namespace XRay::Animation
```

---

## Common Patterns {#patterns}

### Pattern 1: RAII Resource Management

```cpp
// GOOD: RAII wrapper for X-Ray resources
class ScopedReader {
    IReader* reader_;

public:
    explicit ScopedReader(const char* path)
        : reader_(FS.r_open(path))
    {
        if (!reader_)
            xrDebug::Fatal(DEBUG_INFO, "Failed to open: %s", path);
    }

    ~ScopedReader() {
        if (reader_)
            FS.r_close(reader_);
    }

    IReader* Get() const { return reader_; }
    IReader* operator->() const { return reader_; }

    // No copy, allow move
    ScopedReader(const ScopedReader&) = delete;
    ScopedReader& operator=(const ScopedReader&) = delete;
    ScopedReader(ScopedReader&& other) noexcept
        : reader_(other.reader_) { other.reader_ = nullptr; }
};

// Usage
void LoadData(const char* path) {
    ScopedReader reader(path);
    // Automatically closed on scope exit
}
```

### Pattern 2: Lazy Initialization

```cpp
class OzzKinematicsAnimated {
    // Lazy-loaded motion library
    mutable SharedOzzMotions motions_;
    bool motions_initialized_{false};

public:
    const SharedOzzMotions& GetMotions() const {
        if (!motions_initialized_) {
            motions_.Create(motion_file_, skeleton_);
            motions_initialized_ = true;
        }
        return motions_;
    }
};
```

### Pattern 3: State Validation

```cpp
// Always validate state before operations
bool OzzKinematics::CalculateBones(BOOL force_recalc) {
    // 1. Check initialized
    if (!core_.IsInitialized()) {
        Msg("! [OzzKinematics] Not initialized");
        return false;
    }

    // 2. Check throttle (unless forced)
    if (!force_recalc && !ShouldUpdate()) {
        return true;  // Skip update
    }

    // 3. Validate data
    if (core_.Skeleton()->num_joints() == 0) {
        Msg("! [OzzKinematics] Empty skeleton");
        return false;
    }

    // 4. Perform operation
    UpdatePose();
    return true;
}
```

### Pattern 4: Error Reporting

```cpp
// GOOD: Informative error messages with context
bool ConvertOGF(const char* path) {
    IReader* reader = FS.r_open(path);
    if (!reader) {
        Msg("! [OGF Converter] Failed to open file: %s", path);
        Msg("  - Check file path is correct");
        Msg("  - Ensure file exists in game filesystem");
        Msg("  - Verify file is not corrupted");
        return false;
    }

    u32 chunk_id = reader->r_u32();
    if (chunk_id != OGF_HEADER) {
        Msg("! [OGF Converter] Invalid OGF header");
        Msg("  - Expected chunk ID: 0x%08X", OGF_HEADER);
        Msg("  - Found chunk ID:    0x%08X", chunk_id);
        Msg("  - File: %s", path);
        FS.r_close(reader);
        return false;
    }

    // ... continue conversion
}
```

### Pattern 5: Performance Measurement

```cpp
class ScopedTimer {
    const char* name_;
    std::chrono::high_resolution_clock::time_point start_;

public:
    explicit ScopedTimer(const char* name)
        : name_(name)
        , start_(std::chrono::high_resolution_clock::now())
    {}

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        Msg("[Perf] %s: %.3f ms", name_, duration.count() / 1000.0f);
    }
};

// Usage
void UpdateAnimations() {
    ScopedTimer timer("UpdateAnimations");
    // ... do work
}  // Automatically logs time on scope exit
```

---

## Performance Targets & Metrics

### Current Baseline (MVP)
- **Characters:** 50-100 animated @ 60 FPS
- **Animation Update:** Not yet optimized, ~15ms
- **Memory:** Per-instance motion libraries (~10-50x duplication)
- **Threading:** Single-threaded animation update
- **CPU Utilization:** ~25%

### Optimization Target
- **Characters:** 500-1000 animated @ 60 FPS
- **Animation Update:** <4ms budget
- **Memory:** Shared motion libraries (10-50x reduction)
- **Threading:** Parallel updates (xr_parallel_for)
- **CPU Utilization:** 70%+ on 8-core systems

### Measurement Points
```cpp
// Add profiling markers
void UpdateAllCharacters(float dt) {
    ScopedTimer total_timer("TotalAnimationUpdate");

    {
        ScopedTimer sampling_timer("SamplingPhase");
        // Sample all animations
    }

    {
        ScopedTimer blending_timer("BlendingPhase");
        // Blend animations
    }

    {
        ScopedTimer ltm_timer("LocalToModelPhase");
        // Convert to model space
    }
}

// Expected timings (500 characters):
// - SamplingPhase:      ~1.5ms
// - BlendingPhase:      ~1.0ms
// - LocalToModelPhase:  ~1.0ms
// - TotalAnimationUpdate: ~3.5ms (leaves 12.5ms for other systems @ 60 FPS)
```

---

## Cache-Friendly Data Layouts

### Structure-of-Arrays (SoA) Pattern

**BAD: Array-of-Structures (AoS)**
```cpp
struct BoneData_AoS {
    ozz::math::Float4x4 transform;
    ozz::math::Float3 position;
    ozz::math::Quaternion rotation;
    float scale;
    u32 flags;
};
xr_vector<BoneData_AoS> bones;  // Poor cache locality
```

**GOOD: Structure-of-Arrays (SoA)**
```cpp
struct BoneData_SoA {
    ozz::vector<ozz::math::Float4x4> transforms;
    ozz::vector<ozz::math::Float3> positions;
    ozz::vector<ozz::math::Quaternion> rotations;
    ozz::vector<float> scales;
    ozz::vector<u32> flags;
};
BoneData_SoA bones;  // Excellent cache locality when iterating transforms
```

**Why Better?**
- Sequential memory access (cache-friendly)
- SIMD-friendly (process 4+ bones at once)
- Less memory waste (no padding between fields)

---

## Testing Patterns

### Unit Test Structure
```cpp
TEST(OzzKinematics, InitializeFromOzz) {
    // Arrange
    OzzKinematics kinematics;
    const char* skeleton_path = "testdata/stalker_hero.ozz";

    // Act
    bool success = kinematics.InitializeFromOzz(skeleton_path);

    // Assert
    EXPECT_TRUE(success);
    EXPECT_GT(kinematics.LL_BoneCount(), 0);
    EXPECT_NE(kinematics.LL_BoneID("bip01_spine"), BI_NONE);
}
```

### Parity Test Pattern
```cpp
TEST(OzzKinematicsParity, AnimationPoseMatchesLegacy) {
    // Load same asset in both systems
    CKinematics legacy;
    OzzKinematics ozz;

    legacy.Load("stalker_hero.ogf");
    ozz.InitializeFromOzz("stalker_hero.ozz");

    // Sample same animation at same time
    float time = 0.5f;  // 50% through animation
    legacy.PlayCycle(walk_motion_id);
    legacy.UpdateTracks(time);

    ozz.PlayAnimation(walk_anim, time);
    ozz.CalculateBones(TRUE);

    // Compare bone transforms
    for (u16 bone_id = 0; bone_id < ozz.LL_BoneCount(); ++bone_id) {
        Fmatrix legacy_xform = legacy.LL_GetTransform(bone_id);
        Fmatrix ozz_xform = ozz.LL_GetTransform(bone_id);

        // Compare with tolerance
        EXPECT_MATRIX_NEAR(legacy_xform, ozz_xform, 0.001f);
    }
}
```

---

**Document Version:** 1.0
**Maintainer:** OpenXRay Animation Team
**Last Review:** 2025-10-12
