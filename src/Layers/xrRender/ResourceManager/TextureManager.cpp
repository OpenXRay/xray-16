#include "stdafx.h"
#include "TextureManager.h"
#include "DDSLoader.h"
#include "TextureStreaming.h"
#include "../RenderContext/RenderDevice.h"

namespace xray::render::resources {

using namespace xray::render::ng;

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

    return totalSize;
}

TextureManager::TextureManager(RenderDevice* device)
    : m_device(device)
{
    VERIFY(m_device);

    m_textures.reserve(1024);

    m_streamingManager = xr_make_unique<StreamingManager>(device, this);

    Msg("! [TextureManager] Created with budget: %llu MB",
        m_memoryBudget / (1024 * 1024));
}

TextureManager::~TextureManager() {
    Msg("! [TextureManager] Destroying...");

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
        m_pathToHandle.erase(meta.filePath);
    }

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
    shared_str pathStr = path;

    auto it = m_pathToHandle.find(pathStr);
    if (it != m_pathToHandle.end()) {
        TextureHandle existing = it->second;
        if (ValidateHandle(existing)) {
            Msg("! [TextureManager] Texture already loaded: %s", path);
            AddRef(existing);
            return existing;
        }
    }

    TextureHandle handle = AllocateHandle();
    TextureMetadata& meta = m_textures[handle.index];

    meta.filePath = pathStr;
    meta.state = TextureState::Unloaded;
    meta.priority = priority;
    meta.isAlive = true;
    meta.refCount = 1;

    m_pathToHandle[pathStr] = handle;

    Msg("! [TextureManager] Registered texture: %s (handle=%u.%u)",
        path, handle.index, handle.generation);

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

    m_stats.texturesTotal++;

    return handle;
}

TextureHandle TextureManager::ImportTexture(
    nvrhi::TextureHandle nvrhiTexture,
    const TextureDesc& desc,
    const char* debugName)
{
    TextureHandle handle = AllocateHandle();
    TextureMetadata& meta = m_textures[handle.index];

    meta.desc = desc;
    meta.filePath = debugName;
    meta.state = TextureState::Resident;
    meta.priority = TexturePriority::Critical;
    meta.isAlive = true;
    meta.refCount = 1;
    meta.nvrhiTexture = nvrhiTexture;
    meta.residentMips = desc.mipLevels;
    meta.totalMips = desc.mipLevels;
    meta.memoryUsed = desc.CalculateMemorySize();

    m_memoryUsed += meta.memoryUsed;

    m_stats.texturesTotal++;
    m_stats.texturesResident++;

    return handle;
}

nvrhi::ITexture* TextureManager::GetNVRHITexture(TextureHandle handle) {
    if (!ValidateHandle(handle)) return nullptr;

    TextureMetadata& meta = m_textures[handle.index];

    meta.lastAccessTime = 0.0f;
    meta.accessCount++;

    if (meta.state == TextureState::Unloaded) {
        Msg("! [TextureManager] ⚠️ Accessing unloaded texture: %s",
            meta.filePath.c_str());
    }

    return meta.nvrhiTexture.Get();
}

const TextureMetadata* TextureManager::GetMetadata(TextureHandle handle) const {
    if (!ValidateHandle(handle)) return nullptr;
    return &m_textures[handle.index];
}

bool TextureManager::IsResident(TextureHandle handle) const {
    if (!ValidateHandle(handle)) return false;
    return m_textures[handle.index].IsResident();
}

void TextureManager::SetMemoryBudget(u64 bytes) {
    m_memoryBudget = bytes;
    Msg("! [TextureManager] Memory budget set to: %llu MB",
        m_memoryBudget / (1024 * 1024));
}

void TextureManager::RequestMips(TextureHandle handle, u32 mipCount) {
    if (!ValidateHandle(handle)) return;

    const TextureMetadata& meta = m_textures[handle.index];
    u32 targetMips = std::min(mipCount, meta.totalMips);

    m_textures[handle.index].requestedMips = targetMips;

    m_streamingManager->RequestMips(handle, targetMips, meta.priority);
}

void TextureManager::SetPriority(TextureHandle handle, TexturePriority priority) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];
    meta.priority = priority;
}

void TextureManager::Touch(TextureHandle handle) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];
    meta.lastAccessTime = 0.0f;
    meta.accessCount++;
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

void TextureManager::Evict(TextureHandle handle) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];

    if (meta.state == TextureState::Resident) {
        Msg("! [TextureManager] Evicting: %s", meta.filePath.c_str());
        EvictTextureInternal(handle);
    }
}

bool TextureManager::CheckMemoryBudget(u64 requiredBytes) const {
    return (m_memoryUsed + requiredBytes) <= m_memoryBudget;
}

bool TextureManager::EnforceMemoryBudget(u64 requiredBytes) {
    if (CheckMemoryBudget(requiredBytes)) {
        return true;
    }

    u64 bytesNeeded = (m_memoryUsed + requiredBytes) - m_memoryBudget;

    Msg("! [TextureManager] Over budget! Need to free %llu MB",
        bytesNeeded / (1024 * 1024));

    return EvictTextures(bytesNeeded);
}

bool TextureManager::EvictTextures(u64 bytesNeeded) {
    u64 bytesFreed = 0;

    struct EvictionCandidate {
        TextureHandle handle;
        float score;
        u64 memoryUsed;

        bool operator<(const EvictionCandidate& other) const {
            return score > other.score;
        }
    };

    xr_vector<EvictionCandidate> candidates;

    for (u32 i = 0; i < m_textures.size(); i++) {
        const TextureMetadata& meta = m_textures[i];

        if (!meta.isAlive) continue;
        if (!meta.CanEvict()) continue;

        float score = 0.0f;

        score += meta.lastAccessTime * 10.0f;

        score += (float)meta.priority * 100.0f;

        score -= (float)meta.accessCount * 0.1f;

        score += (float)meta.memoryUsed / (1024.0f * 1024.0f);

        EvictionCandidate candidate;
        candidate.handle = TextureHandle(i, meta.generation);
        candidate.score = score;
        candidate.memoryUsed = meta.memoryUsed;

        candidates.push_back(candidate);
    }

    std::sort(candidates.begin(), candidates.end());

    Msg("! [TextureManager] Found %u eviction candidates", candidates.size());

    for (const auto& candidate : candidates) {
        if (bytesFreed >= bytesNeeded) {
            break;
        }

        const TextureMetadata* meta = GetMetadata(candidate.handle);

        Msg("!   Evicting: %s (score=%.2f, %llu KB)",
            meta->filePath.c_str(),
            candidate.score,
            candidate.memoryUsed / 1024);

        EvictTextureInternal(candidate.handle);

        bytesFreed += candidate.memoryUsed;
    }

    Msg("! [TextureManager] Freed %llu MB (needed %llu MB)",
        bytesFreed / (1024 * 1024),
        bytesNeeded / (1024 * 1024));

    return bytesFreed >= bytesNeeded;
}

void TextureManager::EvictTextureInternal(TextureHandle handle) {
    if (!ValidateHandle(handle)) return;

    TextureMetadata& meta = m_textures[handle.index];

    if (meta.state != TextureState::Resident) {
        return;
    }

    if (meta.nvrhiTexture) {
        meta.nvrhiTexture = nullptr;
    }

    meta.state = TextureState::Evicted;
    meta.residentMips = 0;

    m_memoryUsed -= meta.memoryUsed;
    meta.memoryUsed = 0;

    Msg("! [TextureManager] Evicted: %s", meta.filePath.c_str());
}

void TextureManager::Update(float deltaTime) {
    for (auto& meta : m_textures) {
        if (!meta.isAlive) continue;
        meta.lastAccessTime += deltaTime;
    }

    m_streamingManager->Update(deltaTime);

    if (m_memoryUsed > m_memoryBudget) {
        u64 excess = m_memoryUsed - m_memoryBudget;
        EvictTextures(excess);
    }
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
        Msg("! [TextureManager] Failed to load DDS: %s", meta.filePath.c_str());
        meta.state = TextureState::Unloaded;
        return;
    }

    ng::RenderDevice::TextureDesc deviceDesc;
    deviceDesc.width = ddsData.desc.width;
    deviceDesc.height = ddsData.desc.height;
    deviceDesc.depth = ddsData.desc.depth;
    deviceDesc.arraySize = ddsData.desc.arraySize;
    deviceDesc.mipLevels = ddsData.desc.mipLevels;
    deviceDesc.format = ddsData.desc.format;
    deviceDesc.debugName = meta.filePath;

    switch (ddsData.desc.type) {
        case TextureDesc::Texture1D:
            deviceDesc.dimension = ng::RenderDevice::TextureDesc::Texture1D;
            break;
        case TextureDesc::Texture2D:
            deviceDesc.dimension = ng::RenderDevice::TextureDesc::Texture2D;
            break;
        case TextureDesc::Texture3D:
            deviceDesc.dimension = ng::RenderDevice::TextureDesc::Texture3D;
            break;
        case TextureDesc::TextureCube:
            deviceDesc.dimension = ng::RenderDevice::TextureDesc::TextureCube;
            break;
    }

    ng::TextureHandle deviceHandle = m_device->CreateTexture(deviceDesc, nullptr);
    if (!deviceHandle.IsValid()) {
        Msg("! [TextureManager] Failed to create NVRHI texture: %s", meta.filePath.c_str());
        meta.state = TextureState::Unloaded;
        return;
    }

    xr_vector<ng::RenderDevice::TextureSliceData> slices;
    slices.reserve(ddsData.mipLevels.size());

    for (const DDSMipLevel& mip : ddsData.mipLevels) {
        ng::RenderDevice::TextureSliceData slice;

        u32 totalMips = ddsData.desc.mipLevels;
        u32 mipIndex = (u32)(&mip - &ddsData.mipLevels[0]);

        slice.arraySlice = mipIndex / totalMips;
        slice.mipLevel = mipIndex % totalMips;
        slice.data = mip.data;
        slice.dataSize = mip.size;

        slices.push_back(slice);
    }

    m_device->UploadTextureData(deviceHandle, slices.data(), (u32)slices.size());

    meta.nvrhiTexture = m_device->GetNativeTexture(deviceHandle);

    meta.state = TextureState::Resident;
    meta.residentMips = ddsData.desc.mipLevels;
    meta.totalMips = ddsData.desc.mipLevels;
    meta.requestedMips = ddsData.desc.mipLevels;
    meta.memoryUsed = ddsData.totalDataSize;

    m_memoryUsed += meta.memoryUsed;

    Msg("* [TextureManager] Loaded texture: %s (%u mips, %llu bytes, total memory: %llu / %llu)",
        meta.filePath.c_str(), meta.residentMips, meta.memoryUsed,
        m_memoryUsed, m_memoryBudget);
}

void TextureManager::LoadTextureAsync(TextureHandle handle) {
}

void TextureManager::StreamMips(TextureHandle handle, u32 targetMips) {
    m_streamingManager->RequestMips(handle, targetMips, TexturePriority::Medium);
}

TextureManager::Statistics TextureManager::GetStatistics() const {
    m_stats.totalMemoryUsed = m_memoryUsed;
    m_stats.memoryBudget = m_memoryBudget;
    m_stats.texturesTotal = 0;
    m_stats.texturesResident = 0;
    m_stats.texturesLoading = 0;
    m_stats.texturesEvicted = 0;

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
            case TextureState::Evicted:
                m_stats.texturesEvicted++;
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
    Msg("!   Textures: %u total, %u resident, %u loading, %u evicted",
        stats.texturesTotal,
        stats.texturesResident,
        stats.texturesLoading,
        stats.texturesEvicted);
}

const char* TextureStateToString(TextureState state) {
    switch (state) {
        case TextureState::Unloaded: return "Unloaded";
        case TextureState::Loading: return "Loading";
        case TextureState::Resident: return "Resident";
        case TextureState::Evicting: return "Evicting";
        case TextureState::Evicted: return "Evicted";
        default: return "Unknown";
    }
}

const char* TexturePriorityToString(TexturePriority priority) {
    switch (priority) {
        case TexturePriority::Critical: return "Critical";
        case TexturePriority::High: return "High";
        case TexturePriority::Medium: return "Medium";
        case TexturePriority::Low: return "Low";
        case TexturePriority::VeryLow: return "VeryLow";
        default: return "Unknown";
    }
}

}
