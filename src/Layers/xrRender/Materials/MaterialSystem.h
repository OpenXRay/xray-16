#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/xr_resource.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"

namespace xray::render::resources {
    class FGResourceManager;
}

namespace xray::render::framegraph {
    class ShaderLoader;
}

namespace xray::render
{

class MaterialSystem
{
public:
    class MaterialInfo
    {
    public:
        bool alphaTest = false;
        u32 alphaRef = 0;
        bool transparent = false;
        u8 priority = 1;
        bool foliage = false;
        bool water = false;

        float metallic = 0.0f;
        float roughness = 0.5f;
        float aoScale = 1.0f;

        bool useParallax = false;
        bool useDetail = false;

        u32 shaderVariant = 0;
        float emissive = 0.0f;

        u32 GetMaterialFlags() const;
        float GetAlphaReference() const;
    };

    struct TextureSet
    {
        fg::TextureHandle albedo;
        fg::TextureHandle normal;
        fg::TextureHandle pbr;
        fg::TextureHandle detail;

        bool IsValid() const;
    };

    static MaterialSystem& Instance();

    void Initialize(resources::FGResourceManager* fgResourceManager, framegraph::ShaderLoader* shaderLoader);
    void Shutdown();

    const MaterialInfo& GetMaterialInfo(const char* shaderName, const char* textureName = "");
    const MaterialInfo& GetMaterialInfo(const shared_str& shaderName, const shared_str& textureName = shared_str());

    void PreloadMaterialTextures(const char* textureName);

    const TextureSet& GetTextures(const char* textureName) const;

    bool HasTextures(const char* textureName) const;

    void ClearCaches();

    struct Stats
    {
        u32 materialsCached = 0;
        u32 textureSetsCached = 0;
        u32 materialsFromBlender = 0;
        u32 materialsFromReflection = 0;
        u32 materialsFromDefaults = 0;
    };
    Stats GetStats() const;

private:
    MaterialSystem() = default;
    ~MaterialSystem() = default;
    MaterialSystem(const MaterialSystem&) = delete;
    MaterialSystem& operator=(const MaterialSystem&) = delete;

    friend MaterialSystem& GetMaterialSystemInstance();
    MaterialInfo GetDefaultMaterialInfo() const;

    xr_string GetAlbedoPath(const char* textureName) const;
    xr_string GetNormalPath(const char* textureName) const;
    xr_string GetPBRPath(const char* textureName) const;
    xr_string GetDetailPath(const char* textureName) const;

    xr_map<shared_str, MaterialInfo> m_materialCache;
    xr_map<shared_str, TextureSet> m_textureCache;

    TextureSet m_emptyTextureSet;

    MaterialInfo m_defaultMaterialInfo;

    resources::FGResourceManager* m_fgResourceManager = nullptr;
    framegraph::ShaderLoader* m_shaderLoader = nullptr;

    mutable Stats m_stats;

    bool m_initialized = false;
};

}
