#include "stdafx.h"
#include "ClusteredLightManager.h"
#include "light.h"
#include "Light_Package.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrEngine/IRenderBackend.h"
#include "xrCore/Threading/ParallelFor.hpp"
#include <algorithm>
#include <cstring>

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
    for (u32 i = 0; i < STATS_READBACK_SLOTS; ++i)
        m_statsReadbackBuffers[i] = nullptr;
    m_statsWriteSlot = 0;
    m_statsScheduled = 0;
    m_visibleLightCountCPU = 0;
    m_lightsCPU.clear();
    m_lightSources.clear();
    m_localShadowTiles.clear();
    m_stickyShadowSlots = {};
    m_spotTextureCache.clear();
    m_device = nullptr;
}

void ClusteredLightManager::BeginFrame()
{
    m_lightsCPU.clear();
    m_lightSources.clear();
    m_localShadowTiles.clear();
    m_numLights = 0;
    m_numPoint = 0;
    m_numSpot = 0;
    m_numOmni = 0;
}

Fmatrix ClusteredLightManager::BuildSpotClipVP(const light* L)
{
    Fmatrix spotVP;
    spotVP.identity();
    if (!L)
        return spotVP;

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
    // Classic Light_Render_Direct_ComputeXFS: near=virtual_size, far=range,
    // FOV pad +3.5° (spot) / +11.5° (point faces → OMNIPART).
    const float nearPlane = std::max(L->virtual_size, 0.05f);
    const float tan_shift = (L->flags.type == IRender_Light::OMNIPART)
        ? deg2rad(11.5f)
        : deg2rad(3.5f);
    const float fov = std::min(L->cone + tan_shift, PI * 0.98f);
    const float farPlane = std::max(L->range + EPS_S, nearPlane + 0.5f);
    spotProj.build_projection(fov, 1.f, nearPlane, farPlane);

    spotVP.mul(spotProj, spotView);
    return spotVP;
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
        // .w = local shadow tile (asuint): 0 = none; set later by AssignLocalShadowTiles
        gpu.spotParamsAndType.set(offset, 1.0f, texIdxBits, 0.0f);
        gpu.spotVP = BuildSpotClipVP(L);
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
    m_lightSources.push_back(L);
    m_numLights++;
}

void ClusteredLightManager::CollectLightsParallel(const xr_vector<const light*>& lights)
{
    if (lights.empty())
        return;

    // Expand shadowed POINT → 6 OMNIPART faces (classic Export path).
    xr_vector<const light*> expanded;
    expanded.reserve(lights.size() + 32);
    for (const light* L : lights)
    {
        if (!L)
            continue;
        if (L->flags.type == IRender_Light::POINT && L->flags.bShadow)
        {
            light* mutableL = const_cast<light*>(L);
            mutableL->EnsureOmniparts();
            for (int f = 0; f < 6; ++f)
            {
                if (L->omnipart[f])
                    expanded.push_back(L->omnipart[f]);
            }
        }
        else
        {
            expanded.push_back(L);
        }
    }

    const u32 count = std::min(static_cast<u32>(expanded.size()), MAX_LIGHTS);
    if (count == 0)
        return;

    for (u32 i = 0; i < count; i++)
    {
        const light* L = expanded[i];
        const u32 lt = L->flags.type;
        const bool isSpot = (lt == IRender_Light::SPOT || lt == IRender_Light::OMNIPART);
        if (isSpot && !L->spot_texture_name.empty())
            GetOrLoadSpotTexture(L->spot_texture_name);
    }

    m_lightsCPU.resize(count);
    m_lightSources.resize(count);
    m_numLights = count;

    for (u32 i = 0; i < count; ++i)
        m_lightSources[i] = expanded[i];

    xr_parallel_for(TaskRange<u32>(0, count), [&](const TaskRange<u32>& range) {
        for (u32 i = range.begin(); i != range.end(); ++i)
            m_lightsCPU[i] = BuildGPULightData(expanded[i]);
    });

    if (psDeviceFlags.test(rsStatistic))
    {
        for (u32 i = 0; i < count; i++)
        {
            const u32 lt = expanded[i]->flags.type;
            if (lt == IRender_Light::POINT)
                m_numPoint++;
            else if (lt == IRender_Light::SPOT)
                m_numSpot++;
            else if (lt == IRender_Light::OMNIPART)
                m_numOmni++;
        }
    }
}

void ClusteredLightManager::AssignLocalShadowTiles(const Fvector& cameraPos)
{
    m_localShadowTiles.clear();
    if (ps_r_local_shadows == 0 || m_numLights == 0 || m_lightSources.size() != m_numLights)
    {
        m_stickyShadowSlots = {};
        return;
    }

    struct Candidate
    {
        u32 idx = 0;
        float score = 0.f;
        Fmatrix clipVP;
        const light* L = nullptr;
    };

    // light* → candidate (first occurrence wins; OMNIPART faces are distinct lights)
    xr_map<const light*, Candidate> byLight;

    for (u32 i = 0; i < m_numLights; ++i)
    {
        const light* L = m_lightSources[i];
        if (!L || !L->flags.bShadow)
            continue;
        const u32 lt = L->flags.type;
        if (lt != IRender_Light::SPOT && lt != IRender_Light::OMNIPART)
            continue;

        const float distSq = cameraPos.distance_to_sqr(L->position);
        const float intensity = std::max({L->color.r, L->color.g, L->color.b, 0.01f});
        float score = intensity * L->range / (1.f + distSq);
        // Static level lights (Light_DB) — room fixtures; keep them over the torch.
        if (L->flags.bStatic)
            score *= 2.5f;
        // OMNIPART = POINT faces (Skadovsk cabin lamps). Slightly below spots but not starved.
        if (lt == IRender_Light::OMNIPART)
            score *= 0.85f;
        // Downward faces → long floor shadows from beds/tables.
        if (L->direction.y < -0.35f)
            score *= 1.5f;
        // Player torch: close dynamic SPOT — don't let it eat every atlas slot indoors.
        if (lt == IRender_Light::SPOT && !L->flags.bStatic && distSq < 16.f)
            score *= 0.35f;

        Candidate c;
        c.idx = i;
        c.score = score;
        c.clipVP = m_lightsCPU[i].spotVP;
        c.L = L;
        byLight.emplace(L, c);
    }

    if (byLight.empty())
    {
        m_stickyShadowSlots = {};
        return;
    }

    // Find admission floor from a fresh top-N ranking (for hysteresis).
    xr_vector<Candidate> ranked;
    ranked.reserve(byLight.size());
    for (auto& kv : byLight)
        ranked.push_back(kv.second);
    std::sort(ranked.begin(), ranked.end(), [](const Candidate& a, const Candidate& b) {
        if (a.score != b.score)
            return a.score > b.score;
        return a.L < b.L; // stable tie-break
    });

    const u32 tileCap = std::clamp(static_cast<u32>(std::max(ps_r_local_shadow_tiles, 1)), 1u, MAX_LOCAL_SHADOW_TILES);
    const u32 nWant = std::min(static_cast<u32>(ranked.size()), tileCap);
    const float admitScore = (nWant > 0) ? ranked[nWant - 1].score : 0.f;
    // Wide hysteresis: score flickers every frame as you walk — 0.7 was still thrashing
    // (whole room shadows popping on/off right in front of the camera).
    const float keepScore = admitScore * 0.35f;
    constexpr u32 kStickyGraceFrames = 45;

    std::array<StickyShadowSlot, MAX_LOCAL_SHADOW_TILES> nextSticky{};
    xr_vector<bool> sliceTaken(MAX_LOCAL_SHADOW_TILES, false);
    xr_map<const light*, u32> assigned; // light* → slice

    auto packTile = [&](u32 lightIdx, u32 slice, const light* L, const Fmatrix& clipVP, float score, u32 grace) {
        u32 tilePlusOne = slice + 1;
        float tileBits;
        std::memcpy(&tileBits, &tilePlusOne, sizeof(float));
        m_lightsCPU[lightIdx].spotParamsAndType.w = tileBits;

        LocalShadowTile tile;
        tile.L = L;
        tile.clipVP = clipVP;
        tile.lightIndex = lightIdx;
        tile.slice = slice;
        m_localShadowTiles.push_back(tile);

        nextSticky[slice] = {L, score, grace};
        sliceTaken[slice] = true;
        assigned[L] = slice;
    };

    // 1) Retain previous sticky occupants (score hysteresis + grace frames).
    for (u32 s = 0; s < tileCap; ++s)
    {
        const StickyShadowSlot& prev = m_stickyShadowSlots[s];
        if (!prev.L)
            continue;
        auto it = byLight.find(prev.L);
        if (it == byLight.end())
        {
            // Light briefly left the clustered list — reserve the slice so it isn't
            // stolen; resume packing when the light returns (kills on/off flicker).
            if (prev.graceFrames == 0)
                continue;
            nextSticky[s] = {prev.L, prev.score, prev.graceFrames - 1};
            sliceTaken[s] = true;
            continue;
        }
        const bool eligible = it->second.score >= keepScore;
        if (!eligible && prev.graceFrames == 0)
            continue;
        const u32 grace = eligible ? kStickyGraceFrames
                                   : (prev.graceFrames > 0 ? prev.graceFrames - 1 : 0);
        packTile(it->second.idx, s, prev.L, it->second.clipVP, it->second.score, grace);
    }

    // 2) Fill free slices from ranked list (skip already assigned).
    for (const Candidate& c : ranked)
    {
        if (assigned.find(c.L) != assigned.end())
            continue;
        u32 freeSlice = tileCap;
        for (u32 s = 0; s < tileCap; ++s)
        {
            if (!sliceTaken[s])
            {
                freeSlice = s;
                break;
            }
        }
        if (freeSlice >= tileCap)
            break;
        packTile(c.idx, freeSlice, c.L, c.clipVP, c.score, kStickyGraceFrames);
    }

    m_stickyShadowSlots = nextSticky;
}

void ClusteredLightManager::AddLight(const light* L, u32 type)
{
    if (m_numLights >= MAX_LIGHTS)
        return;

    m_lightsCPU.push_back(BuildGPULightData(L));
    m_lightSources.push_back(L);
    m_numLights++;
}

void ClusteredLightManager::BuildLightBuffer(const light_Package& package)
{
    m_lightsCPU.clear();
    m_lightSources.clear();
    m_localShadowTiles.clear();
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
