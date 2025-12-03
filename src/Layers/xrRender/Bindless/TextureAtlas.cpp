// xrRender/Bindless/TextureAtlas.cpp
// 2D Grid-Packing Texture Atlas Implementation
// Based on DynamicIconAtlas cell-based allocation from yohji/dev branch
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

    Msg("* [BindlessAtlas] Initialized %s atlas (grid-packing: %ux%u pages, %upx cells)",
        GetAtlasTypeName(m_type), ATLAS_PAGE_SIZE, ATLAS_PAGE_SIZE, ATLAS_CELL_SIZE);
}

void TextureAtlas::Shutdown()
{
    m_array = nullptr;
    m_arrayCapacity = 0;
    m_pages.clear();
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
        result.allocation = it->second;
        return result;
    }

    const auto& srcDesc = sourceTexture->getDesc();

    // Check format - only RGBA8 variants supported
    if (srcDesc.format != nvrhi::Format::RGBA8_UNORM &&
        srcDesc.format != nvrhi::Format::SRGBA8_UNORM &&
        srcDesc.format != nvrhi::Format::BGRA8_UNORM &&
        srcDesc.format != nvrhi::Format::SBGRA8_UNORM)
    {
        static u32 s_skippedCount = 0;
        s_skippedCount++;
        if (s_skippedCount <= 5 || (s_skippedCount % 100 == 0)) {
            Msg("! [BindlessAtlas] Skipping compressed texture (format %d) - total skipped: %u",
                static_cast<int>(srcDesc.format), s_skippedCount);
        }
        return result;
    }

    // Check size - must fit in a single page
    if (srcDesc.width > ATLAS_PAGE_SIZE || srcDesc.height > ATLAS_PAGE_SIZE) {
        Msg("! [BindlessAtlas] Texture %ux%u exceeds page size %u",
            srcDesc.width, srcDesc.height, ATLAS_PAGE_SIZE);
        return result;
    }

    // Allocate space using grid packing
    AtlasAllocation allocation = Allocate(srcDesc.width, srcDesc.height);
    if (!allocation.IsValid()) {
        Msg("! [BindlessAtlas] Failed to allocate space for %ux%u texture in %s atlas",
            srcDesc.width, srcDesc.height, GetAtlasTypeName(m_type));
        return result;
    }

    // Copy texture data to allocated region
    if (!CopyTextureToRegion(ctx, sourceTexture, allocation.pageIndex,
                              allocation.GetPixelX(), allocation.GetPixelY())) {
        return result;
    }

    // Success - cache allocation
    m_textureMap[textureName] = allocation;
    result.allocation = allocation;

    // Log first few registrations per atlas type
    static u32 s_logCount[TOTAL_ATLAS_COUNT] = {0};
    u32 typeIdx = static_cast<u32>(m_type);
    if (s_logCount[typeIdx] < 3) {
        s_logCount[typeIdx]++;
        Msg("* [BindlessAtlas] %s: Registered %ux%u at page %u, cell (%u,%u) size (%ux%u cells)",
            GetAtlasTypeName(m_type), srcDesc.width, srcDesc.height,
            allocation.pageIndex, allocation.cellX, allocation.cellY,
            allocation.cellsWide, allocation.cellsTall);
    }

    return result;
}

AtlasAllocation TextureAtlas::GetTextureAllocation(const shared_str& textureName) const
{
    auto it = m_textureMap.find(textureName);
    return it != m_textureMap.end() ? it->second : AtlasAllocation();
}

AtlasAllocation TextureAtlas::Allocate(u32 textureWidth, u32 textureHeight)
{
    // Calculate cells needed (round up to cell boundaries)
    u32 cellsWide = (textureWidth + ATLAS_CELL_SIZE - 1) / ATLAS_CELL_SIZE;
    u32 cellsTall = (textureHeight + ATLAS_CELL_SIZE - 1) / ATLAS_CELL_SIZE;

    // Clamp to valid range (1-32 cells)
    cellsWide = std::clamp(cellsWide, 1u, ATLAS_GRID_SIZE);
    cellsTall = std::clamp(cellsTall, 1u, ATLAS_GRID_SIZE);

    u32 cellsNeeded = cellsWide * cellsTall;

    // Try existing pages first (bottom-left first-fit scan)
    for (u32 pageIdx = 0; pageIdx < m_pages.size(); ++pageIdx) {
        u32 cellX, cellY;
        if (TryAllocateInPage(pageIdx, cellsWide, cellsTall, cellX, cellY) != UINT32_MAX) {
            return AtlasAllocation(
                static_cast<u16>(pageIdx),
                static_cast<u16>(cellX),
                static_cast<u16>(cellY),
                static_cast<u8>(cellsWide),
                static_cast<u8>(cellsTall)
            );
        }
    }

    // No space in existing pages - add new page
    if (!AddPage()) {
        return AtlasAllocation();  // Failed to add page
    }

    // Allocate in new page (guaranteed to succeed for valid sizes)
    u32 newPageIdx = static_cast<u32>(m_pages.size()) - 1;
    u32 cellX, cellY;
    if (TryAllocateInPage(newPageIdx, cellsWide, cellsTall, cellX, cellY) != UINT32_MAX) {
        return AtlasAllocation(
            static_cast<u16>(newPageIdx),
            static_cast<u16>(cellX),
            static_cast<u16>(cellY),
            static_cast<u8>(cellsWide),
            static_cast<u8>(cellsTall)
        );
    }

    return AtlasAllocation();  // Should never reach here
}

u32 TextureAtlas::TryAllocateInPage(u32 pageIndex, u32 cellsWide, u32 cellsTall, u32& outCellX, u32& outCellY)
{
    if (pageIndex >= m_pages.size())
        return UINT32_MAX;

    AtlasPage& page = m_pages[pageIndex];

    // Quick reject if not enough free cells
    if (!page.HasSpace(cellsWide * cellsTall))
        return UINT32_MAX;

    // Bottom-left first-fit scan (same as DynamicIconAtlas)
    // Scan rows from bottom to top, columns from left to right
    for (u32 y = 0; y <= ATLAS_GRID_SIZE - cellsTall; ++y) {
        for (u32 x = 0; x <= ATLAS_GRID_SIZE - cellsWide; ++x) {
            if (page.CanPlace(x, y, cellsWide, cellsTall)) {
                page.MarkOccupied(x, y, cellsWide, cellsTall);
                outCellX = x;
                outCellY = y;
                return pageIndex;
            }
        }
    }

    return UINT32_MAX;
}

bool TextureAtlas::AddPage()
{
    if (m_pages.size() >= MAX_PAGES_PER_ATLAS) {
        Msg("! [BindlessAtlas] %s atlas reached max pages (%u)",
            GetAtlasTypeName(m_type), MAX_PAGES_PER_ATLAS);
        return false;
    }

    u32 requiredPages = static_cast<u32>(m_pages.size()) + 1;
    EnsurePageCapacity(requiredPages);

    if (!m_array)
        return false;

    // Add new page tracking
    m_pages.emplace_back();

    Msg("* [BindlessAtlas] %s: Added page %u (total: %u pages, %.1f MB)",
        GetAtlasTypeName(m_type), requiredPages - 1, requiredPages,
        (requiredPages * ATLAS_PAGE_SIZE * ATLAS_PAGE_SIZE * 4) / (1024.0f * 1024.0f));

    return true;
}

void TextureAtlas::EnsurePageCapacity(u32 requiredPages)
{
    if (m_array && m_arrayCapacity >= requiredPages)
        return;

    // Calculate new capacity (grow by doubling, minimum 4 pages)
    u32 newCapacity = m_arrayCapacity > 0 ? m_arrayCapacity : 4;
    while (newCapacity < requiredPages)
        newCapacity *= 2;
    newCapacity = std::min(newCapacity, MAX_PAGES_PER_ATLAS);

    nvrhi::TextureDesc desc;
    desc.width = ATLAS_PAGE_SIZE;
    desc.height = ATLAS_PAGE_SIZE;
    desc.arraySize = newCapacity;
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
    nvrhi::TextureHandle newArray = nvDevice->createTexture(desc);

    if (!newArray) {
        Msg("! [BindlessAtlas] Failed to create %s Texture2DArray (%ux%u x %u pages)",
            GetAtlasTypeName(m_type), ATLAS_PAGE_SIZE, ATLAS_PAGE_SIZE, newCapacity);
        return;
    }

    // TODO: If we had existing data, we'd need to copy it here
    // For now, we only grow the array before adding new pages

    m_array = newArray;
    m_arrayCapacity = newCapacity;

    Msg("* [BindlessAtlas] %s: Allocated array with %u page capacity (%.1f MB max)",
        GetAtlasTypeName(m_type), newCapacity,
        (newCapacity * ATLAS_PAGE_SIZE * ATLAS_PAGE_SIZE * 4) / (1024.0f * 1024.0f));
}

bool TextureAtlas::CopyTextureToRegion(
    ng::RenderContext* ctx,
    nvrhi::ITexture* sourceTexture,
    u32 pageIndex,
    u32 pixelX,
    u32 pixelY)
{
    if (!m_array || !sourceTexture)
        return false;

    const auto& srcDesc = sourceTexture->getDesc();
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();

    // Source: entire source texture
    nvrhi::TextureSlice srcSlice;
    srcSlice.mipLevel = 0;
    srcSlice.arraySlice = 0;
    srcSlice.x = 0;
    srcSlice.y = 0;
    srcSlice.z = 0;
    srcSlice.width = srcDesc.width;
    srcSlice.height = srcDesc.height;
    srcSlice.depth = 1;

    // Destination: specific region in page
    nvrhi::TextureSlice dstSlice;
    dstSlice.mipLevel = 0;
    dstSlice.arraySlice = pageIndex;
    dstSlice.x = pixelX;
    dstSlice.y = pixelY;
    dstSlice.z = 0;
    dstSlice.width = srcDesc.width;
    dstSlice.height = srcDesc.height;
    dstSlice.depth = 1;

    cmdList->copyTexture(m_array, dstSlice, sourceTexture, srcSlice);

    return true;
}

u64 TextureAtlas::GetAllocatedMemoryBytes() const
{
    // Each page is ATLAS_PAGE_SIZE × ATLAS_PAGE_SIZE × 4 bytes (RGBA8)
    return static_cast<u64>(m_pages.size()) * ATLAS_PAGE_SIZE * ATLAS_PAGE_SIZE * 4;
}

u64 TextureAtlas::GetUsedCellCount() const
{
    u64 usedCells = 0;
    for (const auto& page : m_pages) {
        usedCells += (ATLAS_CELLS_PER_PAGE - page.freeCount);
    }
    return usedCells;
}

float TextureAtlas::GetUtilization() const
{
    if (m_pages.empty())
        return 0.0f;
    u64 totalCells = static_cast<u64>(m_pages.size()) * ATLAS_CELLS_PER_PAGE;
    return static_cast<float>(GetUsedCellCount()) / static_cast<float>(totalCells);
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
    Msg("* [BindlessAtlas] Manager initialized - 4 atlases (grid-packing, %ux%u pages, %upx cells)",
        ATLAS_PAGE_SIZE, ATLAS_PAGE_SIZE, ATLAS_CELL_SIZE);
}

void TextureAtlasManager::Shutdown()
{
    for (auto& atlas : m_atlases)
        atlas.Shutdown();

    m_initialized = false;
}

void TextureAtlasManager::LogMemoryUsage() const
{
    u64 totalAllocated = 0;
    u64 totalTextures = 0;

    Msg("* [BindlessAtlas] Memory Usage:");
    for (u32 i = 0; i < TOTAL_ATLAS_COUNT; ++i) {
        const auto& atlas = m_atlases[i];
        u64 allocated = atlas.GetAllocatedMemoryBytes();
        totalAllocated += allocated;
        totalTextures += atlas.GetTextureCount();

        Msg("*   %s: %u textures, %u pages, %.1f MB (%.1f%% utilization)",
            GetAtlasTypeName(static_cast<AtlasType>(i)),
            atlas.GetTextureCount(),
            atlas.GetPageCount(),
            allocated / (1024.0f * 1024.0f),
            atlas.GetUtilization() * 100.0f);
    }

    Msg("*   Total: %llu textures, %.1f MB allocated",
        totalTextures, totalAllocated / (1024.0f * 1024.0f));
}

} // namespace xray::render::RENDER_NAMESPACE::bindless
