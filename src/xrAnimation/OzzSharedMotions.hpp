#pragma once

#include "Include/xrRender/animation_motion.h"
#include "xrCommon/xr_map.h"
#include "xrCommon/xr_string.h"
#include "xrCommon/xr_unordered_map.h"
#include "xrCommon/xr_vector.h"
#include "xrCore/Animation/Motion.hpp"
#include "xrCore/Threading/Lock.hpp"
#include "xrCore/Threading/ScopeLock.hpp"
#include "xrCore/xrsharedmem.h"

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/skeleton.h"

#include <atomic>
#include <memory>

#ifndef ENGINE_API
#include "xrAPI/xrAPI.h"
#include "xrEngine/Engine.h"
#endif

namespace XRay::Animation
{
using MotionVec = xr_vector<CMotion>;
using BoneMotionMap = xr_map<shared_str, MotionVec>;
using BoneMotionsVec = xr_vector<MotionVec*>;

struct MotionLibraryHandle
{
    u64 id{ 0 };

    bool IsValid() const
    {
        return id != 0;
    }

    void Invalidate()
    {
        id = 0;
    }

    bool operator==(const MotionLibraryHandle& other) const
    {
        return id == other.id;
    }

    bool operator!=(const MotionLibraryHandle& other) const
    {
        return id != other.id;
    }
};

struct SkeletonFingerprint
{
    u32 hash{ 0 };
    u16 jointCount{ 0 };
    u16 version{ 0 };

    static SkeletonFingerprint Compute(const ozz::animation::Skeleton& skeleton);
    bool Matches(const SkeletonFingerprint& other) const;
};

enum class MotionLoadState : u8
{
    Unloaded,
    Loading,
    Loaded,
    Failed,
    Evicted
};

struct OzzMotionsValue
{
    struct MotionRecord
    {
        xr_string name;
        std::shared_ptr<ozz::animation::Animation> animation;
        CMotionDef definition;
        MotionID id;
        u32 frameCount{ 0 };

        u32 GetMemoryUsage() const;
    };

    xr_vector<MotionRecord> records;
    xr_unordered_map<xr_string, u16> lookup;

    BoneMotionMap bone_motions;

    MotionLibraryHandle handle;
    SkeletonFingerprint skelFingerprint;

    std::atomic<u32> refCount{ 0 };

    std::atomic<MotionLoadState> loadState{ MotionLoadState::Unloaded };

    shared_str sourceFile;
    u64 lastAccessTime{ 0 };
    u32 totalMemoryBytes{ 0 };
    u32 fileVersion{ 0 };

    bool Load(pcstr file_path, const ozz::animation::Skeleton& skeleton);

    bool LoadFromMemory(const std::vector<std::uint8_t>& data, const ozz::animation::Skeleton& skeleton, pcstr source_label = "<embedded>");

    MotionRecord* FindMotion(const xr_string& name);
    const MotionRecord* FindMotion(const xr_string& name) const;

    MotionRecord* FindMotion(u16 index);
    const MotionRecord* FindMotion(u16 index) const;

    MotionVec* GetBoneMotions(const shared_str& bone_name);
    const MotionVec* GetBoneMotions(const shared_str& bone_name) const;

    u16 GetMotionCount() const
    {
        return static_cast<u16>(records.size());
    }

    void UpdateMemoryUsage();
    void MarkAccessed();

    bool IsCompatibleWith(const SkeletonFingerprint& fingerprint) const
    {
        return skelFingerprint.Matches(fingerprint);
    }
};

class OzzMotionsContainer
{
public:
    struct Statistics
    {
        std::atomic<u64> cacheHits{ 0 };
        std::atomic<u64> cacheMisses{ 0 };
        std::atomic<u64> totalLoads{ 0 };
        std::atomic<u64> totalEvictions{ 0 };
        std::atomic<u64> peakMemoryBytes{ 0 };
        std::atomic<u64> currentMemoryBytes{ 0 };

        float GetHitRate() const
        {
            u64 total = cacheHits.load() + cacheMisses.load();
            return total > 0 ? static_cast<float>(cacheHits.load()) / total : 0.f;
        }
    };

    struct Config
    {
        u64 maxMemoryBytes = 512 * 1024 * 1024;
        bool enableLRUEviction = true;
        bool enablePrefetching = false;
        u32 prefetchThreads = 2;
        bool enableHotReload = false;
        bool enforceSkeletonCompatibility = true;
    };

    struct LoadRequest
    {
        shared_str key;
        SkeletonFingerprint skelFingerprint;
        bool blocking{ true };

        std::vector<std::uint8_t> embeddedData;
    };

private:
    using HandleToValueMap = xr_unordered_map<u64, OzzMotionsValue*>;
    using KeyToHandleMap = xr_unordered_map<shared_str, u64>;

    HandleToValueMap values;
    KeyToHandleMap keyToHandle;
    Lock containerLock;

    Statistics stats;
    Config config;

    std::atomic<u64> nextHandleID{ 1 };
    std::atomic<u32> globalVersion{ 0 };

public:
    OzzMotionsContainer();
    ~OzzMotionsContainer();

    MotionLibraryHandle Dock(const LoadRequest& request, const ozz::animation::Skeleton& skeleton);

    MotionLibraryHandle Dock(shared_str key, const ozz::animation::Skeleton& skeleton);

    OzzMotionsValue* Resolve(MotionLibraryHandle handle);
    const OzzMotionsValue* Resolve(MotionLibraryHandle handle) const;

    bool Has(shared_str key) const;

    void Release(MotionLibraryHandle handle);

    void Clean(bool force_destroy);

    void PrefetchBatch(const xr_vector<LoadRequest>& requests, const ozz::animation::Skeleton& skeleton);

    xr_vector<MotionLibraryHandle> LoadBatch(const xr_vector<LoadRequest>& requests, const ozz::animation::Skeleton& skeleton);

    void EvictLRU(u64 target_memory_bytes);

    bool EvictMotion(MotionLibraryHandle handle);

    u64 GetMemoryUsage() const
    {
        return stats.currentMemoryBytes.load();
    }

    void SetMemoryLimit(u64 max_bytes);

    void CheckForUpdates();

    bool ReloadMotion(MotionLibraryHandle handle, const ozz::animation::Skeleton& skeleton);

    const Statistics& GetStatistics() const
    {
        return stats;
    }

    void ResetStatistics();
    void Dump();

    u32 GetVersion() const
    {
        return globalVersion.load();
    }

    void SetConfig(const Config& cfg)
    {
        config = cfg;
    }

    const Config& GetConfig() const
    {
        return config;
    }

private:
    MotionLibraryHandle DockInternal(const LoadRequest& request, const ozz::animation::Skeleton& skeleton);
    void UpdateMemoryTracking(OzzMotionsValue* value, bool adding);
    void CheckMemoryPressure();

    u64 GenerateHandleID()
    {
        return nextHandleID.fetch_add(1, std::memory_order_relaxed);
    }

    struct EvictionCandidate
    {
        MotionLibraryHandle handle;
        u64 lastAccessTime;
        u32 memoryBytes;
    };

    xr_vector<EvictionCandidate> GatherEvictionCandidates();
};

extern ENGINE_API OzzMotionsContainer* g_pOzzMotionsContainer;

class SharedOzzMotions
{
    MotionLibraryHandle handle_;

    void Destroy()
    {
        if (handle_.IsValid() && g_pOzzMotionsContainer)
        {
            g_pOzzMotionsContainer->Release(handle_);
            handle_.Invalidate();
        }
    }

public:
    SharedOzzMotions() = default;

    ~SharedOzzMotions()
    {
        Destroy();
    }

    SharedOzzMotions(SharedOzzMotions&& other) noexcept : handle_(other.handle_)
    {
        other.handle_.Invalidate();
    }

    SharedOzzMotions& operator=(SharedOzzMotions&& other) noexcept
    {
        if (this != &other)
        {
            Destroy();
            handle_ = other.handle_;
            other.handle_.Invalidate();
        }
        return *this;
    }

    SharedOzzMotions(const SharedOzzMotions& other);
    SharedOzzMotions& operator=(const SharedOzzMotions& other);

    bool Create(shared_str key, const ozz::animation::Skeleton& skeleton)
    {
        Msg("[OzzMotionsContainer] SharedOzzMotions::Create called for: %s", key.c_str());
        Destroy();
        handle_ = g_pOzzMotionsContainer->Dock(key, skeleton);
        bool valid = handle_.IsValid();
        Msg("[OzzMotionsContainer] SharedOzzMotions::Create result: %s (handle=0x%llX)", valid ? "SUCCESS" : "FAILED", handle_.id);
        return valid;
    }

    bool Create(const OzzMotionsContainer::LoadRequest& request, const ozz::animation::Skeleton& skeleton)
    {
        Msg("[OzzMotionsContainer] SharedOzzMotions::Create(LoadRequest) called for: %s", request.key.c_str());
        Destroy();
        handle_ = g_pOzzMotionsContainer->Dock(request, skeleton);
        bool valid = handle_.IsValid();
        Msg("[OzzMotionsContainer] SharedOzzMotions::Create(LoadRequest) result: %s (handle=0x%llX)", valid ? "SUCCESS" : "FAILED", handle_.id);
        return valid;
    }

    OzzMotionsValue::MotionRecord* FindMotion(const xr_string& name) const
    {
        OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(handle_);
        return value ? value->FindMotion(name) : nullptr;
    }

    OzzMotionsValue::MotionRecord* FindMotion(u16 index) const
    {
        OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(handle_);
        return value ? value->FindMotion(index) : nullptr;
    }

    MotionVec* GetBoneMotions(const shared_str& bone_name) const
    {
        OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(handle_);
        return value ? value->GetBoneMotions(bone_name) : nullptr;
    }

    u16 GetMotionCount() const
    {
        const OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(handle_);
        return value ? value->GetMotionCount() : 0;
    }

    const shared_str& GetSourceFile() const
    {
        static shared_str empty;
        const OzzMotionsValue* value = g_pOzzMotionsContainer->Resolve(handle_);
        return value ? value->sourceFile : empty;
    }

    bool IsValid() const
    {
        return handle_.IsValid();
    }

    MotionLibraryHandle GetHandle() const
    {
        return handle_;
    }
};
} // namespace XRay::Animation
