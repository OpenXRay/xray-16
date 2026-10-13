#pragma once

#include <memory>
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph
{
class BindingSetBuilder;
}

namespace xray::render::fg
{
class GpuParticleCollision
{
public:
    GpuParticleCollision();
    ~GpuParticleCollision();
    GpuParticleCollision(const GpuParticleCollision&) = delete;
    GpuParticleCollision& operator=(const GpuParticleCollision&) = delete;
    void Update(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, u32 definitionFlags);
    void Warm(nvrhi::IDevice* device);
    void Bind(framegraph::BindingSetBuilder& builder) const;
    void Reset();
    u64 GetBindingVersion() const;
    struct Counts { u32 bvhNodes,staticTriangles,dynamicObjects,dynamicShapes,dynamicTriangles; };
    Counts GetCounts() const;

private:
    struct State;
    std::unique_ptr<State> state;
};
}
