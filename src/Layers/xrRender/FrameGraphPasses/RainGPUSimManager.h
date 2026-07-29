#pragma once

#include "xrCore/xrCore.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg { class RenderDevice; }

namespace xray::render::fg::passes {

// Must match rain/rain_update.cs RainSimParams cbuffer layout exactly.
struct alignas(16) RainSimParams
{
    Fvector4 cameraPos;
    Fvector4 windAxis;
    Fvector4 params; // dt, factorVisual, unused, rainDensity
    Fmatrix rainSampleVP;
    Fvector4 rainSM;
    u32 particleCount = 0;
    u32 frameSeed = 0;
    u32 pad[2]{};
};
static_assert(sizeof(RainSimParams) == 144, "RainSimParams must match rain_update.cs (144 bytes)");

struct alignas(16) RainBillboardParams
{
    Fvector4 cameraPos;
    Fvector4 params; // factorVisual, dropLength, dropWidth, rainColorU32 as float bits
};
static_assert(sizeof(RainBillboardParams) == 32, "RainBillboardParams must be 32 bytes");

struct RainHeightmapInfo
{
    nvrhi::ITexture* texture = nullptr;
    float worldMinX = 0.f;
    float worldMinZ = 0.f;
    float texelSize = 0.f;
    bool valid = false;
};

class RainGPUSimManager
{
public:
    static constexpr u32 kThreadGroupSize = 64;
    static constexpr u32 kMaxParticles = 2500;

    RainGPUSimManager() = default;
    ~RainGPUSimManager();

    bool Initialize(fg::RenderDevice* device);
    void Shutdown();
    bool IsReady() const { return m_initialized; }

    void Dispatch(
        nvrhi::ICommandList* cmdList,
        nvrhi::ITexture* rainShadow,
        const Fmatrix& rainSampleVP,
        bool rainSMValid,
        u32 particleCount,
        float rainDensity,
        float factorVisual,
        u32 rainColor);

    nvrhi::IBuffer* GetVertexBuffer() const { return m_vertexBuffer; }
    nvrhi::IBuffer* GetDrawArgsBuffer() const { return m_drawArgsBuffer; }
    u32 GetLastVisibleCount() const { return m_lastVisibleCount; }

private:
    bool CreateShaders();
    bool CreateBuffers();
    bool CreatePipelines();
    void ClearParticleBuffer();
    u32 GetSimCBSize() const;

    fg::RenderDevice* m_device = nullptr;
    bool m_initialized = false;
    u32 m_lastVisibleCount = 0;
    u32 m_simCBSize = sizeof(RainSimParams);

    nvrhi::ShaderHandle m_updateCS;
    nvrhi::ShaderHandle m_billboardCS;
    nvrhi::ComputePipelineHandle m_updatePipeline;
    nvrhi::ComputePipelineHandle m_billboardPipeline;
    nvrhi::BindingLayoutHandle m_updateLayout;
    nvrhi::BindingLayoutHandle m_billboardLayout;
    nvrhi::BindingSetHandle m_updateBindingSet;
    nvrhi::BindingSetHandle m_billboardBindingSet;
    nvrhi::SamplerHandle m_pointSampler;

    nvrhi::BufferHandle m_particleBuffer;
    nvrhi::BufferHandle m_visibleIndicesBuffer;
    nvrhi::BufferHandle m_visibleCountBuffer;
    nvrhi::BufferHandle m_vertexBuffer;
    nvrhi::BufferHandle m_drawArgsBuffer;
    nvrhi::BufferHandle m_updateParamsCB;
    nvrhi::BufferHandle m_billboardParamsCB;
};

} // namespace xray::render::fg::passes
