// xrRender/Bindless/TextureAtlas.h
// 2D Grid-Packing Texture Atlas
// Uses 4096×4096 pages with 128px cell-based allocation for variable-size textures
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

// ═══════════════════════════════════════════════════════
//  TEXTURE REGISTRATION RESULT
// ═══════════════════════════════════════════════════════

struct TextureRegisterResult {
    AtlasAllocation allocation;

    bool IsValid() const { return allocation.IsValid(); }
};

// ═══════════════════════════════════════════════════════
//  GRID-PACKING TEXTURE ATLAS
// ═══════════════════════════════════════════════════════
// Each atlas is a Texture2DArray where each slice is a 4096×4096 page.
// Textures are packed into pages using a 128px cell grid (32×32 cells per page).
// Supports textures from 64×64 to 2048×2048 (or even 4096×4096).

class TextureAtlas {
public:
    explicit TextureAtlas(AtlasType type);
    ~TextureAtlas();

    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    // Register a texture - allocates cells in a page and copies texture data
    // Returns allocation with page index and UV rect info
    TextureRegisterResult RegisterTexture(
        ng::RenderContext* ctx,
        const shared_str& textureName,
        nvrhi::ITexture* sourceTexture
    );

    // Get allocation for already-registered texture (returns invalid if not found)
    AtlasAllocation GetTextureAllocation(const shared_str& textureName) const;

    // Accessors
    AtlasType GetType() const { return m_type; }
    nvrhi::ITexture* GetArray() const { return m_array.Get(); }
    u32 GetTextureCount() const { return static_cast<u32>(m_textureMap.size()); }
    u32 GetPageCount() const { return static_cast<u32>(m_pages.size()); }
    bool IsInitialized() const { return m_initialized; }

    // Memory stats
    u64 GetAllocatedMemoryBytes() const;
    u64 GetUsedCellCount() const;
    float GetUtilization() const;

private:
    AtlasType m_type;
    ng::RenderDevice* m_device = nullptr;
    bool m_initialized = false;

    // Texture2DArray - each slice is a 4096×4096 page
    nvrhi::TextureHandle m_array;
    u32 m_arrayCapacity = 0;

    // Page occupancy tracking
    xr_vector<AtlasPage> m_pages;

    // Name -> allocation lookup
    xr_unordered_map<shared_str, AtlasAllocation> m_textureMap;

    // Try to allocate in an existing page
    // Returns page index, or UINT32_MAX if no space found
    u32 TryAllocateInPage(u32 pageIndex, u32 cellsWide, u32 cellsTall, u32& outCellX, u32& outCellY);

    // Add a new page to the atlas
    bool AddPage();

    // Ensure array has capacity for required pages
    void EnsurePageCapacity(u32 requiredPages);

    // Copy texture to a specific region in a page
    bool CopyTextureToRegion(
        ng::RenderContext* ctx,
        nvrhi::ITexture* sourceTexture,
        u32 pageIndex,
        u32 pixelX,
        u32 pixelY
    );

    // Bottom-left first-fit allocation
    AtlasAllocation Allocate(u32 textureWidth, u32 textureHeight);
};

// ═══════════════════════════════════════════════════════
//  TEXTURE ATLAS MANAGER
// ═══════════════════════════════════════════════════════

class TextureAtlasManager {
public:
    static TextureAtlasManager& Instance();

    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    // Get atlas by type
    TextureAtlas& GetAtlas(AtlasType type) { return m_atlases[static_cast<size_t>(type)]; }
    const TextureAtlas& GetAtlas(AtlasType type) const { return m_atlases[static_cast<size_t>(type)]; }

    // Convenience accessors
    TextureAtlas& Diffuse() { return GetAtlas(AtlasType::Diffuse); }
    TextureAtlas& Normal() { return GetAtlas(AtlasType::Normal); }
    TextureAtlas& Detail() { return GetAtlas(AtlasType::Detail); }
    TextureAtlas& PBR() { return GetAtlas(AtlasType::PBR); }

    bool IsInitialized() const { return m_initialized; }

    // Log memory usage for all atlases
    void LogMemoryUsage() const;

private:
    TextureAtlasManager();
    ~TextureAtlasManager();

    TextureAtlas m_atlases[TOTAL_ATLAS_COUNT];
    bool m_initialized = false;
};

} // namespace xray::render::RENDER_NAMESPACE::bindless
