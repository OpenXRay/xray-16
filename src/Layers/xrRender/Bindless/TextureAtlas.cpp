// xrRender/Bindless/TextureAtlas.cpp
// Simplified texture atlas - one array per type, all same size
#include "stdafx.h"
#include "TextureAtlas.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"

namespace xray::render::RENDER_NAMESPACE::bindless {

// ═══════════════════════════════════════════════════════
//  TEXTURE ATLAS IMPLEMENTATION
// ═══════════════════════════════════════════════════════

TextureAtlas::TextureAtlas(AtlasType type)
    : m_type(type)
{
}

TextureAtlas::~TextureAtlas()
{
    Shutdown();
}

void TextureAtlas::Initialize(ng::RenderDevice* device)
{
    if (m_initialized)
        return;

    m_device = device;
    m_initialized = true;

    Msg("* [BindlessAtlas] Initialized %s atlas (%ux%u, RGBA8)",
        GetAtlasTypeName(m_type), ATLAS_TEXTURE_SIZE, ATLAS_TEXTURE_SIZE);
}

void TextureAtlas::Shutdown()
{
    m_array = nullptr;
    m_currentSlice = 0;
    m_arrayCapacity = 0;
    m_textureMap.clear();
    m_initialized = false;
}

TextureRegisterResult TextureAtlas::RegisterTexture(
    ng::RenderContext* ctx,
    const shared_str& textureName,
    nvrhi::ITexture* sourceTexture)
{
    TextureRegisterResult result;

    if (!m_initialized || !sourceTexture || !textureName.size())
        return result;

    // Check if already registered
    auto it = m_textureMap.find(textureName);
    if (it != m_textureMap.end()) {
        const auto& cached = it->second;
        result.sliceRef = TextureSliceRef(m_type, cached.sliceIndex);
        result.uvScaleU = cached.uvScaleU;
        result.uvScaleV = cached.uvScaleV;
        return result;
    }

    // Check capacity
    if (m_currentSlice >= MAX_TEXTURES_PER_ATLAS) {
        Msg("! [BindlessAtlas] %s atlas is full (%u textures)",
            GetAtlasTypeName(m_type), MAX_TEXTURES_PER_ATLAS);
        return result;
    }

    // Ensure array has capacity
    EnsureCapacity(m_currentSlice + 1);

    if (!m_array) {
        Msg("! [BindlessAtlas] Failed to create %s array", GetAtlasTypeName(m_type));
        return result;
    }

    // Copy texture to slice
    float uvScaleU = 1.0f, uvScaleV = 1.0f;
    u32 sliceIndex = m_currentSlice;

    if (!CopyTextureToSlice(ctx, sourceTexture, sliceIndex, uvScaleU, uvScaleV)) {
        return result;
    }

    // Success - commit allocation
    m_currentSlice++;
    m_textureMap[textureName] = CachedTextureInfo{ sliceIndex, uvScaleU, uvScaleV };

    result.sliceRef = TextureSliceRef(m_type, sliceIndex);
    result.uvScaleU = uvScaleU;
    result.uvScaleV = uvScaleV;

    // Log first few registrations
    static u32 s_logCount[TOTAL_ATLAS_COUNT] = {0};
    u32 typeIdx = static_cast<u32>(m_type);
    if (s_logCount[typeIdx] < 3) {
        s_logCount[typeIdx]++;
        const auto& desc = sourceTexture->getDesc();
    }

    return result;
}

u32 TextureAtlas::GetTextureSlice(const shared_str& textureName) const
{
    auto it = m_textureMap.find(textureName);
    return it != m_textureMap.end() ? it->second.sliceIndex : UINT32_MAX;
}

void TextureAtlas::EnsureCapacity(u32 requiredSlices)
{
    // Already have the array - no resize needed (we allocate full capacity upfront)
    if (m_array)
        return;

    // Allocate reasonable capacity upfront to avoid resize issues
    // 512 slices × 1024×1024 × 4 bytes = 2 GB per atlas (reasonable for most levels)
    // If a level needs more, we'll log a warning when hitting capacity
    constexpr u32 INITIAL_CAPACITY = 512;

    nvrhi::TextureDesc desc;
    desc.width = ATLAS_TEXTURE_SIZE;
    desc.height = ATLAS_TEXTURE_SIZE;
    desc.arraySize = INITIAL_CAPACITY;
    desc.dimension = nvrhi::TextureDimension::Texture2DArray;
    desc.format = nvrhi::Format::RGBA8_UNORM;
    desc.mipLevels = 1;
    desc.sampleCount = 1;
    desc.isShaderResource = true;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;

    string256 debugName;
    xr_sprintf(debugName, "BindlessAtlas_%s", GetAtlasTypeName(m_type));
    desc.debugName = debugName;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    m_array = nvDevice->createTexture(desc);

    if (!m_array) {
        Msg("! [BindlessAtlas] Failed to create %s Texture2DArray (%ux%u x %u slices)",
            GetAtlasTypeName(m_type), ATLAS_TEXTURE_SIZE, ATLAS_TEXTURE_SIZE, INITIAL_CAPACITY);
        return;
    }

    m_arrayCapacity = INITIAL_CAPACITY;

    Msg("* [BindlessAtlas] Created %s array: %ux%u x %u slices (RGBA8)",
        GetAtlasTypeName(m_type), ATLAS_TEXTURE_SIZE, ATLAS_TEXTURE_SIZE, INITIAL_CAPACITY);
}

bool TextureAtlas::CopyTextureToSlice(
    ng::RenderContext* ctx,
    nvrhi::ITexture* sourceTexture,
    u32 sliceIndex,
    float& outUVScaleU,
    float& outUVScaleV)
{
    if (!m_array || !sourceTexture)
        return false;

    const auto& srcDesc = sourceTexture->getDesc();

    // Check if format conversion is needed
    // For now, only support RGBA8 source textures
    // TODO: Add compute shader for BC1/BC3/BC5 decompression
    if (srcDesc.format != nvrhi::Format::RGBA8_UNORM &&
        srcDesc.format != nvrhi::Format::SRGBA8_UNORM &&
        srcDesc.format != nvrhi::Format::BGRA8_UNORM &&
        srcDesc.format != nvrhi::Format::SBGRA8_UNORM)
    {
        // Track how many textures we're skipping due to format
        static u32 s_skippedCount = 0;
        s_skippedCount++;

        // Log first few and periodically after
        if (s_skippedCount <= 5 || (s_skippedCount % 100 == 0)) {
            Msg("! [BindlessAtlas] Skipping compressed texture (format %d) - total skipped: %u",
                static_cast<int>(srcDesc.format), s_skippedCount);
        }
        return false;
    }

    // Validate size
    if (srcDesc.width > ATLAS_TEXTURE_SIZE || srcDesc.height > ATLAS_TEXTURE_SIZE) {
        Msg("! [BindlessAtlas] Texture %ux%u exceeds atlas size %u",
            srcDesc.width, srcDesc.height, ATLAS_TEXTURE_SIZE);
        return false;
    }

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();

    // Calculate how many times to tile the texture to fill the atlas slot
    // This ensures seamless filtering at edges - when linear filter samples
    // beyond the texture boundary, it gets valid texture data, not black padding
    u32 tilesX = ATLAS_TEXTURE_SIZE / srcDesc.width;
    u32 tilesY = ATLAS_TEXTURE_SIZE / srcDesc.height;

    // UV scale is a DIVISOR: shader uses frac(uv) * uvScale
    // For a 64x64 texture in 1024x1024 atlas: uvScale = 0.0625
    // This maps UV [0,1] to atlas region [0, 0.0625]
    // Tiling ensures filtering at edge samples valid data from adjacent tile
    outUVScaleU = static_cast<float>(srcDesc.width) / static_cast<float>(ATLAS_TEXTURE_SIZE);
    outUVScaleV = static_cast<float>(srcDesc.height) / static_cast<float>(ATLAS_TEXTURE_SIZE);

    // Source slice (entire source texture)
    nvrhi::TextureSlice srcSlice;
    srcSlice.mipLevel = 0;
    srcSlice.arraySlice = 0;
    srcSlice.x = 0;
    srcSlice.y = 0;
    srcSlice.z = 0;
    srcSlice.width = srcDesc.width;
    srcSlice.height = srcDesc.height;
    srcSlice.depth = 1;

    // Tile the texture to fill the entire atlas slot
    // This ensures hardware wrap mode works correctly
    for (u32 ty = 0; ty < tilesY; ty++) {
        for (u32 tx = 0; tx < tilesX; tx++) {
            nvrhi::TextureSlice dstSlice;
            dstSlice.mipLevel = 0;
            dstSlice.arraySlice = sliceIndex;
            dstSlice.x = tx * srcDesc.width;
            dstSlice.y = ty * srcDesc.height;
            dstSlice.z = 0;
            dstSlice.width = srcDesc.width;
            dstSlice.height = srcDesc.height;
            dstSlice.depth = 1;

            cmdList->copyTexture(m_array, dstSlice, sourceTexture, srcSlice);
        }
    }

    return true;
}

// ═══════════════════════════════════════════════════════
//  TEXTURE ATLAS MANAGER IMPLEMENTATION
// ═══════════════════════════════════════════════════════

TextureAtlasManager::TextureAtlasManager()
    : m_atlases{
        TextureAtlas(AtlasType::Diffuse),
        TextureAtlas(AtlasType::Normal),
        TextureAtlas(AtlasType::Detail),
        TextureAtlas(AtlasType::PBR)
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

    for (auto& atlas : m_atlases)
        atlas.Initialize(device);

    m_initialized = true;
    Msg("* [BindlessAtlas] Manager initialized - 4 atlases (%ux%u each)",
        ATLAS_TEXTURE_SIZE, ATLAS_TEXTURE_SIZE);
}

void TextureAtlasManager::Shutdown()
{
    for (auto& atlas : m_atlases)
        atlas.Shutdown();

    m_initialized = false;
}

} // namespace xray::render::RENDER_NAMESPACE::bindless
