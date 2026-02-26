#include "stdafx.h"
#include "OverlayManager.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::RENDER_NAMESPACE::decals {

static constexpr u32 MAX_GPU_SPLATS = 1024;

void OverlayManager::Initialize(ng::RenderDevice* device)
{
    m_device = device;
    m_gpuSplats.reserve(MAX_GPU_SPLATS);

    nvrhi::BufferDesc desc;
    desc.byteSize = MAX_GPU_SPLATS * sizeof(GPUPaintSplat);
    desc.structStride = sizeof(GPUPaintSplat);
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    desc.debugName = "PaintSplatBuffer";
    m_splatBuffer = device->GetNVRHIDevice()->createBuffer(desc);
}

void OverlayManager::Shutdown()
{
    m_objects.clear();
    m_rangeCache.clear();
    m_gpuSplats.clear();
    m_splatBuffer = nullptr;
}

void OverlayManager::AddSplat(CKinematics* obj, const TriVertexSkin triVerts[3],
                               float baryU, float baryV, float radius,
                               const Fvector& color, float alpha,
                               Fvector2 uv, float uvRadius,
                               u32 wallmarkMaterialID,
                               const char* targetTextureName,
                               const char* wallmarkTextureName)
{
    // Merge up to 12 bone contributions (3 verts × 4) into top-4 for the hit point
    struct BoneContrib { u16 idx; float weight; };
    BoneContrib contribs[12];
    u32 numContribs = 0;

    float bary[3] = { 1.f - baryU - baryV, baryU, baryV };

    Fvector restPos = { 0, 0, 0 };
    for (u32 i = 0; i < 3; i++)
        restPos.mad(triVerts[i].restPos, bary[i]);

    for (u32 i = 0; i < 3; i++) {
        for (u32 j = 0; j < 4; j++) {
            float w = bary[i] * triVerts[i].weight[j];
            if (w <= 0.f)
                continue;
            bool merged = false;
            for (u32 k = 0; k < numContribs; k++) {
                if (contribs[k].idx == triVerts[i].boneIdx[j]) {
                    contribs[k].weight += w;
                    merged = true;
                    break;
                }
            }
            if (!merged)
                contribs[numContribs++] = { triVerts[i].boneIdx[j], w };
        }
    }

    // Sort descending by weight, take top 4
    for (u32 i = 0; i < numContribs - 1; i++)
        for (u32 j = i + 1; j < numContribs; j++)
            if (contribs[j].weight > contribs[i].weight)
                std::swap(contribs[i], contribs[j]);

    u32 useCount = std::min(numContribs, 4u);
    float totalW = 0.f;
    for (u32 i = 0; i < useCount; i++)
        totalW += contribs[i].weight;
    if (totalW <= EPS_S)
        return;

    GPUPaintSplat gpu = {};
    gpu.posRadius = { restPos.x, restPos.y, restPos.z, radius };
    gpu.color = { color.x, color.y, color.z, alpha };
    for (u32 i = 0; i < useCount; i++) {
        gpu.boneIdx[i] = contribs[i].idx;
        gpu.boneWeight[i] = contribs[i].weight / totalW;
    }
    gpu.hitUV = uv;
    gpu.uvRadius = _max(uvRadius, 0.f);
    gpu.wallmarkMaterialID = wallmarkMaterialID;

    auto& data = m_objects[obj];
    data.lastPaintTime = Device.fTimeGlobal;
    if (data.splats.size() >= MAX_SPLATS_PER_OBJECT) {
        data.splats.erase(data.splats.begin());
        data.splatDebug.erase(data.splatDebug.begin());
    }
    data.splats.push_back(gpu);
    data.splatDebug.push_back({
        uv,
        targetTextureName ? xr_string(targetTextureName) : xr_string(),
        wallmarkTextureName ? xr_string(wallmarkTextureName) : xr_string()
    });
    m_dirty = true;
}

OverlayManager::SplatRange OverlayManager::GetSplatRange(CKinematics* obj) const
{
    auto it = m_rangeCache.find(obj);
    if (it != m_rangeCache.end())
        return it->second;
    return { 0, 0 };
}

void OverlayManager::UploadSplats(nvrhi::ICommandList* cmdList)
{
    if (!m_dirty || !m_splatBuffer || !cmdList)
        return;

    m_gpuSplats.clear();
    m_rangeCache.clear();

    for (auto& [obj, data] : m_objects) {
        SplatRange range;
        range.offset = (u32)m_gpuSplats.size();
        range.count = 0;

        const u32 total = (u32)data.splats.size();
        const u32 remaining = (u32)(MAX_GPU_SPLATS - m_gpuSplats.size());
        if (remaining == 0) {
            m_rangeCache[obj] = range;
            continue;
        }

        const u32 keepCount = std::min(total, remaining);
        const u32 start = total - keepCount;
        for (u32 i = start; i < total; i++)
        {
            const auto& s = data.splats[i];
            m_gpuSplats.push_back(s);
        }
        range.count = keepCount;
        m_rangeCache[obj] = range;
    }

    if (!m_gpuSplats.empty())
        cmdList->writeBuffer(m_splatBuffer, m_gpuSplats.data(),
                             (u32)(m_gpuSplats.size() * sizeof(GPUPaintSplat)));

    m_dirty = false;
}

void OverlayManager::CleanupExpired(float currentTime, float maxAge)
{
    for (auto it = m_objects.begin(); it != m_objects.end(); ) {
        if (currentTime - it->second.lastPaintTime > maxAge) {
            m_rangeCache.erase(it->first);
            it = m_objects.erase(it);
            m_dirty = true;
        } else {
            ++it;
        }
    }
}

} // namespace xray::render::RENDER_NAMESPACE::decals
