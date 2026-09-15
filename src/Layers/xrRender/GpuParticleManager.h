#pragma once
#include "xrCore/xrCore.h"
#include <nvrhi/nvrhi.h>
#include "../../../res/gamedata/shaders/r5/gpu_particle_types.h"
#include "FrameGraph/FGTypes.h"
#include <memory>
namespace xray::render::framegraph { class FrameGraph; }
namespace xray::render::fg {
namespace PS { class CPEDef; }
using GpuParticleHandle = u64;
struct GpuParticleSnapshot {
    u32 count = 0;
    bool playing = false;
    Fbox bounds;
    u32 serial = 0;
};
struct GpuParticleDrawResources {
    nvrhi::BufferHandle particles,emitters,programs,indices,drawArgs,bucketOffsets;
    u32 particleCapacity = 0;
    framegraph::VirtualResourceHandle particleResource,emitterResource,programResource,indexResource,argsResource,bucketResource;
};
class GpuParticleManager {
public:
    GpuParticleManager();
    ~GpuParticleManager();
    GpuParticleHandle CreateEmitter(const PS::CPEDef&);
    void DestroyEmitter(GpuParticleHandle);
    void Play(GpuParticleHandle);
    void Stop(GpuParticleHandle,bool deferred);
    void UpdateParent(GpuParticleHandle,const Fmatrix&,const Fvector&,bool localSpace);
    void SetHudMode(GpuParticleHandle,bool);
    void Tick(GpuParticleHandle,u32 milliseconds);
    bool GetSnapshot(GpuParticleHandle,GpuParticleSnapshot&) const;
    void ConfigureChildren(GpuParticleHandle,const char* birth,const char* play,const char* death,u32 groupFlags);
    const xr_vector<const PS::CPEDef*>& GetDefinitions() const;
    void SetProgramMaterial(u32 program,u32 material,u32 blend,u32 variant);
    void SetupSimulationPasses(framegraph::FrameGraph&,nvrhi::IDevice*);
    GpuParticleDrawResources GetDrawResources() const;
    void LevelUnload();
    void Reset();
private:
    struct Impl;
    std::shared_ptr<Impl> m_impl;
};
GpuParticleManager& GetGpuParticleManager();
}
