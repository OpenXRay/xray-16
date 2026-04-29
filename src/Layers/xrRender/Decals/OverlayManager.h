#pragma once

#include "xrCore/xrCore.h"
#include "MeshPicker.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
namespace fg {
    class RenderDevice;
}
}

namespace xray::render::fg {
class CKinematics;
}

namespace xray::render::fg::decals {

static constexpr u32 MAX_SPLATS_PER_OBJECT = 32;

enum SplatMode : u32 {
    SPLAT_MODE_DECAL = 0,
    SPLAT_MODE_PROCEDURAL_BLOOD = 1
};

struct alignas(16) GPUPaintSplat {
    Fvector4 posRadius;
    Fvector4 color;
    u32 boneIdx[4];
    float boneWeight[4];
    Fvector2 hitUV;
    float uvRadius;
    u32 wallmarkMaterialID;
    Fvector4 evolution;
};
static_assert(sizeof(GPUPaintSplat) == 96);

struct SplatDebugInfo {
    Fvector2 uv;
    xr_string targetTextureName;
    xr_string wallmarkTextureName;
};

struct ObjectSplats {
    xr_vector<GPUPaintSplat> splats;
    xr_vector<SplatDebugInfo> splatDebug;
    float lastPaintTime = 0.f;
};

class OverlayManager {
public:
    void Initialize(fg::RenderDevice* device);
    void Shutdown();

    void AddSplat(CKinematics* obj, const TriVertexSkin triVerts[3],
                  float baryU, float baryV, float radius,
                  const Fvector& color, float alpha,
                  Fvector2 uv = {0.f, 0.f}, float uvRadius = 0.f,
                  u32 wallmarkMaterialID = UINT32_MAX,
                  u32 mode = SPLAT_MODE_DECAL,
                  float lifetime = 12.f,
                  const char* targetTextureName = nullptr,
                  const char* wallmarkTextureName = nullptr);

    struct SplatRange { u32 offset; u32 count; };
    SplatRange GetSplatRange(CKinematics* obj) const;

    void UploadSplats(nvrhi::ICommandList* cmdList);
    nvrhi::IBuffer* GetSplatBuffer() const { return m_splatBuffer.Get(); }

    void CleanupExpired(float currentTime, float maxAge);
    u32 GetOverlayCount() const { return (u32)m_objects.size(); }
    const xr_map<CKinematics*, ObjectSplats>& GetDebugObjects() const { return m_objects; }

private:
    fg::RenderDevice* m_device = nullptr;
    xr_map<CKinematics*, ObjectSplats> m_objects;
    nvrhi::BufferHandle m_splatBuffer;
    xr_vector<GPUPaintSplat> m_gpuSplats;
    xr_map<CKinematics*, SplatRange> m_rangeCache;
    bool m_dirty = false;
};

}
