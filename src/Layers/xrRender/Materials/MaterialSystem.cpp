#include "stdafx.h"
#include "MaterialSystem.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/ShaderVariant/ShaderVariantRegistry.h"
#include "Layers/xrRender/ShaderVariant/VariantPSOCache.h"

using namespace xray::render::resources;
using namespace xray::render::framegraph;

namespace xray::render
{

// Friend function for singleton access (allows private constructor)
MaterialSystem& GetMaterialSystemInstance()
{
    static MaterialSystem instance;
    return instance;
}

MaterialSystem& MaterialSystem::Instance()
{
    return GetMaterialSystemInstance();
}

bool MaterialSystem::TextureSet::IsValid() const
{
    // At minimum, we need an albedo texture
    return albedo.IsValid();
}

void MaterialSystem::Initialize(resources::FGResourceManager* fgResourceManager, framegraph::ShaderLoader* shaderLoader)
{
    if (m_initialized)
    {
        Msg("! [MaterialSystem] Already initialized");
        return;
    }

    m_fgResourceManager = fgResourceManager;
    m_shaderLoader = shaderLoader;

    // Set up defaults
    m_defaultMaterialInfo = MaterialInfo{};
    m_emptyTextureSet = TextureSet{};

    ShaderVariantRegistry::Instance().Initialize();

    m_initialized = true;
    Msg("* [MaterialSystem] Initialized");
}

void MaterialSystem::Shutdown()
{
    if (!m_initialized)
        return;

    VariantPSOCache::Instance().Shutdown();
    ShaderVariantRegistry::Instance().Shutdown();
    ClearCaches();

    m_fgResourceManager = nullptr;
    m_shaderLoader = nullptr;
    m_initialized = false;

    Msg("* [MaterialSystem] Shutdown");
}

const MaterialSystem::MaterialInfo& MaterialSystem::GetMaterialInfo(const char* shaderName, const char* textureName)
{
    if (!shaderName || !shaderName[0])
        return m_defaultMaterialInfo;

    // Check cache first
    shared_str key(shaderName);
    auto it = m_materialCache.find(key);
    if (it != m_materialCache.end())
        return it->second;

    MaterialInfo info = GetDefaultMaterialInfo();

    // Try blender lookup first (most accurate - actual values from shaders.xr)
    using BlendMode = RENDER_NAMESPACE::CResourceManager::BlendMode;
    RENDER_NAMESPACE::CResourceManager::BlenderProperties blenderProps;
    if (RENDER_NAMESPACE::RImplementation.Resources->GetBlenderProperties(shaderName, blenderProps))
    {
        // Convert BlendMode to material properties
        switch (blenderProps.blendMode)
        {
        case BlendMode::Opaque:
            info.alphaTest = false;
            info.alphaRef = 0;
            info.transparent = false;
            break;
        case BlendMode::AlphaTest:
            info.alphaTest = true;
            info.alphaRef = blenderProps.alphaRef;
            info.transparent = false;
            break;
        case BlendMode::AlphaBlend:
        case BlendMode::Additive:
        case BlendMode::Multiply:
        case BlendMode::Multiply2X:
            info.alphaTest = false;
            info.alphaRef = 0;
            info.transparent = true;
            break;
        }
        if (blenderProps.strictB2F)
            info.transparent = true;
        m_stats.materialsFromBlender++;
    }

    auto& registry = ShaderVariantRegistry::Instance();
    u32 variantIdx = registry.GetVariantIndex(shaderName);
    if (variantIdx > 0)
    {
        const auto* variant = registry.GetVariantByIndex(variantIdx);
        info.shaderVariant = variantIdx;
        if (variant->transparent)
            info.transparent = true;
        if (!variant->passes.empty() && variant->passes[0].hasAlphaTestOverride)
        {
            info.alphaTest = true;
            info.alphaRef = variant->passes[0].alphaTestRef;
        }
    }

    m_materialCache[key] = info;
    return m_materialCache[key];
}

MaterialSystem::MaterialInfo MaterialSystem::GetDefaultMaterialInfo() const
{
    MaterialInfo info{};
    info.priority = 1;
    info.roughness = 0.5f;
    info.aoScale = 1.0f;
    return info;
}

void MaterialSystem::PreloadMaterialTextures(const char* textureName)
{
    if (!textureName || !textureName[0])
        return;

    if (!m_fgResourceManager)
    {
        Msg("! [MaterialSystem] Cannot preload textures - FGResourceManager not set");
        return;
    }

    // Check if already loaded
    shared_str key(textureName);
    if (m_textureCache.contains(key))
        return;

    TextureManager* texMgr = m_fgResourceManager->GetTextureManager();
    if (!texMgr)
    {
        Msg("! [MaterialSystem] TextureManager not available");
        return;
    }

    TextureSet set;

    // Load albedo (required)
    xr_string albedoPath = GetAlbedoPath(textureName);
    set.albedo = texMgr->LoadTexture(
        albedoPath.c_str(),
        TexturePriority::High
    );

    // Load normal map (optional - use default if not found)
    xr_string normalPath = GetNormalPath(textureName);
    set.normal = texMgr->LoadTexture(
        normalPath.c_str(),
        TexturePriority::Medium
    );

    // Load PBR map (optional)
    xr_string pbrPath = GetPBRPath(textureName);
    set.pbr = texMgr->LoadTexture(
        pbrPath.c_str(),
        TexturePriority::Medium
    );

    // Detail texture (only if needed - check material info)
    // For now, skip detail textures

    m_textureCache[key] = set;
    m_stats.textureSetsCached++;
}

const MaterialSystem::TextureSet& MaterialSystem::GetTextures(const char* textureName) const
{
    if (!textureName || !textureName[0])
        return m_emptyTextureSet;

    shared_str key(textureName);
    auto it = m_textureCache.find(key);
    if (it != m_textureCache.end())
        return it->second;

    return m_emptyTextureSet;
}

bool MaterialSystem::HasTextures(const char* textureName) const
{
    if (!textureName || !textureName[0])
        return false;

    shared_str key(textureName);
    return m_textureCache.contains(key);
}

void MaterialSystem::ClearCaches()
{
    m_materialCache.clear();
    m_textureCache.clear();

    m_stats = Stats{};

    Msg("* [MaterialSystem] Caches cleared");
}

MaterialSystem::Stats MaterialSystem::GetStats() const
{
    m_stats.materialsCached = static_cast<u32>(m_materialCache.size());
    m_stats.textureSetsCached = static_cast<u32>(m_textureCache.size());
    return m_stats;
}

// Texture path resolution helpers
xr_string MaterialSystem::GetAlbedoPath(const char* textureName) const
{
    // textureName is like "wood\wood1"
    // Return "textures\wood\wood1" (engine adds .dds)
    return textureName;
}

xr_string MaterialSystem::GetNormalPath(const char* textureName) const
{
    // Convention: texture_bump or texture_normal
    xr_string path = textureName;
    path += "_bump";
    return path;
}

xr_string MaterialSystem::GetPBRPath(const char* textureName) const
{
    // Convention: texture_pbr (packed R=AO, G=Roughness, B=Metallic)
    xr_string path = textureName;
    path += "_pbr";
    return path;
}

xr_string MaterialSystem::GetDetailPath(const char* textureName) const
{
    // Convention: texture_detail
    xr_string path = textureName;
    path += "_detail";
    return path;
}

} // namespace xray::render
