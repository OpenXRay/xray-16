#include "stdafx.h"
#include "ShaderVariantRegistry.h"
#include "JsonParser.h"

namespace xray::render
{

ShaderVariantRegistry& ShaderVariantRegistry::Instance()
{
    static ShaderVariantRegistry instance;
    return instance;
}

static void ParseBlendState(const JsonValue& blend, ShaderPassDesc& pass)
{
    if (!blend.is_object()) return;
    pass.blendEnabled = true;
}

static void ParseTextures(const JsonValue& textures, xr_map<shared_str, shared_str>& out)
{
    if (!textures.is_object()) return;
    for (const auto& [key, val] : textures.objVal)
    {
        if (val.is_string())
            out[shared_str(key.c_str())] = shared_str(val.strVal.c_str());
    }
}

static void ParsePassFields(const JsonValue& json, ShaderPassDesc& pass)
{
    if (json.has("name"))
        pass.name = json["name"].as_string();

    ParseBlendState(json["blend"], pass);

    if (json.has("alphaTest"))
    {
        pass.hasAlphaTestOverride = true;
        pass.alphaTestRef = json["alphaTest"]["ref"].as_u32(0);
    }

    ParseTextures(json["textures"], pass.textures);
}

void ShaderVariantRegistry::LoadVariantFile(const char* filename, const char* fileData)
{
    JsonValue root;
    if (!ParseJson(fileData, root) || !root.is_object())
    {
        Msg("! [ShaderVariant] Failed to parse: %s", filename);
        return;
    }

    ShaderVariantDesc desc;

    xr_string baseName(filename);
    auto dotPos = baseName.find(".s.json");
    if (dotPos != xr_string::npos)
        baseName = baseName.substr(0, dotPos);
    desc.name = baseName.c_str();

    if (root.has("compute") && root["compute"].has("shader"))
        desc.csName = root["compute"]["shader"].as_string();

    desc.sortPriority = static_cast<u8>(root["sorting"]["priority"].as_u32(1));
    desc.backToFront = root["sorting"]["backToFront"].as_bool(false);
    desc.fog = root["fog"].as_bool(true);
    desc.distort = root["distort"].as_bool(false);
    desc.emissive = root["emissive"].as_bool(false);
    desc.castsShadow = root["castsShadow"].as_bool(true);
    desc.emissiveIntensity = root["emissiveIntensity"].as_float(1.0f);

    ParseTextures(root["textures"], desc.textures);

    if (root.has("passes") && root["passes"].is_array())
    {
        for (size_t i = 0; i < root["passes"].size(); ++i)
        {
            ShaderPassDesc pass;
            ParsePassFields(root["passes"][i], pass);

            for (const auto& [k, v] : desc.textures)
            {
                if (!pass.textures.contains(k))
                    pass.textures[k] = v;
            }
            desc.passes.push_back(std::move(pass));
        }
    }
    else
    {
        ShaderPassDesc pass;
        ParsePassFields(root, pass);
        desc.passes.push_back(std::move(pass));
    }

    desc.transparent = root["transparent"].as_bool(false);
    if (!desc.transparent && !desc.passes.empty() && desc.passes[0].blendEnabled)
        desc.transparent = true;

    u32 index = static_cast<u32>(m_variants.size());
    m_variants.push_back(std::move(desc));
    m_nameToVariantIndex[shared_str(baseName.c_str())] = index;

    const auto& stored = m_variants.back();
    Msg("* [ShaderVariant] '%s' idx=%d blend=%s transparent=%s",
        stored.name.c_str(), index,
        (!stored.passes.empty() && stored.passes[0].blendEnabled) ? "yes" : "no",
        stored.transparent ? "yes" : "no");
}

void ShaderVariantRegistry::Initialize()
{
    if (m_initialized)
        return;

    m_variants.clear();
    m_nameToVariantIndex.clear();

    ShaderVariantDesc defaultDesc;
    defaultDesc.name = "default";
    m_variants.push_back(std::move(defaultDesc));

    xr_vector<pstr>* files = FS.file_list_open("$game_shaders$", "r5" DELIMITER, FS_ListFiles);
    if (files)
    {
        for (const auto& file : *files)
        {
            if (!strstr(file, ".s.json"))
                continue;

            string_path fullPath;
            FS.update_path(fullPath, "$game_shaders$", "r5" DELIMITER);
            xr_strcat(fullPath, file);

            IReader* reader = FS.r_open(fullPath);
            if (!reader)
                continue;

            xr_string fileData(static_cast<const char*>(reader->pointer()), reader->length());
            FS.r_close(reader);

            LoadVariantFile(file, fileData.c_str());
        }
        FS.file_list_close(files);
    }

    m_initialized = true;
    Msg("* [ShaderVariant] Loaded %d variants (+ default)", static_cast<int>(m_variants.size()) - 1);
}

void ShaderVariantRegistry::Shutdown()
{
    m_variants.clear();
    m_nameToVariantIndex.clear();
    m_initialized = false;
}

const ShaderVariantDesc* ShaderVariantRegistry::GetVariant(const char* shaderName) const
{
    return GetVariantByIndex(GetVariantIndex(shaderName));
}

u32 ShaderVariantRegistry::GetVariantIndex(const char* shaderName) const
{
    if (!shaderName || !shaderName[0])
        return 0;
    string256 buf;
    xr_strcpy(buf, shaderName);
    for (char* p = buf; *p; ++p)
    {
        if (*p == '\\' || *p == '/')
            *p = '_';
    }
    auto it = m_nameToVariantIndex.find(shared_str(buf));
    return it != m_nameToVariantIndex.end() ? it->second : 0;
}

const ShaderVariantDesc* ShaderVariantRegistry::GetVariantByIndex(u32 index) const
{
    return index < m_variants.size() ? &m_variants[index] : nullptr;
}

} // namespace xray::render
