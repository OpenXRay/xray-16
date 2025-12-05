#include "stdafx.h"
#include "MaterialSystem.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

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

    // Check cache first
    shared_str key(shaderName);
    auto it = m_materialCache.find(key);
    if (it != m_materialCache.end())
        return it->second;

    // Try loading from .material file
    MaterialInfo info = LoadFromMetadataFile(shaderName);

    // If no file, try shader reflection
    if (!info.alphaTest && !info.transparent && info.priority == 1)
    {
        MaterialInfo reflectionInfo = InferFromShaderReflection(shaderName);
        if (reflectionInfo.alphaTest || reflectionInfo.transparent)
        {
            info = reflectionInfo;
        }
    }

    // Cache and return
    m_materialCache[key] = info;
    return m_materialCache[key];
}

MaterialSystem::MaterialInfo MaterialSystem::LoadFromMetadataFile(const char* shaderName)
{
    MaterialInfo info = GetDefaultMaterialInfo();

    // Build path to .material file
    // shaderName is like "models\model" or "effects\wallmark"
    // We look for "res/gamedata/materials/models/model.material"
    string_path materialPath;
    xr_sprintf(materialPath, "materials\\%s.material", shaderName);

    // Check if file exists
    string_path fullPath;
    if (!FS.exist(fullPath, "$game_data$", materialPath))
    {
        // No material file - use defaults
        m_stats.materialsFromDefaults++;
        return info;
    }

    // Parse the material file
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

    // Simple key=value parser
    // Format:
    // alpha_test=true
    // transparent=false
    // priority=1
    // metallic=0.0
    // roughness=0.5

    string256 line;
    while (!reader->eof())
    {
        reader->r_string(line, sizeof(line));

        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';')
            continue;

        // Skip section headers [material]
        if (line[0] == '[')
            continue;

        // Parse key=value
        char* eq = strchr(line, '=');
        if (!eq)
            continue;

        *eq = '\0';
        const char* key = line;
        const char* value = eq + 1;

        // Trim whitespace
        while (*key == ' ' || *key == '\t') key++;
        while (*value == ' ' || *value == '\t') value++;

        // Parse known keys
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

    // Try to load the pixel shader and check reflection for discard usage
    // ShaderLoader caches results, so this is efficient
    auto psResult = m_shaderLoader->LoadPixelShader(shaderName);
    if (psResult.reflection)
    {
        // Check if shader uses discard/clip
        // This would be detected during Slang compilation
        // For now, we check the shader name for common patterns as a fallback
        // TODO: Add proper usesDiscard flag to ExtractedReflection

        // Common alpha-test shader name patterns
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

        // Common transparent shader patterns
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
        false,  // alphaTest
        false,  // transparent
        1,      // priority
        0.0f,   // metallic
        0.5f,   // roughness
        1.0f,   // aoScale
        false,  // useParallax
        false   // useDetail
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
