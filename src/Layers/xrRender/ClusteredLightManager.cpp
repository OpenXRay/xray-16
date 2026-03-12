#include "stdafx.h"
#include "ClusteredLightManager.h"
#include "light.h"
#include "Light_Package.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "xrEngine/IRenderBackend.h"
#include "xrCore/Threading/ParallelFor.hpp"

namespace xray::render::RENDER_NAMESPACE
{

ClusteredLightManager& ClusteredLightManager::Instance()
{
    static ClusteredLightManager instance;
    return instance;
}

void ClusteredLightManager::Initialize(ng::RenderDevice* device)
{
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    m_device = nvDevice;
    m_lightsCPU.reserve(MAX_LIGHTS);

    for (u32 i = 0; i < MAX_LIGHTS; i++)
        m_identityIndices[i] = i;

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
    m_statsReadbackBuffer = nullptr;
    m_statsReadbackPending = false;
    m_visibleLightCountCPU = 0;
    m_lightsCPU.clear();
    m_spotTextureCache.clear();
    m_device = nullptr;
}

void ClusteredLightManager::BeginFrame()
{
    m_lightsCPU.clear();
    m_numLights = 0;
    m_numPoint = 0;
    m_numSpot = 0;
    m_numOmni = 0;
}

GPULightData ClusteredLightManager::BuildGPULightData(const light* L)
{
    GPULightData gpu;

    const float range = L->range;
    const float invRangeSq = 1.0f / (range * range + 0.0001f);

    gpu.positionAndInvRangeSq.set(L->position.x, L->position.y, L->position.z, invRangeSq);
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

        u32 texIdx = 0;
        if (!L->spot_texture_name.empty())
            texIdx = GetOrLoadSpotTexture(L->spot_texture_name);

        gpu.directionAndSpotScale.set(L->direction.x, L->direction.y, L->direction.z, scale);

        float texIdxBits;
        std::memcpy(&texIdxBits, &texIdx, sizeof(float));
        gpu.spotParamsAndType.set(offset, 1.0f, texIdxBits, 0.0f);

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
        gpu.spotParamsAndType.set(0.0f, 0.0f, 0.0f, 0.0f);
    }

    return gpu;
}

void ClusteredLightManager::CollectLight(const light* L)
{
    if (m_numLights >= MAX_LIGHTS)
        return;

    m_lightsCPU.push_back(BuildGPULightData(L));
    m_numLights++;
}

void ClusteredLightManager::CollectLightsParallel(const xr_vector<const light*>& lights)
{
    const u32 count = std::min(static_cast<u32>(lights.size()), MAX_LIGHTS);
    if (count == 0)
        return;

    for (u32 i = 0; i < count; i++)
    {
        const light* L = lights[i];
        const u32 lt = L->flags.type;
        const bool isSpot = (lt == IRender_Light::SPOT || lt == IRender_Light::OMNIPART);
        if (isSpot && !L->spot_texture_name.empty())
            GetOrLoadSpotTexture(L->spot_texture_name);
    }

    m_lightsCPU.resize(count);
    m_numLights = count;

    xr_parallel_for(TaskRange<u32>(0, count), [&](const TaskRange<u32>& range) {
        for (u32 i = range.begin(); i != range.end(); ++i)
            m_lightsCPU[i] = BuildGPULightData(lights[i]);
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
    if (m_numLights >= MAX_LIGHTS)
        return;

    m_lightsCPU.push_back(BuildGPULightData(L));
    m_numLights++;
}

void ClusteredLightManager::BuildLightBuffer(const light_Package& package)
{
    m_lightsCPU.clear();
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
    if (!m_visibleLightCountBuffer || !m_device || m_statsReadbackPending)
        return;

    m_statsFrameCounter++;
    if ((m_statsFrameCounter % 30) != 0)
        return;

    if (!m_statsReadbackBuffer)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(u32);
        desc.debugName = "ClusteredLights_StatsReadback";
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        m_statsReadbackBuffer = m_device->createBuffer(desc);
        if (!m_statsReadbackBuffer)
            return;
    }

    cmdList->copyBuffer(m_statsReadbackBuffer, 0, m_visibleLightCountBuffer, 0, sizeof(u32));
    m_statsReadbackPending = true;
}

void ClusteredLightManager::ProcessStatsReadback()
{
    if (!m_statsReadbackPending || !m_statsReadbackBuffer || !m_device)
        return;

    void* mappedData = m_device->mapBuffer(m_statsReadbackBuffer, nvrhi::CpuAccessMode::Read);
    if (mappedData)
    {
        m_visibleLightCountCPU = *static_cast<const u32*>(mappedData);
        m_device->unmapBuffer(m_statsReadbackBuffer);
    }

    m_statsReadbackPending = false;
}

u32 ClusteredLightManager::GetOrLoadSpotTexture(const shared_str& name)
{
    auto it = m_spotTextureCache.find(name);
    if (it != m_spotTextureCache.end())
        return it->second;

    auto* renderDevice = RImplementation.m_renderDevice;
    if (!renderDevice)
        return 0;

    auto* resMgr = renderDevice->GetFGResourceManager();
    auto* backend = renderDevice->GetBackend();
    if (!resMgr || !backend)
        return 0;

    auto* texManager = resMgr->GetTextureManager();
    if (!texManager)
        return 0;

    auto handle = texManager->LoadTexture(name.c_str());
    if (!handle.IsValid())
    {
        m_spotTextureCache[name] = 0;
        return 0;
    }

    nvrhi::ITexture* nvrhiTex = texManager->GetNVRHITexture(handle);
    if (!nvrhiTex)
    {
        m_spotTextureCache[name] = 0;
        return 0;
    }

    u32 bindlessIdx = backend->RegisterBindlessTexture(nvrhiTex);
    m_spotTextureCache[name] = bindlessIdx;
    return bindlessIdx;
}

}
