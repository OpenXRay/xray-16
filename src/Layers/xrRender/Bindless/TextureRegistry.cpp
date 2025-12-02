// xrRender/Bindless/TextureAtlas.cpp
#include "stdafx.h"
#include "TextureAtlas.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::RENDER_NAMESPACE::bindless {

// ═══════════════════════════════════════════════════════
//  TEXTURE REGISTRY IMPLEMENTATION
// ═══════════════════════════════════════════════════════

TextureRegistry::TextureRegistry(TextureType type)
    : m_type(type)
{
}

TextureRegistry::~TextureRegistry()
{
    Shutdown();
}

void TextureRegistry::Initialize(ng::RenderDevice* device)
{
    if (m_initialized)
        return;

    m_device = device;
    m_textures.reserve(256);  // Pre-allocate for typical usage
    CreatePlaceholder();
    m_initialized = true;

    Msg("* [Bindless] Initialized %s texture registry", TextureTypeName(m_type));
}

void TextureRegistry::Shutdown()
{
    m_textures.clear();
    m_textureMap.clear();
    m_placeholder = nullptr;
    m_initialized = false;
}

void TextureRegistry::CreatePlaceholder()
{
    if (!m_device)
        return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    // Create 1x1 placeholder texture
    // Diffuse: magenta (missing texture indicator)
    // Normal: flat normal (0.5, 0.5, 1.0)
    // PBR: black (no metallic/roughness)
    // Detail: gray
    u32 placeholderColor;
    switch (m_type) {
        case TextureType::Diffuse: placeholderColor = 0xFFFF00FF; break;  // Magenta
        case TextureType::Normal:  placeholderColor = 0xFFFF8080; break;  // Flat normal
        case TextureType::PBR:     placeholderColor = 0xFF000000; break;  // Black
        case TextureType::Detail:  placeholderColor = 0xFF808080; break;  // Gray
        default:                   placeholderColor = 0xFFFF00FF; break;
    }

    nvrhi::TextureDesc desc;
    desc.width = 1;
    desc.height = 1;
    desc.mipLevels = 1;
    desc.format = nvrhi::Format::RGBA8_UNORM;
    desc.dimension = nvrhi::TextureDimension::Texture2D;
    desc.isShaderResource = true;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    desc.debugName = xr_string("BindlessPlaceholder_") + TextureTypeName(m_type);

    m_placeholder = nvDevice->createTexture(desc);

    // Note: Initial data would need to be uploaded via command list
    // For now, the texture will be black/undefined which is fine for placeholders
}

u32 TextureRegistry::RegisterTexture(const shared_str& textureName, nvrhi::ITexture* texture)
{
    if (!m_initialized || !texture)
        return INVALID_TEXTURE_INDEX;

    // Check if already registered
    auto it = m_textureMap.find(textureName);
    if (it != m_textureMap.end())
        return it->second;

    // Check capacity
    if (m_textures.size() >= MAX_BINDLESS_TEXTURES) {
        Msg("! [Bindless] %s registry full (max %u textures)",
            TextureTypeName(m_type), MAX_BINDLESS_TEXTURES);
        return INVALID_TEXTURE_INDEX;
    }

    // Get texture info
    const auto& desc = texture->getDesc();

    // Add to registry
    u32 index = static_cast<u32>(m_textures.size());
    TextureEntry entry;
    entry.texture = texture;
    entry.name = textureName;
    entry.width = desc.width;
    entry.height = desc.height;
    entry.format = desc.format;

    m_textures.push_back(entry);
    m_textureMap[textureName] = index;

    // Log first few registrations
    static u32 s_logCount[4] = {0, 0, 0, 0};
    u32 typeIdx = static_cast<u32>(m_type);
    if (s_logCount[typeIdx] < 5) {
        s_logCount[typeIdx]++;
        Msg("* [Bindless] Registered %s[%u]: '%s' (%ux%u)",
            TextureTypeName(m_type), index, textureName.c_str(),
            desc.width, desc.height);
    }

    return index;
}

u32 TextureRegistry::GetTextureIndex(const shared_str& textureName) const
{
    auto it = m_textureMap.find(textureName);
    return (it != m_textureMap.end()) ? it->second : INVALID_TEXTURE_INDEX;
}

nvrhi::ITexture* TextureRegistry::GetTexture(u32 index) const
{
    if (index >= m_textures.size())
        return m_placeholder.Get();
    return m_textures[index].texture.Get();
}

// ═══════════════════════════════════════════════════════
//  TEXTURE ATLAS MANAGER IMPLEMENTATION
// ═══════════════════════════════════════════════════════

TextureAtlasManager::TextureAtlasManager()
    : m_registries{
        TextureRegistry(TextureType::Diffuse),
        TextureRegistry(TextureType::Normal),
        TextureRegistry(TextureType::Detail),
        TextureRegistry(TextureType::PBR)
    }
{
}

TextureAtlasManager::~TextureAtlasManager()
{
    Shutdown();
}

TextureAtlasManager& TextureAtlasManager::Instance()
{
    static TextureAtlasManager instance;
    return instance;
}

void TextureAtlasManager::Initialize(ng::RenderDevice* device)
{
    if (m_initialized)
        return;

    for (auto& registry : m_registries)
        registry.Initialize(device);

    m_initialized = true;
    Msg("* [Bindless] TextureAtlasManager initialized (4 texture arrays, up to %u textures each)",
        MAX_BINDLESS_TEXTURES);
}

void TextureAtlasManager::Shutdown()
{
    for (auto& registry : m_registries)
        registry.Shutdown();

    m_initialized = false;
}

nvrhi::BindingLayoutHandle TextureAtlasManager::CreateBindingLayout(nvrhi::IDevice* device) const
{
    // Bindless texture layout using texture arrays
    // Each texture type gets a single slot with array of textures
    //
    // b2: StaticGlobals (engine matrices)
    // b4: LightingConstants
    // b5: PerDrawConstants
    // t8: g_Materials (StructuredBuffer)
    // t10+: g_DiffuseTextures[N] - uses N consecutive slots starting at t10
    // After diffuse: g_NormalTextures[N]
    // After normal: g_DetailTextures[N]
    // After detail: g_PBRTextures[N]
    // s0: Linear sampler
    //
    // For initial implementation, limit to reasonable count per type
    constexpr u32 TEXTURES_PER_TYPE = 512;  // Practical limit for DX11

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::ConstantBuffer(2),         // b2 - StaticGlobals
        nvrhi::BindingLayoutItem::ConstantBuffer(4),         // b4 - LightingConstants
        nvrhi::BindingLayoutItem::ConstantBuffer(5),         // b5 - PerDrawConstants
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),   // t8 - g_Materials
        nvrhi::BindingLayoutItem::Sampler(0),                // s0 - Linear sampler
    };

    // Add texture slots for each type
    // t10-t521: Diffuse (512 textures)
    // t522-t1033: Normal (512 textures)
    // t1034-t1545: Detail (512 textures)
    // t1546-t2057: PBR (512 textures)
    u32 baseSlot = 10;
    for (u32 type = 0; type < 4; type++) {
        for (u32 i = 0; i < TEXTURES_PER_TYPE; i++) {
            layoutDesc.bindings.push_back(
                nvrhi::BindingLayoutItem::Texture_SRV(baseSlot + i)
            );
        }
        baseSlot += TEXTURES_PER_TYPE;
    }

    return device->createBindingLayout(layoutDesc);
}

nvrhi::BindingSetHandle TextureAtlasManager::CreateBindingSet(
    nvrhi::IDevice* device,
    nvrhi::IBindingLayout* layout,
    nvrhi::IBuffer* materialBuffer,
    nvrhi::ISampler* sampler) const
{
    constexpr u32 TEXTURES_PER_TYPE = 512;

    nvrhi::BindingSetDesc bindDesc;
    bindDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(2, nullptr),  // Filled per-frame
        nvrhi::BindingSetItem::ConstantBuffer(4, nullptr),  // Filled per-frame
        nvrhi::BindingSetItem::ConstantBuffer(5, nullptr),  // Filled per-draw
        nvrhi::BindingSetItem::StructuredBuffer_SRV(8, materialBuffer),
        nvrhi::BindingSetItem::Sampler(0, sampler),
    };

    // Add textures for each type
    u32 baseSlot = 10;
    for (u32 typeIdx = 0; typeIdx < 4; typeIdx++) {
        const auto& textures = m_registries[typeIdx].GetTextures();
        nvrhi::ITexture* placeholder = m_registries[typeIdx].GetPlaceholder();

        for (u32 i = 0; i < TEXTURES_PER_TYPE; i++) {
            nvrhi::ITexture* tex = (i < textures.size() && textures[i].IsValid())
                ? textures[i].texture.Get()
                : placeholder;
            bindDesc.bindings.push_back(
                nvrhi::BindingSetItem::Texture_SRV(baseSlot + i, tex)
            );
        }
        baseSlot += TEXTURES_PER_TYPE;
    }

    return device->createBindingSet(bindDesc, layout);
}

} // namespace xray::render::RENDER_NAMESPACE::bindless
