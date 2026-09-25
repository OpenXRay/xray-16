#include "stdafx.h"
#include "TextureManager.h"
#include "DDSLoader.h"
#include "../RenderContext/RenderDevice.h"
#include "FGResourceManager.h"

namespace xray::render::resources {

using namespace xray::render::fg;

u64 TextureDesc::CalculateMemorySize(u32 startMip, u32 mipCount) const {
    if (mipCount == 0) {
        mipCount = mipLevels;
    }

    u64 totalSize = 0;
    u32 mipWidth = width;
    u32 mipHeight = height;

    for (u32 i = 0; i < startMip && i < mipLevels; i++) {
        mipWidth = std::max(1u, mipWidth / 2);
        mipHeight = std::max(1u, mipHeight / 2);
    }

    for (u32 i = 0; i < mipCount && (startMip + i) < mipLevels; i++) {
        u32 w = std::max(1u, mipWidth);
        u32 h = std::max(1u, mipHeight);

        u64 mipSize = 0;

        switch (format) {
            case nvrhi::Format::BC1_UNORM:
            case nvrhi::Format::BC1_UNORM_SRGB:
                mipSize = ((w + 3) / 4) * ((h + 3) / 4) * 8;
                break;

            case nvrhi::Format::BC2_UNORM:
            case nvrhi::Format::BC2_UNORM_SRGB:
            case nvrhi::Format::BC3_UNORM:
            case nvrhi::Format::BC3_UNORM_SRGB:
            case nvrhi::Format::BC5_UNORM:
            case nvrhi::Format::BC5_SNORM:
                mipSize = ((w + 3) / 4) * ((h + 3) / 4) * 16;
                break;

            case nvrhi::Format::BC4_UNORM:
            case nvrhi::Format::BC4_SNORM:
                mipSize = ((w + 3) / 4) * ((h + 3) / 4) * 8;
                break;

            case nvrhi::Format::BC6H_UFLOAT:
            case nvrhi::Format::BC6H_SFLOAT:
            case nvrhi::Format::BC7_UNORM:
            case nvrhi::Format::BC7_UNORM_SRGB:
                mipSize = ((w + 3) / 4) * ((h + 3) / 4) * 16;
                break;

            case nvrhi::Format::RGBA8_UNORM:
            case nvrhi::Format::RGBA8_SNORM:
            case nvrhi::Format::SRGBA8_UNORM:
            case nvrhi::Format::BGRA8_UNORM:
            case nvrhi::Format::SBGRA8_UNORM:
                mipSize = w * h * 4;
                break;

            case nvrhi::Format::RGBA16_FLOAT:
            case nvrhi::Format::RGBA16_UNORM:
            case nvrhi::Format::RGBA16_SNORM:
                mipSize = w * h * 8;
                break;

            case nvrhi::Format::RGBA32_FLOAT:
                mipSize = w * h * 16;
                break;

            case nvrhi::Format::R8_UNORM:
                mipSize = w * h;
                break;

            case nvrhi::Format::RG8_UNORM:
                mipSize = w * h * 2;
                break;

            case nvrhi::Format::R16_FLOAT:
            case nvrhi::Format::R16_UNORM:
                mipSize = w * h * 2;
                break;

            case nvrhi::Format::RG16_FLOAT:
                mipSize = w * h * 4;
                break;

            case nvrhi::Format::D24S8:
            case nvrhi::Format::D32:
                mipSize = w * h * 4;
                break;

            default:
                mipSize = w * h * 4;
                break;
        }

        totalSize += mipSize;

        mipWidth = std::max(1u, mipWidth / 2);
        mipHeight = std::max(1u, mipHeight / 2);
    }

    totalSize *= arraySize;
    totalSize *= std::max(1u, sampleCount);

    return totalSize;
}

bool TextureKey::operator<(const TextureKey& other) const {
    if (colorSpace != other.colorSpace)
        return colorSpace < other.colorSpace;
    return path < other.path;
}

TextureManager::TextureManager(RenderDevice* device)
    : m_device(device)
{
    VERIFY(m_device);

    m_textures.reserve(1024);

}

TextureManager::~TextureManager() {

    u32 leakCount = 0;
    for (const auto& tex : m_textures) {
        if (tex.isAlive) {
            Msg("! [TextureManager] ⚠️ Leak: %s (refCount=%u)",
                tex.filePath.c_str(), tex.refCount);
            leakCount++;
        }
    }

    if (leakCount > 0) {
        Msg("! [TextureManager] ❌ %u texture leaks detected!", leakCount);
    }

    PrintStatistics();
}

TextureHandle TextureManager::AllocateHandle() {
    u32 index;
    u32 generation;

    if (!m_freeSlots.empty()) {
        index = m_freeSlots.back();
        m_freeSlots.pop_back();

        generation = m_textures[index].generation + 1;
        m_textures[index].generation = generation;
    } else {
        index = (u32)m_textures.size();
        generation = 0;

        m_textures.emplace_back();
        m_textures[index].generation = generation;
    }

    return TextureHandle(index, generation);
}

void TextureManager::FreeHandle(TextureHandle handle) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];
    meta.isAlive = false;

    if (!meta.filePath.empty()) {
        const auto mapped = m_pathToHandle.find(TextureKey{ meta.filePath, meta.colorSpace });
        if (mapped != m_pathToHandle.end() && mapped->second == handle)
            m_pathToHandle.erase(mapped);
    }

    meta.nvrhiTexture = nullptr;
    meta.videoTextureData.reset();
    meta.sequenceTextureData.reset();
    meta.filePath = shared_str();
    meta.colorSpace = fg::TextureColorSpace::Linear;
    meta.state = TextureState::Unloaded;
    meta.refCount = 0;
    meta.memoryUsed = 0;
    meta.lastAccessTime = 0.0f;
    meta.videoActiveThisFrame = false;

    m_freeSlots.push_back(handle.index);
}

bool TextureManager::ValidateHandle(TextureHandle handle) const {
    if (!handle.IsValid()) return false;
    if (handle.index >= m_textures.size()) return false;

    const TextureMetadata& meta = m_textures[handle.index];
    if (!meta.isAlive) return false;
    if (meta.generation != handle.generation) return false;

    return true;
}

TextureHandle TextureManager::LoadTexture(
    const char* path,
    TexturePriority priority)
{
    return LoadTexture(path, fg::TextureColorSpace::Linear, priority);
}

TextureHandle TextureManager::LoadTexture(
    const char* path,
    fg::TextureColorSpace colorSpace,
    TexturePriority priority)
{
    const TextureKey key{ shared_str(path), colorSpace };

    auto it = m_pathToHandle.find(key);
    if (it != m_pathToHandle.end()) {
        TextureHandle existing = it->second;
        if (ValidateHandle(existing)) {
            AddRef(existing);
            return existing;
        }
    }

    TextureHandle handle = AllocateHandle();
    TextureMetadata& meta = m_textures[handle.index];

    meta.filePath = key.path;
    meta.colorSpace = colorSpace;
    meta.state = TextureState::Unloaded;
    meta.priority = priority;
    meta.isAlive = true;
    meta.refCount = 1;

    m_pathToHandle[key] = handle;

    m_stats.texturesTotal++;

    LoadTextureSync(handle);

    return handle;
}

TextureHandle TextureManager::CreateTexture(
    const TextureDesc& desc,
    const void* initialData)
{
    TextureHandle handle = AllocateHandle();
    TextureMetadata& meta = m_textures[handle.index];

    meta.desc = desc;
    meta.state = TextureState::Unloaded;
    meta.priority = TexturePriority::High;
    meta.isAlive = true;
    meta.refCount = 1;

    nvrhi::TextureDesc nvrhiDesc;
    nvrhiDesc.width = desc.width;
    nvrhiDesc.height = desc.height;
    nvrhiDesc.depth = desc.depth;
    nvrhiDesc.arraySize = desc.arraySize;
    nvrhiDesc.mipLevels = desc.mipLevels;
    nvrhiDesc.format = desc.format;
    nvrhiDesc.sampleCount = std::max(1u, desc.sampleCount);
    nvrhiDesc.debugName = desc.debugName.c_str();
    nvrhiDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    nvrhiDesc.keepInitialState = true;

    switch (desc.type) {
        case TextureDesc::Texture1D:
            nvrhiDesc.dimension = nvrhi::TextureDimension::Texture1D;
            break;
        case TextureDesc::Texture2D:
            nvrhiDesc.dimension = nvrhiDesc.sampleCount > 1
                ? nvrhi::TextureDimension::Texture2DMS
                : nvrhi::TextureDimension::Texture2D;
            break;
        case TextureDesc::Texture2DArray:
            nvrhiDesc.dimension = nvrhiDesc.sampleCount > 1
                ? nvrhi::TextureDimension::Texture2DMSArray
                : nvrhi::TextureDimension::Texture2DArray;
            break;
        case TextureDesc::Texture3D:
            nvrhiDesc.dimension = nvrhi::TextureDimension::Texture3D;
            break;
        case TextureDesc::TextureCube:
            nvrhiDesc.dimension = nvrhi::TextureDimension::TextureCube;
            break;
        default:
            nvrhiDesc.dimension = nvrhi::TextureDimension::Texture2D;
            break;
    }

    nvrhiDesc.isRenderTarget = desc.isRenderTarget;
    nvrhiDesc.isUAV = desc.isUAV;
    nvrhiDesc.isShaderResource = true;

    if (desc.isRenderTarget && !desc.isDepthStencil) {
        nvrhiDesc.useClearValue = true;
        nvrhiDesc.clearValue = nvrhi::Color(0.0f);
    }

    if (desc.isDepthStencil) {
        nvrhiDesc.isRenderTarget = true;
        nvrhiDesc.isTypeless = true;
        nvrhiDesc.useClearValue = true;
        nvrhiDesc.clearValue = nvrhi::Color(0.0f);
    }

    meta.nvrhiTexture = m_device->GetNVRHIDevice()->createTexture(nvrhiDesc);

    if (meta.nvrhiTexture) {
        meta.state = TextureState::Resident;

        meta.memoryUsed = desc.CalculateMemorySize();
        m_stats.texturesResident++;
        m_memoryUsed += meta.memoryUsed;

    } else {
        Msg("! [TextureManager] Failed to create NVRHI texture '%s'", desc.debugName.c_str());
    }

    m_stats.texturesTotal++;

    return handle;
}

nvrhi::ITexture* TextureManager::GetNVRHITexture(TextureHandle handle) {
    if (!ValidateHandle(handle)) return nullptr;

    TextureMetadata& meta = m_textures[handle.index];

    meta.lastAccessTime = 0.0f;

    if (meta.videoTextureData) {
        meta.videoActiveThisFrame = true;
    }

    if (meta.state == TextureState::Unloaded)
        LoadTextureSync(handle);

    return meta.nvrhiTexture.Get();
}

const TextureMetadata* TextureManager::GetMetadata(TextureHandle handle) const {
    if (!ValidateHandle(handle)) return nullptr;
    return &m_textures[handle.index];
}

void TextureManager::NotifyContentChanged(TextureMetadata& metadata)
{
    std::lock_guard<std::mutex> lock(m_texturesMutex);
    metadata.contentRevision = ++m_contentRevision;
}

u64 TextureManager::GetContentRevision(const xr_set<nvrhi::ITexture*>& textures,
    nvrhi::ITexture* sky0, nvrhi::ITexture* sky1) const
{
    std::lock_guard<std::mutex> lock(m_texturesMutex);
    u64 revision = 0;
    for (const auto& metadata : m_textures)
    {
        auto* texture = metadata.nvrhiTexture.Get();
        if (metadata.isAlive && texture && (texture == sky0 || texture == sky1 || textures.find(texture) != textures.end()))
            revision = std::max(revision, metadata.contentRevision);
    }
    return revision;
}

TextureHandle TextureManager::FindTexture(const char* path, fg::TextureColorSpace colorSpace) const {
    auto it = m_pathToHandle.find(TextureKey{ shared_str(path), colorSpace });
    if (it != m_pathToHandle.end()) {
        TextureHandle handle = it->second;
        if (ValidateHandle(handle))
            return handle;
    }
    return TextureHandle();
}

void TextureManager::AddRef(TextureHandle handle) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];
    meta.refCount++;
}

void TextureManager::Release(TextureHandle handle) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];

    if (meta.refCount > 0) {
        meta.refCount--;
    }

    if (meta.refCount == 0) {
        if (meta.nvrhiTexture) {
            m_memoryUsed -= meta.memoryUsed;
            meta.nvrhiTexture = nullptr;
            meta.memoryUsed = 0;
        }

        FreeHandle(handle);
    }
}

void TextureManager::Update(float deltaTime) {
    for (auto& meta : m_textures) {
        if (!meta.isAlive) continue;
        meta.lastAccessTime += deltaTime;
    }

    UpdateVideoTextures();

    UpdateSequenceTextures(deltaTime);
}


void TextureManager::LoadTextureSync(TextureHandle handle) {
    if (!ValidateHandle(handle)) {
        Msg("! [TextureManager] LoadTextureSync: Invalid handle");
        return;
    }

    TextureMetadata& meta = m_textures[handle.index];

    if (meta.state == TextureState::Resident) {
        return;
    }

    meta.state = TextureState::Loading;

    DDSData ddsData;
    if (!DDSLoader::LoadFromFile(meta.filePath.c_str(), ddsData)) {
        Msg("! [TextureManager] Failed to load texture: %s", meta.filePath.c_str());
        meta.state = TextureState::Missing;
        return;
    }

    bool isVideoTexture = (ddsData.type == DDSData::TextureType::Video);
    bool isSequenceTexture = (ddsData.type == DDSData::TextureType::Sequence);

    const TextureDesc sourceDesc = ddsData.desc;
    const u64 sourceDataSize = ddsData.totalDataSize;

    nvrhi::TextureDesc textureDesc;
    textureDesc.width = sourceDesc.width;
    textureDesc.height = sourceDesc.height;
    textureDesc.depth = sourceDesc.depth;
    textureDesc.arraySize = sourceDesc.arraySize;
    textureDesc.mipLevels = sourceDesc.mipLevels;
    textureDesc.format = fg::FormatForColorSpace(sourceDesc.format, meta.colorSpace);
    textureDesc.debugName = ddsData.filePath.c_str();
    textureDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    textureDesc.keepInitialState = true;
    switch (sourceDesc.type) {
        case TextureDesc::Texture1D:
            textureDesc.dimension = nvrhi::TextureDimension::Texture1D;
            break;
        case TextureDesc::Texture2DArray:
            textureDesc.dimension = nvrhi::TextureDimension::Texture2DArray;
            break;
        case TextureDesc::Texture3D:
            textureDesc.dimension = nvrhi::TextureDimension::Texture3D;
            break;
        case TextureDesc::TextureCube:
            textureDesc.dimension = nvrhi::TextureDimension::TextureCube;
            break;
        default:
            textureDesc.dimension = nvrhi::TextureDimension::Texture2D;
            break;
    }

    nvrhi::TextureHandle texture = m_device->GetNVRHIDevice()->createTexture(textureDesc);
    if (!texture) {
        Msg("! [TextureManager] Failed to create NVRHI texture: %s", meta.filePath.c_str());
        meta.state = TextureState::Missing;
        return;
    }

    const u32 mipsPerSlice = std::max(1u, sourceDesc.mipLevels);
    xr_vector<fg::RenderDevice::TextureSliceData> slices;
    slices.reserve(ddsData.mipLevels.size());
    for (u32 index = 0; index < ddsData.mipLevels.size(); ++index) {
        const DDSMipLevel& mip = ddsData.mipLevels[index];
        fg::RenderDevice::TextureSliceData slice;
        slice.arraySlice = index / mipsPerSlice;
        slice.mipLevel = index % mipsPerSlice;
        slice.data = mip.data;
        slice.dataSize = mip.size;
        slice.rowPitch = mip.rowPitch;
        slice.slicePitch = mip.slicePitch;
        slices.push_back(slice);
    }
    m_device->UploadTextureSlices(texture, slices.data(), (u32)slices.size());

    meta.nvrhiTexture = texture;
    meta.desc = sourceDesc;
    meta.desc.format = textureDesc.format;
    meta.desc.isSRGB = nvrhi::getFormatInfo(textureDesc.format).isSRGB;

    if (isVideoTexture) {
        meta.videoTextureData = xr_make_unique<DDSData>();
        *meta.videoTextureData = std::move(ddsData);
    }

    if (isSequenceTexture) {
        meta.sequenceTextureData = xr_make_unique<DDSData>();
        *meta.sequenceTextureData = std::move(ddsData);
        meta.sequenceTextureData->sequenceState->currentFrame = 0;
    }

    meta.state = TextureState::Resident;
    meta.memoryUsed = sourceDataSize;

    m_memoryUsed += meta.memoryUsed;
}

TextureManager::Statistics TextureManager::GetStatistics() const {
    m_stats.totalMemoryUsed = m_memoryUsed;
    m_stats.memoryBudget = m_memoryBudget;
    m_stats.texturesTotal = 0;
    m_stats.texturesResident = 0;
    m_stats.texturesLoading = 0;

    for (const auto& meta : m_textures) {
        if (!meta.isAlive) continue;

        m_stats.texturesTotal++;

        switch (meta.state) {
            case TextureState::Resident:
                m_stats.texturesResident++;
                break;
            case TextureState::Loading:
                m_stats.texturesLoading++;
                break;
            default:
                break;
        }
    }

    return m_stats;
}

void TextureManager::PrintStatistics() const {
    auto stats = GetStatistics();

    Msg("! [TextureManager] Statistics:");
    Msg("!   Memory: %llu / %llu MB (%.1f%%)",
        stats.totalMemoryUsed / (1024 * 1024),
        stats.memoryBudget / (1024 * 1024),
        stats.memoryUsagePercent());
    Msg("!   Textures: %u total, %u resident, %u loading",
        stats.texturesTotal,
        stats.texturesResident,
        stats.texturesLoading);
}

void TextureManager::UpdateVideoTextures() {
    static int s_frameCount = 0;
    s_frameCount++;

    int videoTextureCount = 0;
    int updatedCount = 0;
    int skippedInactive = 0;

    for (auto& meta : m_textures) {
        if (!meta.isAlive || !meta.videoTextureData) {
            continue;
        }

        videoTextureCount++;

        if (!meta.videoActiveThisFrame && meta.lastAccessTime > 0.5f) {
            skippedInactive++;
            continue;
        }

        meta.videoActiveThisFrame = false;

        bool frameChanged = DDSLoader::UpdateVideoFrame(*meta.videoTextureData, Device.dwTimeContinual);

        if (!frameChanged) {
            continue;
        }

        updatedCount++;

        auto* videoState = meta.videoTextureData->videoState;
        if (!videoState || !videoState->needsUpdate) {
            Msg("! [TextureManager] Frame changed but needsUpdate=false for: %s", meta.filePath.c_str());
            continue;
        }

        nvrhi::ITexture* nvrhiTex = meta.nvrhiTexture;
        if (!nvrhiTex) {
            Msg("! [TextureManager] Video texture has no NVRHI texture: %s", meta.filePath.c_str());
            continue;
        }

        u32 texWidth = videoState->textureWidth;
        u32 texHeight = videoState->textureHeight;
        size_t dataSize = texWidth * texHeight * 4;

        m_device->UploadTextureDataToNVRHI(
            nvrhiTex,
            0,
            0,
            videoState->frameBuffer.data(),
            dataSize
        );
        NotifyContentChanged(meta);

        videoState->needsUpdate = false;

    }

}

void TextureManager::UpdateSequenceTextures(float deltaTime) {

    u32 currentTime = Device.dwTimeContinual;

    for (auto& meta : m_textures) {
        if (!meta.isAlive || !meta.sequenceTextureData) {
            continue;
        }

        auto* seqState = meta.sequenceTextureData->sequenceState;
        if (!seqState || seqState->frameData.empty()) {
            continue;
        }

        if (seqState->lastUpdateTime == 0) {
            seqState->lastUpdateTime = currentTime;
            continue;
        }

        u32 elapsedMs = currentTime - seqState->lastUpdateTime;

        if (elapsedMs >= seqState->msPerFrame) {
            seqState->lastUpdateTime = currentTime;

            u32 nextFrame = seqState->currentFrame + 1;

            if (nextFrame >= seqState->frameData.size()) {
                if (seqState->cycled) {
                    nextFrame = 0;
                } else {
                    nextFrame = seqState->currentFrame;
                    continue;
                }
            }

            if (DDSLoader::UpdateSequenceFrame(*meta.sequenceTextureData, nextFrame)) {
                nvrhi::ITexture* nvrhiTex = meta.nvrhiTexture;
                if (!nvrhiTex) {
                    continue;
                }

                const DDSMipLevel& frameData = seqState->frameData[nextFrame];

                m_device->UploadTextureDataToNVRHI(
                    nvrhiTex,
                    0,
                    0,
                    seqState->frameBuffer.data(),
                    frameData.size,
                    frameData.rowPitch,
                    frameData.slicePitch
                );
                NotifyContentChanged(meta);

                seqState->currentFrame = nextFrame;
                seqState->needsUpdate = false;
            }
        }
    }
}

TextureManager* GetActiveTextureManager()
{
    RenderDevice* device = GEnv.Render ? GEnv.Render->GetRenderDevice() : nullptr;
    FGResourceManager* resourceManager = device ? device->GetFGResourceManager() : nullptr;
    return resourceManager ? resourceManager->GetTextureManager() : nullptr;
}

TextureRef::TextureRef(const TextureRef& other)
    : m_handle(other.m_handle)
{
    if (TextureManager* textures = m_handle.IsValid() ? GetActiveTextureManager() : nullptr)
        textures->AddRef(m_handle);
}

TextureRef::TextureRef(TextureRef&& other) noexcept
    : m_handle(other.m_handle)
{
    other.m_handle = TextureHandle();
}

TextureRef::~TextureRef()
{
    Reset();
}

TextureRef& TextureRef::operator=(const TextureRef& other)
{
    if (this != &other)
        *this = TextureRef(other);
    return *this;
}

TextureRef& TextureRef::operator=(TextureRef&& other) noexcept
{
    if (this != &other)
    {
        Reset();
        m_handle = other.m_handle;
        other.m_handle = TextureHandle();
    }
    return *this;
}

nvrhi::ITexture* TextureRef::Load(const char* path, fg::TextureColorSpace colorSpace)
{
    TextureManager* textures = GetActiveTextureManager();
    if (!textures)
    {
        Reset();
        return nullptr;
    }

    const TextureHandle handle = textures->LoadTexture(path, colorSpace);
    Reset();
    m_handle = handle;
    return textures->GetNVRHITexture(m_handle);
}

nvrhi::ITexture* TextureRef::Get() const
{
    TextureManager* textures = m_handle.IsValid() ? GetActiveTextureManager() : nullptr;
    return textures ? textures->GetNVRHITexture(m_handle) : nullptr;
}

void TextureRef::Reset()
{
    if (!m_handle.IsValid())
        return;
    if (TextureManager* textures = GetActiveTextureManager())
        textures->Release(m_handle);
    m_handle = TextureHandle();
}

}
