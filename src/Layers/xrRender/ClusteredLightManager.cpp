#include "stdafx.h"
#include "ClusteredLightManager.h"
#include "light.h"
#include "Light_Package.h"
#include "Layers/xrRender/Bindless/BindlessTypes.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "xrEngine/IRenderBackend.h"
#include "xrCore/Threading/ParallelFor.hpp"
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <numeric>

using xray::render::fg::bindless::INVALID_TEXTURE_INDEX;

namespace xray::render::fg
{

ClusteredLightManager& ClusteredLightManager::Instance()
{
    static ClusteredLightManager instance;
    return instance;
}

void ClusteredLightManager::Initialize(fg::RenderDevice* device)
{
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    m_device = nvDevice;
    m_lightsCPU.reserve(MAX_LIGHTS);
    std::iota(m_identityIndices.begin(), m_identityIndices.end(), 0u);
    m_visibleMaskOnes.fill(1u);

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = MAX_LIGHTS * sizeof(GPULightData);
        desc.structStride = sizeof(GPULightData);
        desc.debugName = "ClusteredLights_LightData";
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        desc.canHaveTypedViews = false;
        desc.canHaveUAVs = false;
        m_lightDataBuffer = nvDevice->createBuffer(desc);
    }

    const u32 maxClusters = 128 * 128 * CLUSTER_NUM_SLICES;
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = maxClusters * sizeof(u32) * 2;
        desc.structStride = sizeof(u32) * 2;
        desc.debugName = "ClusteredLights_ClusterGrid";
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs = true;
        m_clusterGridBuffer = nvDevice->createBuffer(desc);
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = MAX_LIGHT_INDICES * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.debugName = "ClusteredLights_LightIndexList";
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs = true;
        m_lightIndexListBuffer = nvDevice->createBuffer(desc);
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(u32);
        desc.debugName = "ClusteredLights_IndexCounter";
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        m_lightIndexCounterBuffer = nvDevice->createBuffer(desc);
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = MAX_LIGHTS * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.debugName = "ClusteredLights_VisibleIndices";
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs = true;
        m_visibleLightIndicesBuffer = nvDevice->createBuffer(desc);
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(u32);
        desc.debugName = "ClusteredLights_VisibleCount";
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        m_visibleLightCountBuffer = nvDevice->createBuffer(desc);
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = MAX_LIGHTS * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.debugName = "ClusteredLights_DIIndices";
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_diLightIndicesBuffer = nvDevice->createBuffer(desc);
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = MAX_LIGHTS * sizeof(float);
        desc.structStride = sizeof(float);
        desc.debugName = "ClusteredLights_DICDF";
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_diLightCDFBuffer = nvDevice->createBuffer(desc);
    }

    m_diIndicesCPU.reserve(MAX_LIGHTS);
    m_diCDFCPU.reserve(MAX_LIGHTS);

    Msg("* [ClusteredLights] Created GPU buffers (max %u lights, %u max clusters)",
        MAX_LIGHTS, maxClusters);
}

void ClusteredLightManager::Shutdown()
{
    m_lightDataBuffer = nullptr;
    m_clusterGridBuffer = nullptr;
    m_lightIndexListBuffer = nullptr;
    m_lightIndexCounterBuffer = nullptr;
    m_visibleLightIndicesBuffer = nullptr;
    m_visibleLightCountBuffer = nullptr;
    m_diLightIndicesBuffer = nullptr;
    m_diLightCDFBuffer = nullptr;
    for (u32 i = 0; i < STATS_READBACK_SLOTS; ++i)
        m_statsReadbackBuffers[i] = nullptr;
    m_statsWriteSlot = 0;
    m_statsScheduled = 0;
    m_visibleLightCountCPU = 0;
    m_diLightCount = 0;
    m_diPowerSum = 0.f;
    m_diIndicesCPU.clear();
    m_diCDFCPU.clear();
    m_lightsCPU.clear();
    m_slotOwners.clear();
    m_freeSlots.clear();
    m_lightToSlot.clear();
    m_spotTextureCache.clear();
    m_lightSetFingerprint = 0;
    m_device = nullptr;
}

void ClusteredLightManager::BeginFrame()
{
    m_numPoint = 0;
    m_numSpot = 0;
    m_numOmni = 0;
    m_diLightCount = 0;
    m_diPowerSum = 0.f;
    m_diIndicesCPU.clear();
    m_diCDFCPU.clear();
}

GPULightData ClusteredLightManager::BuildGPULightData(const light* L)
{
    GPULightData gpu;

    const float range = L->range;
    const float invRangeSq = 1.0f / (range * range + 0.0001f);
    const float virt = std::max(L->virtual_size, 0.1f);
    const float virtSizeSq = virt * virt;

    const float signedInv = L->flags.bHudMode ? -invRangeSq : invRangeSq;
    gpu.positionAndInvRangeSq.set(L->position.x, L->position.y, L->position.z, signedInv);
    gpu.colorAndRange.set(L->color.r, L->color.g, L->color.b, range);

    std::memset(&gpu.spotVP, 0, sizeof(gpu.spotVP));

    const u32 lightType = L->flags.type;
    const bool isSpot = (lightType == IRender_Light::SPOT || lightType == IRender_Light::OMNIPART);

    if (isSpot)
    {
        const float cosOuter = _cos(L->cone);
        const float cosInner = _cos(L->cone * 0.8f);
        const float scale = 1.0f / std::max(cosInner - cosOuter, 0.001f);
        const float offset = -cosOuter * scale;

        u32 texIdx = INVALID_TEXTURE_INDEX;
        if (!L->spot_texture_name.empty())
            texIdx = GetOrLoadSpotTexture(L->spot_texture_name);

        gpu.directionAndSpotScale.set(L->direction.x, L->direction.y, L->direction.z, scale);

        float texIdxBits;
        std::memcpy(&texIdxBits, &texIdx, sizeof(float));
        gpu.spotParamsAndType.set(offset, 1.0f, texIdxBits, 0.0f);

        if (texIdx != INVALID_TEXTURE_INDEX)
        {
            Fvector L_dir, L_up, L_right;
            L_dir.set(L->direction);
            float l_dir_m = L_dir.magnitude();
            if (_valid(l_dir_m) && l_dir_m > EPS_S)
                L_dir.div(l_dir_m);
            else
                L_dir.set(0, 0, 1);

            if (L->right.square_magnitude() > EPS)
            {
                L_right.set(L->right);
                L_right.normalize();
                L_up.crossproduct(L_dir, L_right);
                L_up.normalize();
                L_right.crossproduct(L_up, L_dir);
                L_right.normalize();
            }
            else
            {
                L_up.set(0, 1, 0);
                if (_abs(L_up.dotproduct(L_dir)) > .99f)
                    L_up.set(0, 0, 1);
                L_right.crossproduct(L_up, L_dir);
                L_right.normalize();
                L_up.crossproduct(L_dir, L_right);
                L_up.normalize();
            }

            Fmatrix spotView;
            spotView.build_camera_dir(L->position, L_dir, L_up);

            Fmatrix spotProj;
            float nearPlane = std::max(L->virtual_size, 0.01f);
            spotProj.build_projection(L->cone + deg2rad(3.5f), 1.f, nearPlane, range + EPS_S);

            gpu.spotVP.mul(spotProj, spotView);
        }
    }
    else
    {
        gpu.directionAndSpotScale.set(0.0f, -1.0f, 0.0f, 0.0f);
        gpu.spotParamsAndType.set(0.0f, 0.0f, 0.0f, virtSizeSq);
    }

    return gpu;
}

void ClusteredLightManager::CollectLight(const light* L)
{
    if (!L)
        return;
    auto it = m_lightToSlot.find(L);
    u32 slot;
    if (it != m_lightToSlot.end())
    {
        slot = it->second;
    }
    else if (m_lightsCPU.size() < MAX_LIGHTS)
    {
        slot = static_cast<u32>(m_lightsCPU.size());
        m_lightsCPU.emplace_back();
        m_slotOwners.push_back(L);
        m_lightToSlot[L] = slot;
    }
    else if (!m_freeSlots.empty())
    {
        slot = m_freeSlots.back();
        m_freeSlots.pop_back();
        m_lightToSlot[L] = slot;
        if (slot >= m_lightsCPU.size())
        {
            m_lightsCPU.resize(slot + 1);
            m_slotOwners.resize(slot + 1, nullptr);
        }
    }
    else
    {
        return;
    }
    m_lightsCPU[slot] = BuildGPULightData(L);
    m_slotOwners[slot] = L;
    m_numLights = static_cast<u32>(m_lightsCPU.size());
}

void ClusteredLightManager::PurgeTransientLights()
{
    for (u32 i = 0; i < (u32)m_lightsCPU.size(); ++i)
    {
        if (m_slotOwners[i])
            continue;
        if (m_lightsCPU[i].colorAndRange.w <= 1e-4f)
            continue;
        std::memset(&m_lightsCPU[i], 0, sizeof(GPULightData));
        m_freeSlots.push_back(i);
    }
}

void ClusteredLightManager::CollectLightsParallel(const xr_vector<const light*>& lights)
{
    PurgeTransientLights();

    u64 fingerprint = lights.size() * 0x9E3779B97F4A7C15ull;
    for (const light* L : lights)
        fingerprint ^= reinterpret_cast<uintptr_t>(L) + 0x9E3779B97F4A7C15ull + (fingerprint << 6) + (fingerprint >> 2);

    if (fingerprint == m_lightSetFingerprint && !m_slotOwners.empty())
    {
        for (u32 i = 0; i < (u32)m_slotOwners.size(); ++i)
        {
            if (m_slotOwners[i])
                m_lightsCPU[i] = BuildGPULightData(m_slotOwners[i]);
        }
        m_numLights = static_cast<u32>(m_lightsCPU.size());
        return;
    }

    xr_vector<const light*> sorted = lights;
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    const u32 count = std::min(static_cast<u32>(sorted.size()), MAX_LIGHTS);

    if (count == 0)
    {
        m_lightsCPU.clear();
        m_slotOwners.clear();
        m_freeSlots.clear();
        m_lightToSlot.clear();
        m_numLights = 0;
        m_lightSetFingerprint = 0;
        return;
    }

    for (u32 i = 0; i < count; i++)
    {
        const light* L = sorted[i];
        const u32 lt = L->flags.type;
        const bool isSpot = (lt == IRender_Light::SPOT || lt == IRender_Light::OMNIPART);
        if (isSpot && !L->spot_texture_name.empty())
            GetOrLoadSpotTexture(L->spot_texture_name);
    }

    xr_vector<const light*> stale;
    stale.reserve(m_lightToSlot.size());
    for (const auto& kv : m_lightToSlot)
    {
        if (!std::binary_search(sorted.begin(), sorted.end(), kv.first))
            stale.push_back(kv.first);
    }
    for (const light* L : stale)
    {
        const u32 slot = m_lightToSlot[L];
        m_lightToSlot.erase(L);
        if (slot < m_lightsCPU.size())
        {
            std::memset(&m_lightsCPU[slot], 0, sizeof(GPULightData));
            m_slotOwners[slot] = nullptr;
            m_freeSlots.push_back(slot);
        }
    }

    for (u32 i = 0; i < count; i++)
    {
        const light* L = sorted[i];
        auto it = m_lightToSlot.find(L);
        u32 slot;
        if (it != m_lightToSlot.end())
        {
            slot = it->second;
        }
        else if (m_lightsCPU.size() < MAX_LIGHTS)
        {
            slot = static_cast<u32>(m_lightsCPU.size());
            m_lightsCPU.emplace_back();
            m_slotOwners.push_back(L);
            m_lightToSlot[L] = slot;
        }
        else if (!m_freeSlots.empty())
        {
            slot = m_freeSlots.back();
            m_freeSlots.pop_back();
            m_lightToSlot[L] = slot;
            m_slotOwners[slot] = L;
        }
        else
        {
            continue;
        }
        m_lightsCPU[slot] = BuildGPULightData(L);
        m_slotOwners[slot] = L;
    }

    m_numLights = static_cast<u32>(m_lightsCPU.size());
    m_lightSetFingerprint = fingerprint;

    if (psDeviceFlags.test(rsStatistic))
    {
        for (u32 i = 0; i < count; i++)
        {
            const u32 lt = sorted[i]->flags.type;
            if (lt == IRender_Light::POINT)
                m_numPoint++;
            else if (lt == IRender_Light::SPOT)
                m_numSpot++;
            else if (lt == IRender_Light::OMNIPART)
                m_numOmni++;
        }
    }
}

bool ClusteredLightManager::HasNearbyPointLight(const Fvector& pos, float radius) const
{
    const float cover = std::max(radius, 1.0f);
    const float coverSq = cover * cover;
    for (u32 i = 0; i < (u32)m_lightsCPU.size(); ++i)
    {
        if (!m_slotOwners[i])
            continue;
        const GPULightData& L = m_lightsCPU[i];
        if (L.colorAndRange.w <= 1e-4f)
            continue;
        const float dx = pos.x - L.positionAndInvRangeSq.x;
        const float dy = pos.y - L.positionAndInvRangeSq.y;
        const float dz = pos.z - L.positionAndInvRangeSq.z;
        const float reach = std::max(L.colorAndRange.w, 1.0f);
        if (dx * dx + dy * dy + dz * dz < reach * reach * 0.36f + coverSq * 0.15f)
            return true;
    }
    return false;
}

void ClusteredLightManager::AddTransientPointLight(const Fvector& pos, const Fvector& color, float range)
{
    if (range <= 0.05f)
        return;
    if (color.x + color.y + color.z <= 1e-4f)
        return;

    GPULightData gpu{};
    const float invRangeSq = 1.0f / (range * range + 0.0001f);
    gpu.positionAndInvRangeSq.set(pos.x, pos.y, pos.z, invRangeSq);
    gpu.colorAndRange.set(color.x, color.y, color.z, range);
    gpu.directionAndSpotScale.set(0.0f, -1.0f, 0.0f, 0.0f);
    gpu.spotParamsAndType.set(1.0f, 0.0f, 0.0f, 0.36f);

    if (!m_freeSlots.empty())
    {
        const u32 slot = m_freeSlots.back();
        m_freeSlots.pop_back();
        if (slot >= m_lightsCPU.size())
        {
            m_lightsCPU.resize(slot + 1);
            m_slotOwners.resize(slot + 1, nullptr);
        }
        m_lightsCPU[slot] = gpu;
        m_slotOwners[slot] = nullptr;
    }
    else if (m_lightsCPU.size() < MAX_LIGHTS)
    {
        m_lightsCPU.push_back(gpu);
        m_slotOwners.push_back(nullptr);
    }
    else
        return;

    m_numLights = static_cast<u32>(m_lightsCPU.size());
}

void ClusteredLightManager::AddLight(const light* L, u32 type)
{
    if (m_numLights >= MAX_LIGHTS)
        return;

    m_lightsCPU.push_back(BuildGPULightData(L));
    m_numLights++;
}

void ClusteredLightManager::BuildLightBuffer(const light_Package& package)
{
    m_lightsCPU.clear();
    m_numLights = 0;
    m_lightSetFingerprint = 0;

    for (const light* L : package.v_point)
        AddLight(L, 0);

    for (const light* L : package.v_spot)
        AddLight(L, 1);

    for (const light* L : package.v_shadowed)
    {
        const u32 lightType = L->flags.type;
        if (lightType == IRender_Light::SPOT || lightType == IRender_Light::OMNIPART)
            AddLight(L, 1);
        else if (lightType == IRender_Light::POINT)
            AddLight(L, 0);
    }
}

void ClusteredLightManager::BuildDISampleTable()
{
    m_diIndicesCPU.clear();
    m_diCDFCPU.clear();
    m_diLightCount = 0;
    m_diPowerSum = 0.f;
    if (m_numLights == 0)
        return;

    float sum = 0.f;
    m_diIndicesCPU.reserve(m_numLights);
    m_diCDFCPU.reserve(m_numLights);

    for (u32 i = 0; i < m_numLights; ++i)
    {
        const GPULightData& L = m_lightsCPU[i];
        if (L.colorAndRange.w <= 1e-4f)
            continue;
        const float lum =
            L.colorAndRange.x * 0.2126f +
            L.colorAndRange.y * 0.7152f +
            L.colorAndRange.z * 0.0722f;
        const float range = std::max(L.colorAndRange.w, 0.5f);
        const Fvector pos = { L.positionAndInvRangeSq.x, L.positionAndInvRangeSq.y, L.positionAndInvRangeSq.z };
        const float distToCam = std::max(Device.vCameraPosition.distance_to(pos), 0.01f);
        float power = std::max(lum, 1e-4f) * (range * range) / std::max(distToCam * distToCam, range * range * 0.25f);
        if (L.spotParamsAndType.y > 0.5f)
            power *= 0.65f;
        sum += power;
        m_diIndicesCPU.push_back(i);
        m_diCDFCPU.push_back(sum);
    }

    m_diLightCount = static_cast<u32>(m_diIndicesCPU.size());
    m_diPowerSum = sum;
}

void ClusteredLightManager::Upload(nvrhi::ICommandList* cmdList)
{
    if (!m_lightDataBuffer || m_numLights == 0)
        return;

    BuildDISampleTable();

    cmdList->writeBuffer(m_lightDataBuffer, m_lightsCPU.data(),
        m_numLights * sizeof(GPULightData));

    if (m_diLightIndicesBuffer && m_diLightCDFBuffer && m_diLightCount > 0)
    {
        cmdList->writeBuffer(m_diLightIndicesBuffer, m_diIndicesCPU.data(),
            m_diLightCount * sizeof(u32));
        cmdList->writeBuffer(m_diLightCDFBuffer, m_diCDFCPU.data(),
            m_diLightCount * sizeof(float));
    }

    const u32 zero = 0;
    cmdList->writeBuffer(m_lightIndexCounterBuffer, &zero, sizeof(u32));
}

void ClusteredLightManager::UploadAllVisible(nvrhi::ICommandList* cmdList)
{
    if (!m_visibleLightIndicesBuffer || m_numLights == 0)
        return;

    xr_vector<u32> visibleMask(m_numLights, 0u);
    u32 liveCount = 0;
    for (u32 i = 0; i < m_numLights; i++)
    {
        if (m_lightsCPU[i].colorAndRange.w > 1e-4f)
        {
            visibleMask[i] = 1u;
            liveCount++;
        }
    }
    cmdList->writeBuffer(m_visibleLightIndicesBuffer, visibleMask.data(), m_numLights * sizeof(u32));
    cmdList->writeBuffer(m_visibleLightCountBuffer, &m_numLights, sizeof(u32));
    m_visibleLightCountCPU = liveCount;
}

ClusterCB ClusteredLightManager::BuildClusterCB(u32 screenWidth, u32 screenHeight, float zNear, float zFar)
{
    const u32 tilesX = (screenWidth + CLUSTER_TILE_SIZE - 1) / CLUSTER_TILE_SIZE;
    const u32 tilesY = (screenHeight + CLUSTER_TILE_SIZE - 1) / CLUSTER_TILE_SIZE;
    const float logRatio = static_cast<float>(CLUSTER_NUM_SLICES) / log2f(zFar / zNear);

    ClusterCB cb;
    cb.gridDims.set(static_cast<float>(tilesX), static_cast<float>(tilesY),
        static_cast<float>(CLUSTER_NUM_SLICES), static_cast<float>(m_numLights));
    cb.screenSize.set(static_cast<float>(screenWidth), static_cast<float>(screenHeight),
        1.0f / static_cast<float>(screenWidth), 1.0f / static_cast<float>(screenHeight));
    cb.depthParams.set(zNear, zFar, logRatio, static_cast<float>(CLUSTER_TILE_SIZE));
    cb.pad.set(Device.mProject._22, 0.f, 0.f, 0.f);

    m_tilesX = tilesX;
    m_tilesY = tilesY;

    return cb;
}

void ClusteredLightManager::ScheduleStatsReadback(nvrhi::ICommandList* cmdList)
{
    if (!m_visibleLightCountBuffer || !m_device)
        return;

    m_statsFrameCounter++;
    if ((m_statsFrameCounter % 30) != 0)
        return;

    nvrhi::BufferHandle& slot = m_statsReadbackBuffers[m_statsWriteSlot];
    if (!slot)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(u32);
        desc.debugName = "ClusteredLights_StatsReadback";
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        slot = m_device->createBuffer(desc);
        if (!slot)
            return;
    }

    cmdList->copyBuffer(slot, 0, m_visibleLightCountBuffer, 0, sizeof(u32));
    m_statsWriteSlot = (m_statsWriteSlot + 1) % STATS_READBACK_SLOTS;
    if (m_statsScheduled < STATS_READBACK_SLOTS)
        ++m_statsScheduled;
}

void ClusteredLightManager::ProcessStatsReadback()
{
    if (m_statsScheduled < STATS_READBACK_SLOTS || !m_device)
        return;

    nvrhi::IBuffer* oldest = m_statsReadbackBuffers[m_statsWriteSlot];
    void* mappedData = m_device->mapBuffer(oldest, nvrhi::CpuAccessMode::Read);
    if (mappedData)
    {
        m_visibleLightCountCPU = *static_cast<const u32*>(mappedData);
        m_device->unmapBuffer(oldest);
    }
}

u32 ClusteredLightManager::GetOrLoadSpotTexture(const shared_str& name)
{
    auto it = m_spotTextureCache.find(name);
    if (it != m_spotTextureCache.end())
        return it->second;

    auto* renderDevice = GEnv.Render ? GEnv.Render->GetRenderDevice() : nullptr;
    if (!renderDevice)
        return INVALID_TEXTURE_INDEX;

    auto* resMgr = renderDevice->GetFGResourceManager();
    auto* backend = renderDevice->GetBackend();
    if (!resMgr || !backend)
        return INVALID_TEXTURE_INDEX;

    auto* texManager = resMgr->GetTextureManager();
    if (!texManager)
        return INVALID_TEXTURE_INDEX;

    auto handle = texManager->LoadTexture(name.c_str());
    if (!handle.IsValid())
    {
        m_spotTextureCache[name] = INVALID_TEXTURE_INDEX;
        return INVALID_TEXTURE_INDEX;
    }

    nvrhi::ITexture* nvrhiTex = texManager->GetNVRHITexture(handle);
    if (!nvrhiTex)
    {
        m_spotTextureCache[name] = INVALID_TEXTURE_INDEX;
        return INVALID_TEXTURE_INDEX;
    }

    u32 bindlessIdx = backend->RegisterBindlessTexture(nvrhiTex);
    m_spotTextureCache[name] = bindlessIdx;
    return bindlessIdx;
}

}
