#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_matrix.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
namespace fg {
    class RenderDevice;
    class RenderContext;
}
}

namespace xray::render::fg {
class CKinematics;
}

namespace xray::render::fg::decals {

constexpr u32 MAX_DECALS = 4096;
constexpr u32 CUBE_INDEX_COUNT = 36;
constexpr u32 CUBE_VERTEX_COUNT = 8;
constexpr float DEFAULT_NORMAL_THRESHOLD = 0.5f;
constexpr u32 DECAL_FLAG_STATIC = 0;
constexpr u32 DECAL_FLAG_SKELETON = 1;

struct alignas(16) GPUDecalData {
    Fmatrix worldToDecal;
    Fmatrix decalToWorld;
    u32 materialID;
    float opacity;
    float normalThreshold;
    u32 flags;
};
static_assert(sizeof(GPUDecalData) == 144, "GPUDecalData must be 144 bytes");

struct DecalInstance {
    Fmatrix decalToWorld;
    Fmatrix worldToDecal;
    Fvector position;
    float radius;
    u32 materialID;
    float creationTime;
    float ttl;
    Fvector normal;
};

struct SkeletonDecalInstance {
    CKinematics* parent;
    const Fmatrix* parentXForm;
    u16 boneID;
    Fmatrix localOBB;
    float size;
    u32 materialID;
    float creationTime;
    float ttl;
};

class DecalManager {
public:
    void Initialize(fg::RenderDevice* device);
    void Shutdown();

    void AddStaticDecal(const Fvector& pos, const Fvector& normal, float size, u32 materialID);
    void AddSkeletonDecal(CKinematics* parent, const Fmatrix* parentXForm,
                          u16 boneID, const Fvector& localPos,
                          const Fvector& localNormal, float size, u32 materialID);
    void AddSkeletonDecalFromRay(const Fmatrix* xf, CKinematics* obj,
                                  const Fvector& start, const Fvector& dir,
                                  float size, u32 materialID);

    void Update(float dt, float currentTime);
    void Upload(fg::RenderContext* ctx);

    nvrhi::IBuffer* GetDecalBuffer() const { return m_decalBuffer.Get(); }
    nvrhi::IBuffer* GetCubeVB() const { return m_cubeVB.Get(); }
    nvrhi::IBuffer* GetCubeIB() const { return m_cubeIB.Get(); }
    u32 GetActiveCount() const { return m_gpuDecalCount; }
    u32 GetStaticCount() const { return (u32)m_staticDecals.size(); }
    u32 GetSkeletonCount() const { return (u32)m_skeletonDecals.size(); }

private:
    void CreateCubeMesh(nvrhi::IDevice* device);
    void BuildOBB(DecalInstance& decal, const Fvector& pos, const Fvector& normal, float size, float angle);
    static Fmatrix BuildOBBMatrix(const Fvector& pos, const Fvector& normal, float size, float angle);
    void UpdateSkeletonDecals(float currentTime);

    fg::RenderDevice* m_device = nullptr;

    xr_vector<DecalInstance> m_staticDecals;
    xr_vector<SkeletonDecalInstance> m_skeletonDecals;

    xr_vector<GPUDecalData> m_gpuData;
    u32 m_gpuDecalCount = 0;

    nvrhi::BufferHandle m_decalBuffer;
    nvrhi::BufferHandle m_cubeVB;
    nvrhi::BufferHandle m_cubeIB;

    Lock m_lock;
};

} // namespace xray::render::fg::decals
