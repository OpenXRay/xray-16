#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_vector2.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
namespace ng {
    class RenderDevice;
    class RenderContext;
}
}

namespace xray::render::RENDER_NAMESPACE {
class CKinematics;
}

namespace xray::render::RENDER_NAMESPACE::decals {

struct PaintCommand {
    CKinematics* target;
    Fvector2 hitUV;
    float uvRadius;
    u32 decalTextureIndex;
    Fvector4 tintColor;
    u32 flags;
};

struct ObjectOverlay {
    nvrhi::TextureHandle colorOverlay;
    u32 bindlessIndex = UINT32_MAX;
    u32 resolution = 256;
    float lastPaintTime = 0.f;
    bool dirty = true;
};

class OverlayManager {
public:
    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    ObjectOverlay& GetOrCreateOverlay(CKinematics* obj, float distSq = 0.f);
    void ReleaseOverlay(CKinematics* obj);
    u32 GetBindlessIndex(CKinematics* obj) const;

    void QueuePaint(CKinematics* target, const Fvector2& hitUV,
                    float uvRadius, u32 decalTextureIndex,
                    const Fvector4& tintColor, u32 flags = 1);

    const xr_vector<PaintCommand>& GetPendingPaints() const { return m_pendingPaints; }
    void ClearPendingPaints() { m_pendingPaints.clear(); }

    void CleanupExpired(float currentTime, float maxAge);
    u32 GetOverlayCount() const { return (u32)m_overlays.size(); }

private:
    static u32 ResolutionForDistance(float distSq);

    ng::RenderDevice* m_device = nullptr;
    xr_map<CKinematics*, ObjectOverlay> m_overlays;
    xr_vector<PaintCommand> m_pendingPaints;
};

} // namespace xray::render::RENDER_NAMESPACE::decals
