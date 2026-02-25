#include "stdafx.h"
#include "OverlayManager.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "xrEngine/xr_object.h"

namespace xray::render::RENDER_NAMESPACE::decals {

void OverlayManager::Initialize(ng::RenderDevice* device)
{
    m_device = device;
    m_pendingPaints.reserve(32);
}

void OverlayManager::Shutdown()
{
    for (auto& [obj, overlay] : m_overlays) {
        if (overlay.bindlessIndex != UINT32_MAX)
            GEnv.Backend->UnregisterBindlessTexture(overlay.bindlessIndex);
    }
    m_overlays.clear();
    m_pendingPaints.clear();
}

u32 OverlayManager::ResolutionForDistance(float distSq)
{
    if (distSq < 15.f * 15.f)
        return 512;
    if (distSq < 40.f * 40.f)
        return 256;
    return 128;
}

ObjectOverlay& OverlayManager::GetOrCreateOverlay(CKinematics* obj, float distSq)
{
    auto it = m_overlays.find(obj);
    if (it != m_overlays.end())
        return it->second;

    u32 res = ResolutionForDistance(distSq);
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    nvrhi::TextureDesc desc;
    desc.width = res;
    desc.height = res;
    desc.format = nvrhi::Format::RGBA16_FLOAT;
    desc.isRenderTarget = true;
    desc.isUAV = true;
    desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    desc.keepInitialState = true;
    desc.debugName = "NPC_Overlay";

    ObjectOverlay overlay;
    overlay.colorOverlay = nvDevice->createTexture(desc);
    overlay.bindlessIndex = GEnv.Backend->RegisterBindlessTexture(overlay.colorOverlay.Get());
    overlay.resolution = res;
    overlay.dirty = true;
    overlay.lastPaintTime = Device.fTimeGlobal;

    return m_overlays.emplace(obj, std::move(overlay)).first->second;
}

void OverlayManager::ReleaseOverlay(CKinematics* obj)
{
    auto it = m_overlays.find(obj);
    if (it == m_overlays.end())
        return;

    if (it->second.bindlessIndex != UINT32_MAX)
        GEnv.Backend->UnregisterBindlessTexture(it->second.bindlessIndex);
    m_overlays.erase(it);
}

u32 OverlayManager::GetBindlessIndex(CKinematics* obj) const
{
    auto it = m_overlays.find(obj);
    if (it == m_overlays.end())
        return UINT32_MAX;
    return it->second.bindlessIndex;
}

void OverlayManager::QueuePaint(CKinematics* target, const Fvector2& hitUV,
                                 float uvRadius, u32 decalTextureIndex,
                                 const Fvector4& tintColor, u32 flags)
{
    PaintCommand cmd;
    cmd.target = target;
    cmd.hitUV = hitUV;
    cmd.uvRadius = uvRadius;
    cmd.decalTextureIndex = decalTextureIndex;
    cmd.tintColor = tintColor;
    cmd.flags = flags;
    m_pendingPaints.push_back(cmd);
}

void OverlayManager::CleanupExpired(float currentTime, float maxAge)
{
    for (auto it = m_overlays.begin(); it != m_overlays.end(); ) {
        if (currentTime - it->second.lastPaintTime > maxAge) {
            if (it->second.bindlessIndex != UINT32_MAX)
                GEnv.Backend->UnregisterBindlessTexture(it->second.bindlessIndex);
            it = m_overlays.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace xray::render::RENDER_NAMESPACE::decals
