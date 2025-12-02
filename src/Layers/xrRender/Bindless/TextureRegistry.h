// xrRender/Bindless/TextureAtlas.h
// Bindless texture registry - manages textures for dynamic indexing
//
// NEW ARCHITECTURE:
// - Each texture keeps its native format (BC1, BC3, RGBA8, etc.)
// - Textures stored in flat arrays per type
// - Shader uses: Texture2D g_DiffuseTextures[MAX_BINDLESS_TEXTURES]
// - Sample with: g_DiffuseTextures[NonUniformResourceIndex(mat.diffuseIndex)]
// - Only 4 binding points needed (one per texture type)
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
//  TEXTURE REGISTRY
// ═══════════════════════════════════════════════════════
// Manages a single texture type (diffuse, normal, etc.)
// Stores textures in a flat array for bindless access

class TextureRegistry {
public:
    explicit TextureRegistry(TextureType type);
    ~TextureRegistry();

    // Initialize registry
    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    // Register a texture and get its index
    // Returns INVALID_TEXTURE_INDEX if registry is full
    u32 RegisterTexture(const shared_str& textureName, nvrhi::ITexture* texture);

    // Get index for an already-registered texture
    u32 GetTextureIndex(const shared_str& textureName) const;

    // Get texture by index
    nvrhi::ITexture* GetTexture(u32 index) const;

    // Get all textures for binding
    const xr_vector<TextureEntry>& GetTextures() const { return m_textures; }
    u32 GetTextureCount() const { return static_cast<u32>(m_textures.size()); }

    // Get type
    TextureType GetType() const { return m_type; }
    bool IsInitialized() const { return m_initialized; }

    // Create placeholder texture for empty slots
    nvrhi::ITexture* GetPlaceholder() const { return m_placeholder.Get(); }

private:
    TextureType m_type;
    ng::RenderDevice* m_device = nullptr;
    bool m_initialized = false;

    // Flat texture storage
    xr_vector<TextureEntry> m_textures;

    // Name -> index lookup
    xr_unordered_map<shared_str, u32> m_textureMap;

    // Placeholder texture for invalid/empty slots
    nvrhi::TextureHandle m_placeholder;

    void CreatePlaceholder();
};

// ═══════════════════════════════════════════════════════
//  TEXTURE REGISTRY MANAGER
// ═══════════════════════════════════════════════════════
// Manages all texture registries (diffuse, normal, detail, PBR)

class TextureAtlasManager {
public:
    static TextureAtlasManager& Instance();

    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    // Get registry by type
    TextureRegistry& GetRegistry(TextureType type) { return m_registries[static_cast<size_t>(type)]; }
    const TextureRegistry& GetRegistry(TextureType type) const { return m_registries[static_cast<size_t>(type)]; }

    // Convenience accessors
    TextureRegistry& Diffuse() { return GetRegistry(TextureType::Diffuse); }
    TextureRegistry& Normal() { return GetRegistry(TextureType::Normal); }
    TextureRegistry& Detail() { return GetRegistry(TextureType::Detail); }
    TextureRegistry& PBR() { return GetRegistry(TextureType::PBR); }

    // Legacy compatibility - aliases to new names
    TextureRegistry& GetAtlas(TextureType type) { return GetRegistry(type); }

    bool IsInitialized() const { return m_initialized; }

    // Create binding layout for all texture arrays
    nvrhi::BindingLayoutHandle CreateBindingLayout(nvrhi::IDevice* device) const;

    // Create binding set with all registered textures
    nvrhi::BindingSetHandle CreateBindingSet(
        nvrhi::IDevice* device,
        nvrhi::IBindingLayout* layout,
        nvrhi::IBuffer* materialBuffer,
        nvrhi::ISampler* sampler
    ) const;

private:
    TextureAtlasManager();
    ~TextureAtlasManager();

    TextureRegistry m_registries[static_cast<size_t>(TextureType::Count)];
    bool m_initialized = false;
};

// ═══════════════════════════════════════════════════════
//  LEGACY COMPATIBILITY ALIASES
// ═══════════════════════════════════════════════════════
// These allow existing code to compile during transition

using TextureAtlas = TextureRegistry;

} // namespace xray::render::RENDER_NAMESPACE::bindless
