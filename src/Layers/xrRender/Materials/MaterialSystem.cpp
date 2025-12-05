#include "stdafx.h"
#include "MaterialSystem.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

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

const MaterialSystem::MaterialInfo& MaterialSystem::GetMaterialInfo(const char* shaderName)
{
    if (!shaderName || !shaderName[0])
        return m_defaultMaterialInfo;

    shared_str key(shaderName);
    auto it = m_materialCache.find(key);
    if (it != m_materialCache.end())
        return it->second;

    MaterialInfo info = LoadFromMetadataFile(shaderName);

    if (!info.alphaTest && !info.transparent && info.priority == 1)
    {
        MaterialInfo reflectionInfo = InferFromShaderReflection(shaderName);
        if (reflectionInfo.alphaTest || reflectionInfo.transparent)
        {
            info = reflectionInfo;
        }
    }

    m_materialCache[key] = info;
    return m_materialCache[key];
}

MaterialSystem::MaterialInfo MaterialSystem::LoadFromMetadataFile(const char* shaderName)
{
    MaterialInfo info = GetDefaultMaterialInfo();

    string_path materialPath;
    xr_sprintf(materialPath, "materials\\%s.material", shaderName);

    string_path fullPath;
    if (!FS.exist(fullPath, "$game_data$", materialPath))
    {
        m_stats.materialsFromDefaults++;
        return info;
    }

    if (ParseMaterialFile(fullPath, info))
    {
        m_stats.materialsFromFiles++;
    }
    else
    {
        m_stats.materialsFromDefaults++;
    }

    return info;
}

bool MaterialSystem::ParseMaterialFile(const char* path, MaterialInfo& outInfo)
{
    IReader* reader = FS.r_open(path);
    if (!reader)
        return false;

    string256 line;
    while (!reader->eof())
    {
        reader->r_string(line, sizeof(line));

        if (line[0] == '\0' || line[0] == '#' || line[0] == ';')
            continue;

        if (line[0] == '[')
            continue;

        char* eq = strchr(line, '=');
        if (!eq)
            continue;

        *eq = '\0';
        const char* key = line;
        const char* value = eq + 1;

        while (*key == ' ' || *key == '\t') key++;
        while (*value == ' ' || *value == '\t') value++;

        if (xr_strcmp(key, "alpha_test") == 0)
        {
            outInfo.alphaTest = (xr_strcmp(value, "true") == 0 || xr_strcmp(value, "1") == 0);
        }
        else if (xr_strcmp(key, "transparent") == 0)
        {
            outInfo.transparent = (xr_strcmp(value, "true") == 0 || xr_strcmp(value, "1") == 0);
        }
        else if (xr_strcmp(key, "priority") == 0)
        {
            outInfo.priority = static_cast<u8>(atoi(value));
        }
        else if (xr_strcmp(key, "metallic") == 0)
        {
            outInfo.metallic = static_cast<float>(atof(value));
        }
        else if (xr_strcmp(key, "roughness") == 0)
        {
            outInfo.roughness = static_cast<float>(atof(value));
        }
        else if (xr_strcmp(key, "ao_scale") == 0)
        {
            outInfo.aoScale = static_cast<float>(atof(value));
        }
        else if (xr_strcmp(key, "use_parallax") == 0)
        {
            outInfo.useParallax = (xr_strcmp(value, "true") == 0 || xr_strcmp(value, "1") == 0);
        }
        else if (xr_strcmp(key, "use_detail") == 0)
        {
            outInfo.useDetail = (xr_strcmp(value, "true") == 0 || xr_strcmp(value, "1") == 0);
        }
    }

    FS.r_close(reader);
    return true;
}

MaterialSystem::MaterialInfo MaterialSystem::InferFromShaderReflection(const char* shaderName)
{
    MaterialInfo info = GetDefaultMaterialInfo();

    if (!m_shaderLoader)
        return info;

    auto psResult = m_shaderLoader->LoadPixelShader(shaderName);
    if (psResult.reflection)
    {

        const char* alphaPatterns[] = {
            "_alpha", "_aref", "_atest", "flora", "tree", "grass", "leaves"
        };

        xr_string lowerName = shaderName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

        for (const char* pattern : alphaPatterns)
        {
            if (lowerName.find(pattern) != xr_string::npos)
            {
                info.alphaTest = true;
                m_stats.materialsFromReflection++;
                return info;
            }
        }

        const char* transparentPatterns[] = {
            "glass", "water", "particle", "distort", "blend"
        };

        for (const char* pattern : transparentPatterns)
        {
            if (lowerName.find(pattern) != xr_string::npos)
            {
                info.transparent = true;
                m_stats.materialsFromReflection++;
                return info;
            }
        }
    }

    return info;
}

MaterialSystem::MaterialInfo MaterialSystem::GetDefaultMaterialInfo() const
{
    return MaterialInfo{
        false,
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
