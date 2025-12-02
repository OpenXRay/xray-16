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
    TextureSliceRef sliceRef;
    float uvScaleU = 1.0f;
    float uvScaleV = 1.0f;

    bool IsValid() const { return sliceRef.IsValid(); }
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

    u32 GetTextureSlice(const shared_str& textureName) const;

    AtlasType GetType() const { return m_type; }
    nvrhi::ITexture* GetArray() const { return m_array.Get(); }
    u32 GetTextureCount() const { return m_currentSlice; }
    bool IsInitialized() const { return m_initialized; }

private:
    AtlasType m_type;
    ng::RenderDevice* m_device = nullptr;
    bool m_initialized = false;

    nvrhi::TextureHandle m_array;
    u32 m_currentSlice = 0;
    u32 m_arrayCapacity = 0;

    struct CachedTextureInfo {
        u32 sliceIndex;
        float uvScaleU;
        float uvScaleV;
    };

    xr_unordered_map<shared_str, CachedTextureInfo> m_textureMap;

    void EnsureCapacity(u32 requiredSlices);

    bool CopyTextureToSlice(
        ng::RenderContext* ctx,
        nvrhi::ITexture* sourceTexture,
        u32 sliceIndex,
        float& outUVScaleU,
        float& outUVScaleV
    );
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

private:
    TextureAtlasManager();
    ~TextureAtlasManager();

    TextureAtlas m_atlases[TOTAL_ATLAS_COUNT];
    bool m_initialized = false;
};

}
