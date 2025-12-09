#include "stdafx.h"
#include "MaterialSystem.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/ResourceManager.h"

using namespace xray::render::resources;
using namespace xray::render::framegraph;

namespace xray::render
{

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

    m_defaultMaterialInfo = MaterialInfo{};
    m_emptyTextureSet = TextureSet{};

    m_initialized = true;
    Msg("* [MaterialSystem] Initialized");
}

void MaterialSystem::Shutdown()
{
    if (!m_initialized)
        return;

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

    shared_str key(shaderName);
    auto it = m_materialCache.find(key);
    if (it != m_materialCache.end())
        return it->second;

    MaterialInfo info = GetDefaultMaterialInfo();

    RENDER_NAMESPACE::CResourceManager::BlenderProperties blenderProps;
    if (RENDER_NAMESPACE::RImplementation.Resources->GetBlenderProperties(shaderName, blenderProps))
    {
        info.alphaTest = blenderProps.alphaTest || blenderProps.alphaBlend;
        info.alphaRef = blenderProps.alphaBlend ? 127 : blenderProps.alphaRef;
        info.transparent = false;
        m_stats.materialsFromBlender++;

        if (blenderProps.alphaBlend)
            Msg("* [MaterialSystem] '%s - %s': from blender -> alphaTest=%d, alphaRef=%d, alphaBlend=%d",
            shaderName, textureName, info.alphaTest, info.alphaRef, blenderProps.alphaBlend);

        m_materialCache[key] = info;
        return m_materialCache[key];
    }

    m_materialCache[key] = info;
    return m_materialCache[key];
}

MaterialSystem::MaterialInfo MaterialSystem::GetDefaultMaterialInfo() const
{
    return MaterialInfo{
        false,
        0,
        false,
        1,
        0.0f,
        0.5f,
        1.0f,
        false,
        false
    };
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

    xr_string albedoPath = GetAlbedoPath(textureName);
    set.albedo = texMgr->LoadTexture(
        albedoPath.c_str(),
        TexturePriority::High
    );

    xr_string normalPath = GetNormalPath(textureName);
    set.normal = texMgr->LoadTexture(
        normalPath.c_str(),
        TexturePriority::Medium
    );

    xr_string pbrPath = GetPBRPath(textureName);
    set.pbr = texMgr->LoadTexture(
        pbrPath.c_str(),
        TexturePriority::Medium
    );

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

xr_string MaterialSystem::GetAlbedoPath(const char* textureName) const
{
    return textureName;
}

xr_string MaterialSystem::GetNormalPath(const char* textureName) const
{
    xr_string path = textureName;
    path += "_bump";
    return path;
}

xr_string MaterialSystem::GetPBRPath(const char* textureName) const
{
    xr_string path = textureName;
    path += "_pbr";
    return path;
}

xr_string MaterialSystem::GetDetailPath(const char* textureName) const
{
    xr_string path = textureName;
    path += "_detail";
    return path;
}

}
