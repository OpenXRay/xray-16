#include "stdafx.h"
#include "ClusteredLightManager.h"
#include "light.h"
#include "Light_Package.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/Light_Render_Direct.h"
#include "Layers/xrRender/r2_types.h"
#include "Layers/xrRender/MaxRectsAllocator.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "xrEngine/IRenderBackend.h"
#include "xrCDB/Frustum.h"
#include "xrCore/Threading/ParallelFor.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <climits>

namespace xray::render::fg
{

namespace
{
bool LightSphereOnScreen(const Fvector& center, float radius, const Fmatrix& fullXform, float ndcPad = 0.f)
{
    Fvector4 clip;
    fullXform.transform(clip, center);
    if (clip.w <= 1e-4f)
        return Device.vCameraPosition.distance_to(center) <= (radius + 1.f);

    const float invW = 1.f / clip.w;
    const float ndcX = clip.x * invW;
    const float ndcY = clip.y * invW;
    const float extent = radius * invW *
        std::max(std::abs(Device.mProject._11), std::abs(Device.mProject._22));
    const float pad = std::max(0.f, ndcPad);
    const float minX = ndcX - extent;
    const float maxX = ndcX + extent;
    const float minY = ndcY - extent;
    const float maxY = ndcY + extent;
    return !(maxX < -1.f - pad || minX > 1.f + pad || maxY < -1.f - pad || minY > 1.f + pad);
}

bool OmniFaceUsefulForCamera(const light* L, const Fvector& cameraPos, const CFrustum& camFrustum)
{
    if (!L)
        return false;
    if (!camFrustum.testSphere_dirty(L->spatial.sphere.P, L->spatial.sphere.R))
        return false;

    Fvector toCam;
    toCam.sub(cameraPos, L->position);
    const float distCam = toCam.magnitude();
    if (distCam <= L->range * 0.9f)
        return true;
    if (distCam > 1e-3f)
    {
        toCam.mul(1.f / distCam);
        if (L->direction.dotproduct(toCam) < -0.2f)
            return false;
    }
    return true;
}
} // namespace

ClusteredLightManager& ClusteredLightManager::Instance()
{
    static ClusteredLightManager instance;
    return instance;
}

u32 ClusterTileSize()
{
    return (ps_r_cluster_tile_size <= 32) ? 32u : 64u;
}

u32 LocalShadowAtlasSize()
{
    u32 s = static_cast<u32>(ps_r_local_shadow_atlas);
    if (s <= 1024u)
        return 1024u;
    if (s <= 2048u)
        return 2048u;
    if (s <= 4096u)
        return 4096u;
    return 8192u;
}

u32 LocalShadowPageCount()
{
    return (LocalShadowAtlasSize() >= 4096u) ? 1u : MAX_LOCAL_SHADOW_PAGES;
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

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = MAX_LOCAL_SHADOW_TILES * sizeof(GPUShadowData);
        desc.structStride = sizeof(GPUShadowData);
        desc.debugName = "ClusteredLights_ShadowData";
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        desc.canHaveTypedViews = false;
        desc.canHaveUAVs = false;
        m_shadowDataBuffer = nvDevice->createBuffer(desc);
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
        desc.canHaveTypedViews = false;
        desc.canHaveUAVs = false;
        m_diLightIndicesBuffer = nvDevice->createBuffer(desc);
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = MAX_LIGHTS * sizeof(float);
        desc.structStride = sizeof(float);
        desc.debugName = "ClusteredLights_DICDF";
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        desc.canHaveTypedViews = false;
        desc.canHaveUAVs = false;
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
    m_shadowDataBuffer = nullptr;
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
    m_shadowDataCPU.clear();
    m_lightSources.clear();
    m_localShadowTiles.clear();
    m_localShadowSticky.clear();
    m_spotTextureCache.clear();
    m_device = nullptr;
}

void ClusteredLightManager::BeginFrame()
{
    m_lightsCPU.clear();
    m_shadowDataCPU.clear();
    m_lightSources.clear();
    m_localShadowTiles.clear();
    m_diIndicesCPU.clear();
    m_diCDFCPU.clear();
    m_diLightCount = 0;
    m_diPowerSum = 0.f;
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

    light* mutableL = const_cast<light*>(L);
    CLight_Compute_XFORM_and_VIS xfVis;
    xfVis.compute_xf_spot(mutableL);
    return mutableL->X.S.combine;
}

GPULightData ClusteredLightManager::BuildGPULightData(const light* L)
{
    GPULightData gpu;

    const float range = L->range;
    const float invRangeSq = 1.0f / (range * range + 0.0001f);

    const float signedInvRangeSq = L->flags.bHudMode ? -invRangeSq : invRangeSq;
    gpu.positionAndInvRangeSq.set(L->position.x, L->position.y, L->position.z, signedInvRangeSq);
    gpu.colorAndRange.set(L->color.r, L->color.g, L->color.b, range);

    std::memset(&gpu.spotVP, 0, sizeof(gpu.spotVP));

    const u32 lightType = L->flags.type;
    const bool isSpot = (lightType == IRender_Light::SPOT || lightType == IRender_Light::OMNIPART);

    const u32 shadowWant = L->flags.bShadow ? 0u : 0xFFFFFFFFu;
    float shadowWantBits;
    std::memcpy(&shadowWantBits, &shadowWant, sizeof(float));

    if (isSpot)
    {
        const float cosOuter = _cos(L->cone);
        const float cosInner = _cos(L->cone * 0.8f);
        const float scale = 1.0f / std::max(cosInner - cosOuter, 0.001f);
        const float offset = -cosOuter * scale;

        u32 texIdx = 0xFFFFFFFFu;
        if (!L->spot_texture_name.empty())
            texIdx = GetOrLoadSpotTexture(L->spot_texture_name);

        gpu.directionAndSpotScale.set(L->direction.x, L->direction.y, L->direction.z, scale);

        float texIdxBits;
        std::memcpy(&texIdxBits, &texIdx, sizeof(float));
        gpu.spotParamsAndType.set(offset, 1.0f, texIdxBits, shadowWantBits);
        gpu.spotVP = BuildSpotClipVP(L);
    }
    else
    {
        gpu.directionAndSpotScale.set(0.0f, -1.0f, 0.0f, 0.0f);
        const u32 noCookie = 0xFFFFFFFFu;
        float noCookieBits;
        std::memcpy(&noCookieBits, &noCookie, sizeof(float));
        gpu.spotParamsAndType.set(0.0f, 0.0f, noCookieBits, shadowWantBits);
    }

    gpu.localShadowRect.set(0.f, 0.f, 0.f, 0.f);

    return gpu;
}

void ClusteredLightManager::RefreshHudSpotXForms()
{
    if (m_numLights == 0 || m_lightSources.size() != m_numLights)
        return;

    for (u32 i = 0; i < m_numLights; ++i)
    {
        light* L = const_cast<light*>(m_lightSources[i]);
        if (!L || !L->flags.bHudMode)
            continue;

        const u32 lt = L->flags.type;
        const bool isSpot = (lt == IRender_Light::SPOT || lt == IRender_Light::OMNIPART);
        if (!isSpot)
        {
            m_lightsCPU[i].positionAndInvRangeSq.set(
                L->position.x, L->position.y, L->position.z,
                m_lightsCPU[i].positionAndInvRangeSq.w);
            continue;
        }

        const float cosOuter = _cos(L->cone);
        const float cosInner = _cos(L->cone * 0.8f);
        const float scale = 1.0f / std::max(cosInner - cosOuter, 0.001f);
        const float offset = -cosOuter * scale;

        m_lightsCPU[i].positionAndInvRangeSq.set(
            L->position.x, L->position.y, L->position.z,
            m_lightsCPU[i].positionAndInvRangeSq.w);
        m_lightsCPU[i].directionAndSpotScale.set(
            L->direction.x, L->direction.y, L->direction.z, scale);
        m_lightsCPU[i].spotParamsAndType.x = offset;
        m_lightsCPU[i].spotVP = BuildSpotClipVP(L);
        if (m_lightsCPU[i].positionAndInvRangeSq.w > 0.f)
            m_lightsCPU[i].positionAndInvRangeSq.w = -m_lightsCPU[i].positionAndInvRangeSq.w;
    }
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

    m_expandedLights.clear();
    m_expandedLights.reserve(lights.size() + 32);
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
                    m_expandedLights.push_back(L->omnipart[f]);
            }
        }
        else
        {
            m_expandedLights.push_back(L);
        }
    }

    const u32 count = std::min(static_cast<u32>(m_expandedLights.size()), MAX_LIGHTS);
    if (count == 0)
        return;

    for (u32 i = 0; i < count; i++)
    {
        const light* L = m_expandedLights[i];
        const u32 lt = L->flags.type;
        const bool isSpot = (lt == IRender_Light::SPOT || lt == IRender_Light::OMNIPART);
        if (isSpot && !L->spot_texture_name.empty())
            GetOrLoadSpotTexture(L->spot_texture_name);
    }

    m_lightsCPU.resize(count);
    m_lightSources.resize(count);
    m_numLights = count;

    for (u32 i = 0; i < count; ++i)
        m_lightSources[i] = m_expandedLights[i];

    xr_parallel_for(TaskRange<u32>(0, count), [&](const TaskRange<u32>& range) {
        for (u32 i = range.begin(); i != range.end(); ++i)
            m_lightsCPU[i] = BuildGPULightData(m_expandedLights[i]);
    });

    if (psDeviceFlags.test(rsStatistic))
    {
        for (u32 i = 0; i < count; i++)
        {
            const u32 lt = m_expandedLights[i]->flags.type;
            if (lt == IRender_Light::POINT)
                m_numPoint++;
            else if (lt == IRender_Light::SPOT)
                m_numSpot++;
            else if (lt == IRender_Light::OMNIPART)
                m_numOmni++;
        }
    }
}

void ClusteredLightManager::ClearLocalShadowAssignments()
{
    m_localShadowTiles.clear();
    m_localShadowCandidates = 0;
    m_localShadowDropped = 0;
    m_localShadowRedraw = 0;
    m_shadowDataCPU.clear();
    for (u32 i = 0; i < m_numLights; ++i)
    {
        m_lightsCPU[i].localShadowRect.set(0.f, 0.f, 0.f, 0.f);
        m_lightsCPU[i].spotParamsAndType.w = 0.f;
    }
}

void ClusteredLightManager::AssignLocalShadowTiles(const Fvector& cameraPos)
{
    ZoneScopedN("LocalShadow::AssignTiles");
    m_localShadowTiles.clear();
    if (m_localShadowTiles.capacity() < 128)
        m_localShadowTiles.reserve(128);
    m_localShadowCandidates = 0;
    m_localShadowDropped = 0;
    m_localShadowRedraw = 0;
    if (ps_r_local_shadows == 0 || m_numLights == 0 || m_lightSources.size() != m_numLights)
    {
        m_localShadowSticky.clear();
        ClearLocalShadowAssignments();
        return;
    }

    struct Candidate
    {
        u32 idx = 0;
        u32 smapSize = 0;
        Fmatrix clipVP;
        const light* L = nullptr;
        bool wasSticky = false;
        bool cameraInside = false;
        float dist = 0.f;
    };

    CLight_Compute_XFORM_and_VIS xfVis;
    static thread_local xr_vector<Candidate> s_ranked;
    static thread_local xr_vector<Candidate> s_remaining;
    static thread_local xr_vector<Candidate> s_leftover;
    static thread_local MaxRectsAllocator s_pools[MAX_LOCAL_SHADOW_PAGES];
    static u32 s_lastAtlasSize = 0;
    s_ranked.clear();
    s_ranked.reserve(m_numLights);
    auto& candidateIndex = m_localShadowCandidateIndex;
    candidateIndex.clear();

    CFrustum camFrustum;
    camFrustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);
    const Fmatrix& fullXform = Device.mFullTransform;

    for (u32 i = 0; i < m_numLights; ++i)
    {
        light* L = const_cast<light*>(m_lightSources[i]);
        if (!L || !L->flags.bShadow)
            continue;
        const u32 lt = L->flags.type;
        if (lt != IRender_Light::SPOT && lt != IRender_Light::OMNIPART)
            continue;
        if (L->flags.bHudMode)
            continue;

        L->spatial_move();

        const float lod = L->get_LOD();
        m_lightsCPU[i].colorAndRange.set(
            L->color.r * lod, L->color.g * lod, L->color.b * lod, L->range);

        const Fvector& sp = L->spatial.sphere.P;
        const float sr = std::max(L->spatial.sphere.R, 0.01f);
        const float distCenter = cameraPos.distance_to(sp);
        const bool cameraInside = distCenter <= (sr * 1.01f + VIEWPORT_NEAR);
        const bool wasSticky = m_localShadowSticky.find(L) != m_localShadowSticky.end();
        const float dist = std::max(0.f, distCenter - sr);

        if (!cameraInside && lod <= EPS_L)
            continue;

        if (!cameraInside)
        {
            const float dropDist = wasSticky ? 120.f : 100.f;
            if (dist > dropDist)
                continue;
            const float frustumInflate = wasSticky ? 1.35f : 1.1f;
            if (!camFrustum.testSphere_dirty(sp, sr * frustumInflate))
                continue;
            if (!wasSticky &&
                !LightSphereOnScreen(sp, sr, fullXform, 0.15f))
                continue;
        }

        if (lt == IRender_Light::OMNIPART && !wasSticky && !cameraInside &&
            !OmniFaceUsefulForCamera(L, cameraPos, camFrustum))
            continue;

        xfVis.compute_xf_spot(L);
        m_lightsCPU[i].spotVP = L->X.S.combine;

        Candidate c;
        c.idx = i;
        c.smapSize = L->X.S.size;
        c.clipVP = m_lightsCPU[i].spotVP;
        c.L = L;
        c.wasSticky = wasSticky;
        c.cameraInside = cameraInside;
        c.dist = dist;
        candidateIndex[L] = static_cast<u32>(s_ranked.size());
        s_ranked.push_back(c);
    }

    if (s_ranked.empty())
    {
        m_localShadowSticky.clear();
        return;
    }

    const u32 atlasSize = LocalShadowAtlasSize();
    const u32 pageCount = LocalShadowPageCount();
    const bool atlasSizeChanged = (s_lastAtlasSize != 0 && s_lastAtlasSize != atlasSize);
    if (atlasSizeChanged)
        m_localShadowSticky.clear();
    s_lastAtlasSize = atlasSize;

    static int s_lastLocalShadowFilter = -1;
    const bool filterChanged = (s_lastLocalShadowFilter != ps_r_local_shadow_filter);
    if (filterChanged)
        s_lastLocalShadowFilter = ps_r_local_shadow_filter;

    const u32 maxTiles = std::clamp(
        static_cast<u32>(std::max(ps_r_local_shadow_tiles, 1)),
        1u,
        std::min(MAX_LOCAL_SHADOW_TILES, GPUCullingManager::kLocalShadowCullMaxSlots));

    u32 wantedMaxRect = std::min(atlasSize / 2u, 2048u);
    wantedMaxRect = std::max(wantedMaxRect, 256u);
    auto packCapacity = [&](u32 rect) -> u32 {
        const u32 cell = std::max(rect, 128u);
        const u32 perSide = atlasSize / cell;
        return pageCount * perSide * perSide;
    };
    while (wantedMaxRect > 128u && packCapacity(wantedMaxRect) < s_ranked.size())
        wantedMaxRect = std::max(128u, wantedMaxRect / 2u);

    static u32 s_stableMaxRect = 0;
    static u32 s_maxRectShrinkHold = 0;
    if (atlasSizeChanged || s_stableMaxRect == 0)
    {
        s_stableMaxRect = wantedMaxRect;
        s_maxRectShrinkHold = 0;
    }
    else if (wantedMaxRect < s_stableMaxRect)
    {
        ++s_maxRectShrinkHold;
        if (s_maxRectShrinkHold > 90)
        {
            s_stableMaxRect = wantedMaxRect;
            s_maxRectShrinkHold = 0;
        }
    }
    else
    {
        s_maxRectShrinkHold = 0;
        if (wantedMaxRect > s_stableMaxRect)
            s_stableMaxRect = wantedMaxRect;
    }
    const u32 maxRect = s_stableMaxRect;

    std::sort(s_ranked.begin(), s_ranked.end(), [&](const Candidate& a, const Candidate& b) {
        const bool ha = a.L->flags.bHudMode;
        const bool hb = b.L->flags.bHudMode;
        if (ha != hb)
            return ha;
        if (a.wasSticky != b.wasSticky)
            return a.wasSticky;
        if (a.cameraInside != b.cameraInside)
            return a.cameraInside;
        if (a.smapSize != b.smapSize)
            return a.smapSize > b.smapSize;
        if (a.dist != b.dist)
            return a.dist < b.dist;
        return a.L < b.L;
    });
    candidateIndex.clear();
    for (u32 i = 0; i < s_ranked.size(); ++i)
        candidateIndex[s_ranked[i].L] = i;

    const float invSmap = 1.f / float(atlasSize);
    for (u32 page = 0; page < pageCount; ++page)
        s_pools[page].initialize(atlasSize);

    auto& nextSticky = m_localShadowStickyScratch;
    nextSticky.clear();
    m_localShadowKept.assign(s_ranked.size(), 0);
    auto& kept = m_localShadowKept;
    u32 packed = 0;

    auto clampSmapSize = [&](u32 sz) -> u32 {
        if (sz > maxRect)
            sz = maxRect;
        if (sz > atlasSize)
            sz = atlasSize;
        if (sz < 128)
            sz = 128;
        u32 p = 128;
        while (p < sz && p < 2048u)
            p <<= 1;
        return std::min(p, atlasSize);
    };

    auto tryPush = [&](u32 page, u32& sz, MaxRectsRect& R) -> bool {
        u32 trySz = sz;
        while (trySz >= 128)
        {
            if (s_pools[page].push(R, trySz))
            {
                sz = trySz;
                return true;
            }
            trySz >>= 1;
        }
        return false;
    };

    const u32 updateDiv = std::clamp(static_cast<u32>(std::max(ps_r_local_shadow_update_div, 1)), 1u, 8u);
    const float nearDist = std::max(1.f, ps_r_local_shadow_near);
    const float midDist = std::max(nearDist + 1.f, ps_r_local_shadow_mid);
    const u32 farPeriod = std::clamp(static_cast<u32>(std::max(ps_r_local_shadow_far_period, 1)), 1u, 30u);

    auto lightHash = [](const light* L) -> u32 {
        return u32(size_t(L) >> 4) * 2654435761u;
    };

    auto softPeriodFor = [&](const light* L) -> u32 {
        const float d = cameraPos.distance_to(L->position);
        u32 period = farPeriod;
        if (d < nearDist)
            period = 1;
        else if (d < midDist)
            period = 2;
        return std::max(1u, period * updateDiv);
    };

    constexpr float kLocalShadowFadeOutStep = 1.f / 28.f;
    auto packShadowRectW = [](u32 page, float opacity) -> float {
        opacity = std::clamp(opacity, 0.f, 1.f);
        return float(page + 1u) + opacity * 0.999f;
    };
    auto easeFade = [](float t) -> float {
        t = std::clamp(t, 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    };

    auto emitTile = [&](const Candidate& c, u32 page, u32 posX, u32 posY, u32 sz) {
        const u32 border = (sz >= 64u) ? 2u : ((sz >= 32u) ? 1u : 0u);
        const u32 inner = sz - border * 2u;
        const u32 ix = posX + border;
        const u32 iy = posY + border;

        LocalShadowTile tile;
        tile.L = c.L;
        tile.lightIndex = c.idx;
        tile.posX = posX;
        tile.posY = posY;
        tile.size = sz;
        tile.page = page;

        bool slotChanged = true;
        bool lightMoved = true;
        bool lightTurned = true;
        auto prev = m_localShadowSticky.find(c.L);
        if (prev != m_localShadowSticky.end())
        {
            const LocalShadowSticky& s = prev->second;
            slotChanged = (s.page != page || s.posX != posX || s.posY != posY || s.size != sz);
            lightMoved = !s.lightPos.similar(c.L->position, 0.05f);
            lightTurned = s.lightDir.dotproduct(c.L->direction) < 0.9995f;
        }
        const bool hudLight = c.L->flags.bHudMode;
        const bool mandatory = slotChanged || lightMoved || lightTurned || hudLight || filterChanged;
        const u32 period = softPeriodFor(c.L);
        const bool softDue = ((Device.dwFrame + lightHash(c.L)) % period) == 0;
        tile.mandatoryRedraw = mandatory;
        tile.needsRedraw = mandatory || softDue;
        tile.needsStaticRedraw = mandatory;
        tile.needsDynamicRedraw = tile.needsRedraw;

        Fmatrix useVP = c.clipVP;
        if (!tile.needsRedraw && prev != m_localShadowSticky.end() && prev->second.hasClipVP)
            useVP = prev->second.lastClipVP;
        tile.clipVP = useVP;
        m_lightsCPU[c.idx].spotVP = useVP;

        const u32 shadowIdx = static_cast<u32>(m_localShadowTiles.size());
        float shadowIdxBits;
        std::memcpy(&shadowIdxBits, &shadowIdx, sizeof(float));
        m_lightsCPU[c.idx].spotParamsAndType.w = shadowIdxBits;

        LocalShadowSticky sticky;
        sticky.page = page;
        sticky.posX = posX;
        sticky.posY = posY;
        sticky.size = sz;
        sticky.missFrames = 0;
        sticky.shrinkHold = 0;
        if (prev != m_localShadowSticky.end() && prev->second.size == sz)
            sticky.shrinkHold = prev->second.shrinkHold;
        sticky.lightPos = c.L->position;
        sticky.lightDir = c.L->direction;
        sticky.lastClipVP = useVP;
        sticky.hasClipVP = true;
        sticky.fade = 1.f;
        m_lightsCPU[c.idx].localShadowRect.set(
            float(ix) * invSmap,
            float(iy) * invSmap,
            float(inner) * invSmap,
            packShadowRectW(page, easeFade(sticky.fade)));
        nextSticky[c.L] = sticky;
        m_localShadowTiles.push_back(tile);
        ++packed;
    };

    auto lightIndexOf = [&](const light* L) -> u32 {
        for (u32 i = 0; i < m_numLights; ++i)
        {
            if (m_lightSources[i] == L)
                return i;
        }
        return ~0u;
    };

    for (auto it = m_localShadowSticky.begin(); it != m_localShadowSticky.end(); ++it)
    {
        const light* L = it->first;
        LocalShadowSticky sticky = it->second;
        auto found = candidateIndex.find(L);
        if (found == candidateIndex.end() || packed >= maxTiles)
        {
            sticky.missFrames++;
            sticky.fade = std::max(0.f, sticky.fade - kLocalShadowFadeOutStep);
            if (sticky.fade <= 1e-3f && sticky.missFrames > 8)
                continue;

            const u32 li = lightIndexOf(L);
            if (li == ~0u || sticky.page >= pageCount ||
                !s_pools[sticky.page].reserve(sticky.posX, sticky.posY, sticky.size))
            {
                nextSticky[L] = sticky;
                continue;
            }

            const u32 border = (sticky.size >= 64u) ? 2u : ((sticky.size >= 32u) ? 1u : 0u);
            const u32 inner = sticky.size - border * 2u;
            m_lightsCPU[li].localShadowRect.set(
                float(sticky.posX + border) * invSmap,
                float(sticky.posY + border) * invSmap,
                float(inner) * invSmap,
                packShadowRectW(sticky.page, easeFade(sticky.fade)));
            if (sticky.hasClipVP)
                m_lightsCPU[li].spotVP = sticky.lastClipVP;

            LocalShadowTile tile;
            tile.L = L;
            tile.clipVP = sticky.hasClipVP ? sticky.lastClipVP : m_lightsCPU[li].spotVP;
            tile.lightIndex = li;
            tile.posX = sticky.posX;
            tile.posY = sticky.posY;
            tile.size = sticky.size;
            tile.page = sticky.page;
            tile.needsRedraw = false;
            tile.mandatoryRedraw = false;
            tile.needsStaticRedraw = false;
            tile.needsDynamicRedraw = false;
            const u32 shadowIdx = static_cast<u32>(m_localShadowTiles.size());
            float shadowIdxBits;
            std::memcpy(&shadowIdxBits, &shadowIdx, sizeof(float));
            m_lightsCPU[li].spotParamsAndType.w = shadowIdxBits;
            nextSticky[L] = sticky;
            m_localShadowTiles.push_back(tile);
            ++packed;
            continue;
        }

        const Candidate& c = s_ranked[found->second];
        const u32 desired = clampSmapSize(c.smapSize);
        const bool wantGrow = desired > sticky.size + (sticky.size >> 1) &&
            ((Device.dwFrame % 120u) == 0);
        if (desired + 16u < sticky.size)
            sticky.shrinkHold++;
        else
            sticky.shrinkHold = 0;
        it->second.shrinkHold = sticky.shrinkHold;
        const bool wantShrink = sticky.shrinkHold > 180 && desired < (sticky.size >> 1);

        if ((wantGrow || wantShrink) && packed < maxTiles)
        {
            bool upgraded = false;
            for (u32 page = 0; page < pageCount && !upgraded; ++page)
            {
                u32 sz = desired;
                MaxRectsRect R{};
                if (!tryPush(page, sz, R))
                    continue;
                kept[found->second] = 1;
                emitTile(c, page, R.x, R.y, sz);
                upgraded = true;
            }
            if (upgraded)
                continue;
        }

        if (sticky.page >= pageCount ||
            !s_pools[sticky.page].reserve(sticky.posX, sticky.posY, sticky.size))
            continue;

        kept[found->second] = 1;
        emitTile(c, sticky.page, sticky.posX, sticky.posY, sticky.size);
    }

    s_remaining.clear();
    s_remaining.reserve(s_ranked.size());
    for (u32 i = 0; i < s_ranked.size(); ++i)
    {
        if (!kept[i])
            s_remaining.push_back(s_ranked[i]);
    }

    for (u32 page = 0; page < pageCount && !s_remaining.empty() && packed < maxTiles; ++page)
    {
        s_leftover.clear();
        s_leftover.reserve(s_remaining.size());

        for (const Candidate& c : s_remaining)
        {
            if (packed >= maxTiles)
            {
                s_leftover.push_back(c);
                continue;
            }

            u32 sz = clampSmapSize(c.smapSize);
            MaxRectsRect R{};
            if (!tryPush(page, sz, R))
            {
                s_leftover.push_back(c);
                continue;
            }

            emitTile(c, page, R.x, R.y, sz);
        }

        s_remaining.swap(s_leftover);
    }

    m_localShadowSticky.swap(nextSticky);
    m_localShadowCandidates = static_cast<u32>(s_ranked.size());
    m_localShadowDropped = static_cast<u32>(s_remaining.size());

    const u32 tileCount = static_cast<u32>(m_localShadowTiles.size());
    if (tileCount > 1)
    {
        std::stable_partition(m_localShadowTiles.begin(), m_localShadowTiles.end(),
            [](const LocalShadowTile& t) { return t.mandatoryRedraw; });
    }

    if (tileCount > 0)
    {
        const u32 hard = std::clamp(static_cast<u32>(std::max(ps_r_local_shadow_redraw_budget, 1)), 1u, 128u);
        u32 softKept = 0;
        for (LocalShadowTile& t : m_localShadowTiles)
        {
            if (t.mandatoryRedraw)
            {
                t.needsRedraw = true;
                t.needsStaticRedraw = true;
                t.needsDynamicRedraw = true;
                continue;
            }
            if (!t.needsRedraw)
                continue;
            if (softKept < hard)
            {
                ++softKept;
                t.needsDynamicRedraw = true;
                continue;
            }
            t.needsRedraw = false;
            t.needsDynamicRedraw = false;
            t.needsStaticRedraw = false;
            if (t.L)
            {
                auto it = m_localShadowSticky.find(t.L);
                if (it != m_localShadowSticky.end() && it->second.hasClipVP)
                {
                    t.clipVP = it->second.lastClipVP;
                    if (t.lightIndex < m_numLights)
                        m_lightsCPU[t.lightIndex].spotVP = it->second.lastClipVP;
                }
            }
        }
    }

    m_shadowDataCPU.clear();
    m_shadowDataCPU.reserve(tileCount);
    for (u32 ti = 0; ti < tileCount; ++ti)
    {
        const LocalShadowTile& t = m_localShadowTiles[ti];
        const u32 border = (t.size >= 64u) ? 2u : ((t.size >= 32u) ? 1u : 0u);
        const u32 inner = t.size - border * 2u;
        GPUShadowData sd{};
        float opacity = 1.f;
        auto sit = m_localShadowSticky.find(t.L);
        if (sit != m_localShadowSticky.end())
            opacity = easeFade(sit->second.fade);
        sd.atlasOffsetScale.set(
            float(t.posX + border) * invSmap,
            float(t.posY + border) * invSmap,
            float(inner) * invSmap,
            packShadowRectW(t.page, opacity));
        sd.viewProj = t.clipVP;
        m_shadowDataCPU.push_back(sd);

        float shadowIdxBits;
        std::memcpy(&shadowIdxBits, &ti, sizeof(float));
        if (t.lightIndex < m_numLights)
            m_lightsCPU[t.lightIndex].spotParamsAndType.w = shadowIdxBits;
    }

    m_localShadowRedraw = 0;
    for (const LocalShadowTile& t : m_localShadowTiles)
    {
        if (t.needsRedraw)
            ++m_localShadowRedraw;
    }

    {
        static bool s_logged = false;
        if (!s_logged)
        {
            Msg("* [LocalShadow] MaxRects sticky: %u fitted, %u dropped, atlas %u×%u pages (candidates %u, maxRect %u, maxTiles %u, redraw %u div %u budget %d)",
                packed, (u32)s_remaining.size(), atlasSize, pageCount, (u32)s_ranked.size(), maxRect, maxTiles,
                m_localShadowRedraw, updateDiv, ps_r_local_shadow_redraw_budget);
            s_logged = true;
        }
    }
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

void ClusteredLightManager::BuildDISampleTable()
{
    m_diIndicesCPU.clear();
    m_diCDFCPU.clear();
    m_diLightCount = 0;
    m_diPowerSum = 0.f;
    if (m_numLights == 0 || m_lightSources.size() != m_numLights)
        return;

    CFrustum camFrustum;
    camFrustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

    float sum = 0.f;
    m_diIndicesCPU.reserve(m_numLights);
    m_diCDFCPU.reserve(m_numLights);

    for (u32 i = 0; i < m_numLights; ++i)
    {
        const light* L = m_lightSources[i];
        if (!L)
            continue;

        const bool hudLight = L->flags.bHudMode;
        const Fvector& sp = L->spatial.sphere.P;
        const float sr = std::max(L->spatial.sphere.R, 0.01f);
        if (!hudLight && !camFrustum.testSphere_dirty(sp, sr * 1.05f))
            continue;

        const float lod = std::max(L->get_LOD(), 0.f);
        if (!hudLight && lod <= EPS_L)
            continue;

        const float fluxProxy = std::max(
            L->color.r * lod * 0.2126f +
            L->color.g * lod * 0.7152f +
            L->color.b * lod * 0.0722f,
            1e-4f);
        const float range = std::max(L->range, 0.5f);
        const float distToCam = std::max(Device.vCameraPosition.distance_to(sp), 0.01f);
        const float geomProxy = (range * range) / std::max(distToCam * distToCam, range * range * 0.25f);
        float power = fluxProxy * geomProxy;

        const u32 lt = L->flags.type;
        if (lt == IRender_Light::SPOT || lt == IRender_Light::OMNIPART)
        {
            const float cosOuter = _cos(L->cone);
            const float solid = std::max(1.f - cosOuter, 0.05f);
            power *= solid;
        }
        if (hudLight)
            power *= 2.f;

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

    if (m_shadowDataBuffer && !m_shadowDataCPU.empty())
    {
        cmdList->writeBuffer(m_shadowDataBuffer, m_shadowDataCPU.data(),
            m_shadowDataCPU.size() * sizeof(GPUShadowData));
    }

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

    cmdList->writeBuffer(m_visibleLightIndicesBuffer, m_identityIndices.data(), m_numLights * sizeof(u32));
    cmdList->writeBuffer(m_visibleLightCountBuffer, &m_numLights, sizeof(u32));
    m_visibleLightCountCPU = m_numLights;
}

ClusterCB ClusteredLightManager::BuildClusterCB(u32 screenWidth, u32 screenHeight, float zNear, float zFar) const
{
    const u32 tileSize = ClusterTileSize();
    const u32 tilesX = (screenWidth + tileSize - 1) / tileSize;
    const u32 tilesY = (screenHeight + tileSize - 1) / tileSize;
    const float logRatio = static_cast<float>(CLUSTER_NUM_SLICES) / log2f(zFar / zNear);

    ClusterCB cb;
    cb.gridDims.set(static_cast<float>(tilesX), static_cast<float>(tilesY),
        static_cast<float>(CLUSTER_NUM_SLICES), static_cast<float>(m_numLights));
    cb.screenSize.set(static_cast<float>(screenWidth), static_cast<float>(screenHeight),
        1.0f / static_cast<float>(screenWidth), 1.0f / static_cast<float>(screenHeight));
    cb.depthParams.set(zNear, zFar, logRatio, static_cast<float>(tileSize));
    // cot(fovY/2) for cluster XY radius — matches m_P._22 from build_projection_HAT
    cb.pad.set(Device.mProject._22, 0.f, 0.f, 0.f);

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

    constexpr u32 kInvalidCookie = 0xFFFFFFFFu;
    auto* renderDevice = GEnv.Render ? GEnv.Render->GetRenderDevice() : nullptr;
    if (!renderDevice)
        return kInvalidCookie;

    auto* resMgr = renderDevice->GetFGResourceManager();
    auto* backend = renderDevice->GetBackend();
    if (!resMgr || !backend)
        return kInvalidCookie;

    auto* texManager = resMgr->GetTextureManager();
    if (!texManager)
        return kInvalidCookie;

    auto handle = texManager->LoadTexture(name.c_str());
    if (!handle.IsValid())
    {
        m_spotTextureCache[name] = kInvalidCookie;
        return kInvalidCookie;
    }

    nvrhi::ITexture* nvrhiTex = texManager->GetNVRHITexture(handle);
    if (!nvrhiTex)
    {
        m_spotTextureCache[name] = kInvalidCookie;
        return kInvalidCookie;
    }

    u32 bindlessIdx = backend->RegisterBindlessTexture(nvrhiTex);
    m_spotTextureCache[name] = bindlessIdx;
    return bindlessIdx;
}

}
