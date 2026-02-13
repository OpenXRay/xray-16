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

static nvrhi::BlendFactor ParseBlendFactor(const char* str)
{
    if (!str || !str[0]) return nvrhi::BlendFactor::One;
    if (!xr_strcmp(str, "Zero")) return nvrhi::BlendFactor::Zero;
    if (!xr_strcmp(str, "One")) return nvrhi::BlendFactor::One;
    if (!xr_strcmp(str, "SrcColor")) return nvrhi::BlendFactor::SrcColor;
    if (!xr_strcmp(str, "InvSrcColor")) return nvrhi::BlendFactor::InvSrcColor;
    if (!xr_strcmp(str, "SrcAlpha")) return nvrhi::BlendFactor::SrcAlpha;
    if (!xr_strcmp(str, "InvSrcAlpha")) return nvrhi::BlendFactor::InvSrcAlpha;
    if (!xr_strcmp(str, "DstAlpha")) return nvrhi::BlendFactor::DstAlpha;
    if (!xr_strcmp(str, "InvDstAlpha")) return nvrhi::BlendFactor::InvDstAlpha;
    if (!xr_strcmp(str, "DstColor")) return nvrhi::BlendFactor::DstColor;
    if (!xr_strcmp(str, "InvDstColor")) return nvrhi::BlendFactor::InvDstColor;
    if (!xr_strcmp(str, "SrcAlphaSaturate")) return nvrhi::BlendFactor::SrcAlphaSaturate;
    Msg("! [ShaderVariant] Unknown blend factor: '%s'", str);
    return nvrhi::BlendFactor::One;
}

static nvrhi::BlendOp ParseBlendOp(const char* str)
{
    if (!str || !str[0]) return nvrhi::BlendOp::Add;
    if (!xr_strcmp(str, "Add")) return nvrhi::BlendOp::Add;
    if (!xr_strcmp(str, "Subtract")) return nvrhi::BlendOp::Subtract;
    if (!xr_strcmp(str, "ReverseSubtract")) return nvrhi::BlendOp::ReverseSubtract;
    if (!xr_strcmp(str, "Min")) return nvrhi::BlendOp::Min;
    if (!xr_strcmp(str, "Max")) return nvrhi::BlendOp::Max;
    Msg("! [ShaderVariant] Unknown blend op: '%s'", str);
    return nvrhi::BlendOp::Add;
}

static nvrhi::ComparisonFunc ParseComparisonFunc(const char* str)
{
    if (!str || !str[0]) return nvrhi::ComparisonFunc::LessOrEqual;
    if (!xr_strcmp(str, "Never")) return nvrhi::ComparisonFunc::Never;
    if (!xr_strcmp(str, "Less")) return nvrhi::ComparisonFunc::Less;
    if (!xr_strcmp(str, "Equal")) return nvrhi::ComparisonFunc::Equal;
    if (!xr_strcmp(str, "LessOrEqual")) return nvrhi::ComparisonFunc::LessOrEqual;
    if (!xr_strcmp(str, "Greater")) return nvrhi::ComparisonFunc::Greater;
    if (!xr_strcmp(str, "NotEqual")) return nvrhi::ComparisonFunc::NotEqual;
    if (!xr_strcmp(str, "GreaterOrEqual")) return nvrhi::ComparisonFunc::GreaterOrEqual;
    if (!xr_strcmp(str, "Always")) return nvrhi::ComparisonFunc::Always;
    Msg("! [ShaderVariant] Unknown comparison func: '%s'", str);
    return nvrhi::ComparisonFunc::LessOrEqual;
}

static nvrhi::RasterCullMode ParseCullMode(const char* str)
{
    if (!str || !str[0]) return nvrhi::RasterCullMode::Back;
    if (!xr_strcmp(str, "Back")) return nvrhi::RasterCullMode::Back;
    if (!xr_strcmp(str, "Front")) return nvrhi::RasterCullMode::Front;
    if (!xr_strcmp(str, "None")) return nvrhi::RasterCullMode::None;
    Msg("! [ShaderVariant] Unknown cull mode: '%s'", str);
    return nvrhi::RasterCullMode::Back;
}

static nvrhi::StencilOp ParseStencilOp(const char* str)
{
    if (!str || !str[0]) return nvrhi::StencilOp::Keep;
    if (!xr_strcmp(str, "Keep")) return nvrhi::StencilOp::Keep;
    if (!xr_strcmp(str, "Zero")) return nvrhi::StencilOp::Zero;
    if (!xr_strcmp(str, "Replace")) return nvrhi::StencilOp::Replace;
    if (!xr_strcmp(str, "IncrementClamp")) return nvrhi::StencilOp::IncrementAndClamp;
    if (!xr_strcmp(str, "DecrementClamp")) return nvrhi::StencilOp::DecrementAndClamp;
    if (!xr_strcmp(str, "Invert")) return nvrhi::StencilOp::Invert;
    if (!xr_strcmp(str, "IncrementWrap")) return nvrhi::StencilOp::IncrementAndWrap;
    if (!xr_strcmp(str, "DecrementWrap")) return nvrhi::StencilOp::DecrementAndWrap;
    Msg("! [ShaderVariant] Unknown stencil op: '%s'", str);
    return nvrhi::StencilOp::Keep;
}

static nvrhi::ColorMask ParseColorMask(const char* str)
{
    if (!str || !str[0]) return nvrhi::ColorMask::All;
    uint8_t mask = 0;
    for (const char* c = str; *c; ++c)
    {
        switch (*c)
        {
        case 'R': case 'r': mask |= static_cast<uint8_t>(nvrhi::ColorMask::Red); break;
        case 'G': case 'g': mask |= static_cast<uint8_t>(nvrhi::ColorMask::Green); break;
        case 'B': case 'b': mask |= static_cast<uint8_t>(nvrhi::ColorMask::Blue); break;
        case 'A': case 'a': mask |= static_cast<uint8_t>(nvrhi::ColorMask::Alpha); break;
        }
    }
    return static_cast<nvrhi::ColorMask>(mask);
}

static void ParseBlendState(const JsonValue& blend, ShaderPassDesc& pass)
{
    if (!blend.is_object()) return;
    pass.blendEnabled = true;
    pass.blendRT.blendEnable = true;
    pass.blendRT.srcBlend = ParseBlendFactor(blend["src"].as_string("One"));
    pass.blendRT.destBlend = ParseBlendFactor(blend["dst"].as_string("Zero"));
    pass.blendRT.blendOp = ParseBlendOp(blend["op"].as_string("Add"));
    pass.blendRT.srcBlendAlpha = ParseBlendFactor(
        blend.has("srcAlpha") ? blend["srcAlpha"].as_string() : blend["src"].as_string("One"));
    pass.blendRT.destBlendAlpha = ParseBlendFactor(
        blend.has("dstAlpha") ? blend["dstAlpha"].as_string() : blend["dst"].as_string("Zero"));
    pass.blendRT.blendOpAlpha = ParseBlendOp(
        blend.has("opAlpha") ? blend["opAlpha"].as_string() : blend["op"].as_string("Add"));
}

static void ParseDepthState(const JsonValue& depth, ShaderPassDesc& pass)
{
    if (!depth.is_object()) return;
    pass.depthStencil.depthTestEnable = depth["test"].as_bool(true);
    pass.depthStencil.depthWriteEnable = depth["write"].as_bool(true);
    pass.depthStencil.depthFunc = ParseComparisonFunc(depth["func"].as_string("LessOrEqual"));
}

static void ParseStencilState(const JsonValue& stencil, ShaderPassDesc& pass)
{
    if (!stencil.is_object()) return;
    pass.depthStencil.stencilEnable = stencil["enabled"].as_bool(true);
    pass.depthStencil.stencilReadMask = static_cast<uint8_t>(stencil["readMask"].as_u32(0xFF));
    pass.depthStencil.stencilWriteMask = static_cast<uint8_t>(stencil["writeMask"].as_u32(0xFF));
    pass.depthStencil.stencilRefValue = static_cast<uint8_t>(stencil["ref"].as_u32(0));

    auto func = ParseComparisonFunc(stencil["func"].as_string("Always"));
    auto passOp = ParseStencilOp(stencil["passOp"].as_string("Keep"));
    auto failOp = ParseStencilOp(stencil["failOp"].as_string("Keep"));
    auto depthFailOp = ParseStencilOp(stencil["depthFailOp"].as_string("Keep"));

    pass.depthStencil.frontFaceStencil.stencilFunc = func;
    pass.depthStencil.frontFaceStencil.passOp = passOp;
    pass.depthStencil.frontFaceStencil.failOp = failOp;
    pass.depthStencil.frontFaceStencil.depthFailOp = depthFailOp;
    pass.depthStencil.backFaceStencil = pass.depthStencil.frontFaceStencil;
}

static void ParseRasterState(const JsonValue& raster, ShaderPassDesc& pass)
{
    if (!raster.is_object()) return;
    pass.rasterState.cullMode = ParseCullMode(raster["cull"].as_string("Back"));
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
    if (json.has("vs"))
        pass.vsName = json["vs"].as_string("bindless_forward");
    if (json.has("ps"))
        pass.psName = json["ps"].as_string("bindless_forward");
    if (json.has("name"))
        pass.name = json["name"].as_string();

    ParseBlendState(json["blend"], pass);
    ParseDepthState(json["depth"], pass);
    ParseStencilState(json["stencil"], pass);
    ParseRasterState(json["raster"], pass);

    if (json.has("colorWriteMask"))
        pass.colorWriteMask = ParseColorMask(json["colorWriteMask"].as_string("RGBA"));
    if (json.has("alphaToCoverage"))
        pass.alphaToCoverage = json["alphaToCoverage"].as_bool(false);
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
    desc.wmark = root["wmark"].as_bool(false);

    ParseTextures(root["textures"], desc.textures);

    if (root.has("passes") && root["passes"].is_array())
    {
        for (size_t i = 0; i < root["passes"].size(); ++i)
        {
            ShaderPassDesc pass;
            pass.vsName = "bindless_forward";
            pass.psName = "bindless_forward";
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
        pass.vsName = "bindless_forward";
        pass.psName = "bindless_forward";
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
    Msg("* [ShaderVariant] '%s' idx=%d ps=%s blend=%s transparent=%s",
        stored.name.c_str(), index,
        stored.passes.empty() ? "none" : stored.passes[0].psName.c_str(),
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
