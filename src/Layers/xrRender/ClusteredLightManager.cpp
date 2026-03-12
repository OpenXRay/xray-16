#include "stdafx.h"
#include "ClusteredLightManager.h"
#include "light.h"
#include "Light_Package.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

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

    Msg("* [ClusteredLights] Created GPU buffers (max %u lights, %u max clusters)",
        MAX_LIGHTS, maxClusters);
}

void ClusteredLightManager::Shutdown()
{
    m_lightDataBuffer = nullptr;
    m_clusterGridBuffer = nullptr;
    m_lightIndexListBuffer = nullptr;
    m_lightIndexCounterBuffer = nullptr;
    m_lightsCPU.clear();
    m_device = nullptr;
}

void ClusteredLightManager::AddLight(const light* L, u32 type)
{
    if (m_numLights >= MAX_LIGHTS)
        return;

    GPULightData gpu;

    const float range = L->range;
    const float invRangeSq = 1.0f / (range * range + 0.0001f);

    gpu.positionAndInvRangeSq.set(L->position.x, L->position.y, L->position.z, invRangeSq);
    gpu.colorAndRange.set(L->color.r, L->color.g, L->color.b, range);

    if (type == 1)
    {
        const float cosOuter = _cos(L->cone);
        const float cosInner = _cos(L->cone * 0.8f);
        const float scale = 1.0f / std::max(cosInner - cosOuter, 0.001f);
        const float offset = -cosOuter * scale;

        gpu.directionAndSpotScale.set(L->direction.x, L->direction.y, L->direction.z, scale);
        gpu.spotParamsAndType.set(offset, 1.0f, 0.0f, 0.0f);
        gpu.spotVP = Fidentity;
    }
    else
    {
        gpu.directionAndSpotScale.set(0.0f, -1.0f, 0.0f, 0.0f);
        gpu.spotParamsAndType.set(0.0f, 0.0f, 0.0f, 0.0f);
        gpu.spotVP = Fidentity;
    }

    m_lightsCPU.push_back(gpu);
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

}
