#pragma once

#include "xrCore/xrCore.h"
#include "MeshPicker.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
namespace ng {
    class RenderDevice;
}
}

namespace xray::render::RENDER_NAMESPACE {
class CKinematics;
}

namespace xray::render::RENDER_NAMESPACE::decals {

static constexpr u32 MAX_SPLATS_PER_OBJECT = 32;

struct alignas(16) GPUPaintSplat {
    Fvector4 posRadius;     // xyz = rest-pose position (bind pose), w = world-space radius
    Fvector4 color;         // rgb + alpha
    u32 boneIdx[4];         // up to 4 bone indices (local to skeleton)
    float boneWeight[4];    // corresponding weights (sum = 1)
};
static_assert(sizeof(GPUPaintSplat) == 64);

struct SplatDebugInfo {
    Fvector2 uv;
    xr_string textureName;
};

struct ObjectSplats {
    xr_vector<GPUPaintSplat> splats;
    xr_vector<SplatDebugInfo> splatDebug;
    float lastPaintTime = 0.f;
};

class OverlayManager {
public:
    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    void AddSplat(CKinematics* obj, const TriVertexSkin triVerts[3],
                  float baryU, float baryV, float radius,
                  const Fvector& color, float alpha,
                  Fvector2 uv = {0.f, 0.f}, const char* textureName = nullptr);

    struct SplatRange { u32 offset; u32 count; };
    SplatRange GetSplatRange(CKinematics* obj) const;

    void UploadSplats(nvrhi::ICommandList* cmdList);
    nvrhi::IBuffer* GetSplatBuffer() const { return m_splatBuffer.Get(); }

    void CleanupExpired(float currentTime, float maxAge);
    u32 GetOverlayCount() const { return (u32)m_objects.size(); }
    const xr_map<CKinematics*, ObjectSplats>& GetDebugObjects() const { return m_objects; }

private:
    ng::RenderDevice* m_device = nullptr;
    xr_map<CKinematics*, ObjectSplats> m_objects;
    nvrhi::BufferHandle m_splatBuffer;
    xr_vector<GPUPaintSplat> m_gpuSplats;
    xr_map<CKinematics*, SplatRange> m_rangeCache;
    bool m_dirty = false;
};

} // namespace xray::render::RENDER_NAMESPACE::decals
