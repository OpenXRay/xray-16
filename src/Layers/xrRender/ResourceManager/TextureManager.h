#pragma once

#include "ResourceHandle.h"
#include <nvrhi/nvrhi.h>
#include <mutex>

namespace xray::render::ng {
    class RenderDevice;
}

namespace xray::render::resources {

class StreamingManager;
struct DDSData;

enum class TextureState : u8 {
    Unloaded,
    Loading,
    Resident,
    Evicting,
    Evicted,
};

const char* TextureStateToString(TextureState state);

enum class TexturePriority : u8 {
    Critical = 0,
    High = 1,
    Medium = 2,
    Low = 3,
    VeryLow = 4,
};

const char* TexturePriorityToString(TexturePriority priority);

struct TextureDesc {
    enum Type {
        Texture1D,
        Texture2D,
        Texture2DArray,
        Texture3D,
        TextureCube
    };

    Type type = Texture2D;

    u32 width = 0;
    u32 height = 0;
    u32 depth = 1;
    u32 arraySize = 1;
    u32 mipLevels = 1;

    nvrhi::Format format = nvrhi::Format::RGBA8_UNORM;

    bool isRenderTarget = false;
    bool isDepthStencil = false;
    bool isUAV = false;
    bool isSRGB = false;

    bool allowStreaming = true;
    u32 minResidentMips = 3;

    shared_str debugName;

    u64 CalculateMemorySize(u32 startMip = 0, u32 mipCount = 0) const;
};

struct TextureMetadata {
    shared_str filePath;
    TextureDesc desc;

    TextureState state = TextureState::Unloaded;
    TexturePriority priority = TexturePriority::Medium;
    u32 generation = 0;
    bool isAlive = true;

    u32 residentMips = 0;
    u32 requestedMips = 0;
    u32 totalMips = 0;

    u64 memoryUsed = 0;

    float lastAccessTime = 0.0f;
    u32 accessCount = 0;
    u32 refCount = 0;

    nvrhi::TextureHandle nvrhiTexture;

    xr_unique_ptr<DDSData> videoTextureData;

    struct LoadRequest {
        bool inProgress = false;
        u32 targetMips = 0;
    } loadRequest;

    bool IsResident() const {
        return state == TextureState::Resident;
    }

    bool NeedsStreaming() const {
        return requestedMips > residentMips && state == TextureState::Resident;
    }

    bool CanEvict() const {
        return priority >= TexturePriority::Low &&
               refCount == 0 &&
               state == TextureState::Resident;
    }
};

class TextureManager {
public:
    explicit TextureManager(xray::render::ng::RenderDevice* device);
    ~TextureManager();

    TextureHandle LoadTexture(
        const char* path,
        TexturePriority priority = TexturePriority::Medium
    );

    TextureHandle LoadTextureThreadSafe(
        const char* path,
        TexturePriority priority = TexturePriority::Medium
    );

    TextureHandle CreateTexture(
        const TextureDesc& desc,
        const void* initialData = nullptr
    );

    TextureHandle ImportTexture(
        nvrhi::TextureHandle nvrhiTexture,
        const TextureDesc& desc,
        const char* debugName
    );

    nvrhi::ITexture* GetNVRHITexture(TextureHandle handle);

    const TextureMetadata* GetMetadata(TextureHandle handle) const;

    bool IsResident(TextureHandle handle) const;

    void SetMemoryBudget(u64 bytes);
    u64 GetMemoryBudget() const { return m_memoryBudget; }

    void RequestMips(TextureHandle handle, u32 mipCount);

    void SetPriority(TextureHandle handle, TexturePriority priority);

    void Touch(TextureHandle handle);

    void AddRef(TextureHandle handle);

    void Release(TextureHandle handle);

    void Evict(TextureHandle handle);

    void Update(float deltaTime);

    struct Statistics {
        u64 totalMemoryUsed = 0;
        u64 memoryBudget = 0;

        u32 texturesTotal = 0;
        u32 texturesResident = 0;
        u32 texturesLoading = 0;
        u32 texturesEvicted = 0;

        u32 streamingRequestsPending = 0;
        u32 evictionsPending = 0;

        float memoryUsagePercent() const {
            if (memoryBudget == 0) return 0.0f;
            return (float)totalMemoryUsed / (float)memoryBudget * 100.0f;
        }
    };

    Statistics GetStatistics() const;
    void PrintStatistics() const;

    StreamingManager* GetStreamingManager() { return m_streamingManager.get(); }

private:
    xray::render::ng::RenderDevice* m_device;

    xr_vector<TextureMetadata> m_textures;
    xr_vector<u32> m_freeSlots;

    xr_map<shared_str, TextureHandle> m_pathToHandle;

    u64 m_memoryBudget = 2ULL * 1024 * 1024 * 1024;
    u64 m_memoryUsed = 0;

    xr_unique_ptr<StreamingManager> m_streamingManager;

    TextureHandle AllocateHandle();
    void FreeHandle(TextureHandle handle);
    bool ValidateHandle(TextureHandle handle) const;

    void LoadTextureSync(TextureHandle handle);
    void LoadTextureAsync(TextureHandle handle);
    void StreamMips(TextureHandle handle, u32 targetMips);

    bool CheckMemoryBudget(u64 requiredBytes) const;
    bool EnforceMemoryBudget(u64 requiredBytes);
    bool EvictTextures(u64 bytesNeeded);
    void EvictTextureInternal(TextureHandle handle);

    void UpdateVideoTextures();

    mutable std::mutex m_texturesMutex;
    mutable std::mutex m_pathLookupMutex;

    TextureHandle AllocateHandleThreadSafe();
    bool ValidateHandleThreadSafe(TextureHandle handle) const;

    mutable Statistics m_stats;
};

}
