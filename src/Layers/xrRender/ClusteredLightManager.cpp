#include "stdafx.h"
#include "ClusteredLightManager.h"
#include "light.h"
#include "Light_Package.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "xrEngine/IRenderBackend.h"
#include "xrCore/Threading/ParallelFor.hpp"

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
    m_lightsCPU.reserve(INITIAL_LIGHT_CAPACITY);
    m_lightIDs.reserve(INITIAL_LIGHT_CAPACITY);
    m_lightCapacity = INITIAL_LIGHT_CAPACITY;
    m_identityIndices.resize(m_lightCapacity);

    for (u32 i = 0; i < INITIAL_LIGHT_CAPACITY; i++)
        m_identityIndices[i] = i;

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = INITIAL_LIGHT_CAPACITY * sizeof(GPULightData);
        desc.structStride = sizeof(GPULightData);
        desc.debugName = "ClusteredLights_LightData";
        desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
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
        desc.byteSize = INITIAL_LIGHT_CAPACITY * sizeof(u32);
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

    Msg("* [ClusteredLights] Created GPU buffers (max %u lights, %u max clusters)",
        INITIAL_LIGHT_CAPACITY, maxClusters);
}

void ClusteredLightManager::Shutdown()
{
    m_lightDataBuffer = nullptr;
    m_clusterGridBuffer = nullptr;
    m_lightIndexListBuffer = nullptr;
    m_lightIndexCounterBuffer = nullptr;
    m_visibleLightIndicesBuffer = nullptr;
    m_visibleLightCountBuffer = nullptr;
    for (u32 i = 0; i < STATS_READBACK_SLOTS; ++i)
        m_statsReadbackBuffers[i] = nullptr;
    m_statsWriteSlot = 0;
    m_statsScheduled = 0;
    m_visibleLightCountCPU = 0;
    m_lightsThisFrame.clear();
    for (auto& snapshot : m_lightSnapshots)
        snapshot.clear();
    m_culledLights.clear();
    m_lightsCPU.clear();
    m_lightIDs.clear();
    ReleaseSpotTextures();
    m_device = nullptr;
}

void ClusteredLightManager::EnsureLightCapacity(u32 count)
{
    if (count <= m_lightCapacity)
        return;
    R_ASSERT(m_device);
    u32 capacity = std::max(1u, m_lightCapacity);
    while (capacity < count)
        capacity *= 2u;
    auto lightDesc = m_lightDataBuffer->getDesc();
    lightDesc.byteSize = u64(capacity) * sizeof(GPULightData);
    auto visibleDesc = m_visibleLightIndicesBuffer->getDesc();
    visibleDesc.byteSize = u64(capacity) * sizeof(u32);
    auto lights = m_device->createBuffer(lightDesc);
    auto visible = m_device->createBuffer(visibleDesc);
    R_ASSERT2(lights && visible, "Cannot grow the light buffers without dropping lights");
    m_lightDataBuffer = lights;
    m_visibleLightIndicesBuffer = visible;
    m_identityIndices.resize(capacity);
    for (u32 i = m_lightCapacity; i < capacity; ++i)
        m_identityIndices[i] = i;
    m_lightCapacity = capacity;
}

void ClusteredLightManager::BeginFrame(bool rayTracingLighting)
{
    m_lightsCPU.clear();
    m_lightIDs.clear();
    m_rayTracingLighting = rayTracingLighting;
    m_numLights = 0;
    m_numPoint = 0;
    m_numSpot = 0;
    m_numOmni = 0;
}

u32 ClusteredLightManager::ResolveSpotTexture(const light* L)
{
    const u32 lightType = L->flags.type;
    const bool isSpot = (lightType == IRender_Light::SPOT || lightType == IRender_Light::OMNIPART);
    if (!isSpot || L->spot_texture_name.empty())
        return 0;
    return GetOrLoadSpotTexture(L->spot_texture_name);
}

GPULightData ClusteredLightManager::BuildGPULightData(const light* L, u32 shadowSlot, u32 spotTexture) const
{
    GPULightData gpu;

    const float range = L->range;
    const float invRangeSq = 1.0f / (range * range + 0.0001f);

    gpu.positionAndInvRangeSq.set(L->position.x, L->position.y, L->position.z, invRangeSq);
    const Fvector color = SrgbToLinear(Fvector().set(L->color.r, L->color.g, L->color.b));
    const float lod = m_rayTracingLighting ? 1.0f : L->get_LOD();
    gpu.colorAndRange.set(color.x * lod, color.y * lod, color.z * lod, range);

    std::memset(&gpu.spotVP, 0, sizeof(gpu.spotVP));

    const u32 lightType = L->flags.type;
    const bool isSpot = (lightType == IRender_Light::SPOT || lightType == IRender_Light::OMNIPART);

    if (isSpot)
    {
        const float cosOuter = _cos(L->cone * 0.5f);
        const float cosInner = _cos(L->cone * 0.4f);
        const float scale = 1.0f / std::max(cosInner - cosOuter, 0.001f);
        const float offset = -cosOuter * scale;

        const u32 texIdx = spotTexture;

        gpu.directionAndSpotScale.set(L->direction.x, L->direction.y, L->direction.z, scale);

        float texIdxBits;
        std::memcpy(&texIdxBits, &texIdx, sizeof(float));
        gpu.spotParamsAndType.set(offset, 1.0f, texIdxBits, float(shadowSlot));

        if (texIdx != 0)
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
        gpu.spotParamsAndType.set(0.0f, 0.0f, 0.0f, float(shadowSlot));
    }

    return gpu;
}

void ClusteredLightManager::CollectLight(const light* L)
{
    EnsureLightCapacity(m_numLights + 1u);

    m_lightsCPU.push_back(BuildGPULightData(L, 0, ResolveSpotTexture(L)));
    m_lightIDs.push_back(L->GetLightID());
    m_numLights++;
}

void ClusteredLightManager::CollectLightsParallel(const xr_vector<const light*>& lights, const xr_vector<u32>& shadowSlots)
{
    const u32 count = static_cast<u32>(lights.size());
    if (count == 0)
        return;
    EnsureLightCapacity(count);

    m_spotTextureScratch.resize(count);
    for (u32 i = 0; i < count; i++)
        m_spotTextureScratch[i] = ResolveSpotTexture(lights[i]);

    m_lightsCPU.resize(count);
    m_lightIDs.resize(count);
    m_numLights = count;
    m_lightsThisFrame = lights;

    xr_parallel_for(TaskRange<u32>(0, count), [&](const TaskRange<u32>& range) {
        for (u32 i = range.begin(); i != range.end(); ++i)
        {
            m_lightsCPU[i] = BuildGPULightData(lights[i], i < shadowSlots.size() ? shadowSlots[i] : 0u, m_spotTextureScratch[i]);
            m_lightIDs[i] = lights[i]->GetLightID();
        }
    });

    if (psDeviceFlags.test(rsStatistic))
    {
        for (u32 i = 0; i < count; i++)
        {
            const u32 lt = lights[i]->flags.type;
            if (lt == IRender_Light::POINT)
                m_numPoint++;
            else if (lt == IRender_Light::SPOT)
                m_numSpot++;
            else if (lt == IRender_Light::OMNIPART)
                m_numOmni++;
        }
    }
}

void ClusteredLightManager::AddLight(const light* L, u32 type)
{
    EnsureLightCapacity(m_numLights + 1u);

    m_lightsCPU.push_back(BuildGPULightData(L, 0, ResolveSpotTexture(L)));
    m_lightIDs.push_back(L->GetLightID());
    m_numLights++;
}

void ClusteredLightManager::BuildLightBuffer(const light_Package& package)
{
    m_lightsCPU.clear();
    m_lightIDs.clear();
    m_numLights = 0;

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

void ClusteredLightManager::Upload(nvrhi::ICommandList* cmdList)
{
    if (!m_lightDataBuffer || m_numLights == 0)
        return;

    cmdList->writeBuffer(m_lightDataBuffer, m_lightsCPU.data(),
        m_numLights * sizeof(GPULightData));

    const u32 zero = 0;
    cmdList->writeBuffer(m_lightIndexCounterBuffer, &zero, sizeof(u32));
}

void ClusteredLightManager::UploadAllVisible(nvrhi::ICommandList* cmdList)
{
    if (!m_visibleLightIndicesBuffer || m_numLights == 0)
        return;

    cmdList->writeBuffer(m_visibleLightIndicesBuffer, m_identityIndices.data(), m_numLights * sizeof(u32));
    cmdList->writeBuffer(m_visibleLightCountBuffer, &m_numLights, sizeof(u32));
    m_visibleLightCountCPU = m_numLights;
    m_culledLights.clear();
}

u64 ClusteredLightManager::GetLightID(u32 index) const
{
    return index < m_lightIDs.size() ? m_lightIDs[index] : INVALID_LIGHT_ID;
}

u32 ClusteredLightManager::FindLightIndex(u64 id) const
{
    for (u32 i = 0; i < m_lightIDs.size(); ++i)
        if (m_lightIDs[i] == id)
            return i;
    return INVALID_LIGHT_INDEX;
}

u64 ClusteredLightManager::GetTransportSignature() const
{
    xr_set<nvrhi::ITexture*> textures;
    u64 signature = 0;
    for (size_t index = 0; index < m_lightsCPU.size(); ++index)
    {
        auto transport = m_lightsCPU[index];
        transport.spotParamsAndType.w = 0.0f;
        u64 lightSignature = 14695981039346656037ull;
        const auto append = [&](const void* data, size_t size)
        {
            const auto* bytes = static_cast<const u8*>(data);
            for (size_t i = 0; i < size; ++i)
            {
                lightSignature ^= bytes[i];
                lightSignature *= 1099511628211ull;
            }
        };
        const u64 lightID = index < m_lightIDs.size() ? m_lightIDs[index] : INVALID_LIGHT_ID;
        append(&lightID, sizeof(lightID));
        append(&transport, sizeof(transport));
        u32 textureIndex = 0;
        std::memcpy(&textureIndex, &transport.spotParamsAndType.z, sizeof(textureIndex));
        if (textureIndex && GEnv.Backend)
        {
            auto* texture = GEnv.Backend->GetBindlessTexture(textureIndex);
            append(&texture, sizeof(texture));
            if (texture)
                textures.insert(texture);
        }
        signature += lightSignature;
    }
    auto* device = GEnv.Render ? GEnv.Render->GetRenderDevice() : nullptr;
    auto* resources = device ? device->GetFGResourceManager() : nullptr;
    auto* textureManager = resources ? resources->GetTextureManager() : nullptr;
    const u64 textureRevision = textureManager ? textureManager->GetContentRevision(textures) : 0;
    signature ^= u64(m_numLights) * 1099511628211ull;
    signature ^= textureRevision + 0x9e3779b97f4a7c15ull + (signature << 6) + (signature >> 2);
    return signature;
}

ClusterCB ClusteredLightManager::BuildClusterCB(u32 screenWidth, u32 screenHeight, float zNear, float zFar) const
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
    cb.pad.set(0, 0, 0, 0);

    const_cast<ClusteredLightManager*>(this)->m_tilesX = tilesX;
    const_cast<ClusteredLightManager*>(this)->m_tilesY = tilesY;

    return cb;
}

void ClusteredLightManager::ScheduleStatsReadback(nvrhi::ICommandList* cmdList)
{
    if (!m_visibleLightCountBuffer || !m_device)
        return;

    nvrhi::BufferHandle& slot = m_statsReadbackBuffers[m_statsWriteSlot];
    const u64 byteSize = u64(1 + m_numLights) * sizeof(u32);
    if (!slot || slot->getDesc().byteSize < byteSize)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = byteSize;
        desc.debugName = "ClusteredLights_StatsReadback";
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        slot = m_device->createBuffer(desc);
        if (!slot)
            return;
    }

    cmdList->copyBuffer(slot, 0, m_visibleLightCountBuffer, 0, sizeof(u32));
    if (m_numLights)
        cmdList->copyBuffer(slot, sizeof(u32), m_visibleLightIndicesBuffer, 0, u64(m_numLights) * sizeof(u32));
    m_lightSnapshots[m_statsWriteSlot] = m_lightsThisFrame;
    m_statsWriteSlot = (m_statsWriteSlot + 1) % STATS_READBACK_SLOTS;
    if (m_statsScheduled < STATS_READBACK_SLOTS)
        ++m_statsScheduled;
}

void ClusteredLightManager::ProcessStatsReadback()
{
    if (m_statsScheduled < STATS_READBACK_SLOTS || !m_device)
        return;

    nvrhi::IBuffer* oldest = m_statsReadbackBuffers[m_statsWriteSlot];
    const u32* words = static_cast<const u32*>(m_device->mapBuffer(oldest, nvrhi::CpuAccessMode::Read));
    if (!words)
        return;
    const xr_vector<const light*>& snapshot = m_lightSnapshots[m_statsWriteSlot];
    const u32 total = u32(snapshot.size());
    m_visibleLightCountCPU = std::min(words[0], total);
    xr_vector<bool> visible(total, false);
    for (u32 i = 0; i < m_visibleLightCountCPU; ++i)
        if (words[1 + i] < total)
            visible[words[1 + i]] = true;
    m_culledLights.clear();
    for (u32 i = 0; i < total; ++i)
        if (!visible[i])
            m_culledLights.push_back(snapshot[i]);
    m_device->unmapBuffer(oldest);
}

u32 ClusteredLightManager::GetOrLoadSpotTexture(const shared_str& name)
{
    auto it = m_spotTextureCache.find(name);
    if (it != m_spotTextureCache.end())
        return it->second;

    auto* renderDevice = GEnv.Render ? GEnv.Render->GetRenderDevice() : nullptr;
    auto* resMgr = renderDevice ? renderDevice->GetFGResourceManager() : nullptr;
    auto* backend = renderDevice ? renderDevice->GetBackend() : nullptr;
    auto* texManager = resMgr ? resMgr->GetTextureManager() : nullptr;
    if (!texManager || !backend)
        return 0;

    u32 bindlessIdx = 0;
    const resources::TextureHandle handle = texManager->LoadTexture(name.c_str(), TextureColorSpace::Srgb);
    if (handle.IsValid())
    {
        const u32 registered = backend->RegisterBindlessTexture(texManager->GetNVRHITexture(handle));
        if (registered != UINT32_MAX && registered != 0)
        {
            bindlessIdx = registered;
            m_spotTextureHandles.push_back(handle);
            m_spotTextureIndices.push_back(registered);
        }
        else
        {
            if (registered != UINT32_MAX)
                backend->ReleaseBindlessTextures(&registered, 1);
            texManager->Release(handle);
        }
    }

    m_spotTextureCache[name] = bindlessIdx;
    return bindlessIdx;
}

void ClusteredLightManager::ReleaseSpotTextures()
{
    auto* renderDevice = GEnv.Render ? GEnv.Render->GetRenderDevice() : nullptr;
    auto* resMgr = renderDevice ? renderDevice->GetFGResourceManager() : nullptr;
    auto* backend = renderDevice ? renderDevice->GetBackend() : nullptr;
    auto* texManager = resMgr ? resMgr->GetTextureManager() : nullptr;
    if (backend && !m_spotTextureIndices.empty())
        backend->ReleaseBindlessTextures(m_spotTextureIndices.data(), u32(m_spotTextureIndices.size()));
    if (texManager)
    {
        for (const resources::TextureHandle& handle : m_spotTextureHandles)
            texManager->Release(handle);
    }
    m_spotTextureIndices.clear();
    m_spotTextureHandles.clear();
    m_spotTextureCache.clear();
}

}
