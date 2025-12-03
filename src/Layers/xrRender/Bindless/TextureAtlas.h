#pragma once

#include "BindlessTypes.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    namespace ng {
        class RenderDevice;
        class RenderContext;
    }
}

namespace xray::render::RENDER_NAMESPACE::bindless {

struct TextureRegisterResult {
    AtlasAllocation allocation;

    bool IsValid() const { return allocation.IsValid(); }
};

class TextureAtlas {
public:
    explicit TextureAtlas(AtlasType type);
    ~TextureAtlas();

    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    TextureRegisterResult RegisterTexture(
        ng::RenderContext* ctx,
        const shared_str& textureName,
        nvrhi::ITexture* sourceTexture
    );

    AtlasAllocation GetTextureAllocation(const shared_str& textureName) const;

    AtlasType GetType() const { return m_type; }
    nvrhi::ITexture* GetArray() const { return m_array.Get(); }
    u32 GetTextureCount() const { return static_cast<u32>(m_textureMap.size()); }
    u32 GetPageCount() const { return static_cast<u32>(m_pages.size()); }
    bool IsInitialized() const { return m_initialized; }

    u64 GetAllocatedMemoryBytes() const;
    u64 GetUsedCellCount() const;
    float GetUtilization() const;

private:
    AtlasType m_type;
    ng::RenderDevice* m_device = nullptr;
    bool m_initialized = false;

    nvrhi::TextureHandle m_array;
    u32 m_arrayCapacity = 0;

    xr_vector<AtlasPage> m_pages;

    xr_unordered_map<shared_str, AtlasAllocation> m_textureMap;

    u32 TryAllocateInPage(u32 pageIndex, u32 cellsWide, u32 cellsTall, u32& outCellX, u32& outCellY);

    bool AddPage();

    void EnsurePageCapacity(u32 requiredPages);

    bool CopyTextureToRegion(
        ng::RenderContext* ctx,
        nvrhi::ITexture* sourceTexture,
        u32 pageIndex,
        u32 pixelX,
        u32 pixelY
    );

    AtlasAllocation Allocate(u32 textureWidth, u32 textureHeight);
};

class TextureAtlasManager {
public:
    static TextureAtlasManager& Instance();

    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    TextureAtlas& GetAtlas(AtlasType type) { return m_atlases[static_cast<size_t>(type)]; }
    const TextureAtlas& GetAtlas(AtlasType type) const { return m_atlases[static_cast<size_t>(type)]; }

    TextureAtlas& Diffuse() { return GetAtlas(AtlasType::Diffuse); }
    TextureAtlas& Normal() { return GetAtlas(AtlasType::Normal); }
    TextureAtlas& Detail() { return GetAtlas(AtlasType::Detail); }
    TextureAtlas& PBR() { return GetAtlas(AtlasType::PBR); }

    bool IsInitialized() const { return m_initialized; }

    void LogMemoryUsage() const;

private:
    TextureAtlasManager();
    ~TextureAtlasManager();

    TextureAtlas m_atlases[TOTAL_ATLAS_COUNT];
    bool m_initialized = false;
};

}
