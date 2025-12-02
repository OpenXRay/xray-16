// xrRender/Bindless/TextureAtlas.h
// Simplified texture atlas - one Texture2DArray per type, all same size/format
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
// Returns slice reference plus UV scale for smaller textures

struct TextureRegisterResult {
    TextureSliceRef sliceRef;
    float uvScaleU = 1.0f;
    float uvScaleV = 1.0f;

    bool IsValid() const { return sliceRef.IsValid(); }
};

// ═══════════════════════════════════════════════════════
//  TEXTURE ATLAS
// ═══════════════════════════════════════════════════════
// Single Texture2DArray with all textures resized to ATLAS_TEXTURE_SIZE
// Format: RGBA8_UNORM for all textures

class TextureAtlas {
public:
    explicit TextureAtlas(AtlasType type);
    ~TextureAtlas();

    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    // Register a texture - resizes to ATLAS_TEXTURE_SIZE, converts to RGBA8
    // Returns slice reference and UV scale for textures smaller than atlas size
    TextureRegisterResult RegisterTexture(
        ng::RenderContext* ctx,
        const shared_str& textureName,
        nvrhi::ITexture* sourceTexture
    );

    // Get slice index for already-registered texture (returns UINT32_MAX if not found)
    u32 GetTextureSlice(const shared_str& textureName) const;

    // Accessors
    AtlasType GetType() const { return m_type; }
    nvrhi::ITexture* GetArray() const { return m_array.Get(); }
    u32 GetTextureCount() const { return m_currentSlice; }
    bool IsInitialized() const { return m_initialized; }

private:
    AtlasType m_type;
    ng::RenderDevice* m_device = nullptr;
    bool m_initialized = false;

    // Single texture array for this type
    nvrhi::TextureHandle m_array;
    u32 m_currentSlice = 0;
    u32 m_arrayCapacity = 0;

    // Cached texture info (slice + UV scale)
    struct CachedTextureInfo {
        u32 sliceIndex;
        float uvScaleU;
        float uvScaleV;
    };

    // Name -> texture info lookup
    xr_unordered_map<shared_str, CachedTextureInfo> m_textureMap;

    // Create/resize the texture array
    void EnsureCapacity(u32 requiredSlices);

    // Copy and resize texture to atlas slice
    bool CopyTextureToSlice(
        ng::RenderContext* ctx,
        nvrhi::ITexture* sourceTexture,
        u32 sliceIndex,
        float& outUVScaleU,
        float& outUVScaleV
    );
};

// ═══════════════════════════════════════════════════════
//  TEXTURE ATLAS MANAGER
// ═══════════════════════════════════════════════════════
// Manages all 4 atlas types

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

private:
    TextureAtlasManager();
    ~TextureAtlasManager();

    TextureAtlas m_atlases[TOTAL_ATLAS_COUNT];
    bool m_initialized = false;
};

} // namespace xray::render::RENDER_NAMESPACE::bindless
