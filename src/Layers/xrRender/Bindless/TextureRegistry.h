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

class TextureRegistry {
public:
    explicit TextureRegistry(TextureType type);
    ~TextureRegistry();

    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    u32 RegisterTexture(const shared_str& textureName, nvrhi::ITexture* texture);

    u32 GetTextureIndex(const shared_str& textureName) const;

    nvrhi::ITexture* GetTexture(u32 index) const;

    const xr_vector<TextureEntry>& GetTextures() const { return m_textures; }
    u32 GetTextureCount() const { return static_cast<u32>(m_textures.size()); }

    TextureType GetType() const { return m_type; }
    bool IsInitialized() const { return m_initialized; }

    nvrhi::ITexture* GetPlaceholder() const { return m_placeholder.Get(); }

private:
    TextureType m_type;
    ng::RenderDevice* m_device = nullptr;
    bool m_initialized = false;

    xr_vector<TextureEntry> m_textures;

    xr_unordered_map<shared_str, u32> m_textureMap;

    nvrhi::TextureHandle m_placeholder;

    void CreatePlaceholder();
};

class TextureAtlasManager {
public:
    static TextureAtlasManager& Instance();

    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    TextureRegistry& GetRegistry(TextureType type) { return m_registries[static_cast<size_t>(type)]; }
    const TextureRegistry& GetRegistry(TextureType type) const { return m_registries[static_cast<size_t>(type)]; }

    TextureRegistry& Diffuse() { return GetRegistry(TextureType::Diffuse); }
    TextureRegistry& Normal() { return GetRegistry(TextureType::Normal); }
    TextureRegistry& Detail() { return GetRegistry(TextureType::Detail); }
    TextureRegistry& PBR() { return GetRegistry(TextureType::PBR); }

    TextureRegistry& GetAtlas(TextureType type) { return GetRegistry(type); }

    bool IsInitialized() const { return m_initialized; }

    nvrhi::BindingLayoutHandle CreateBindingLayout(nvrhi::IDevice* device) const;

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

using TextureAtlas = TextureRegistry;

}
