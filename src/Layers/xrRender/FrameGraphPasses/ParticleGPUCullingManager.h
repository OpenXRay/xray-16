#pragma once

#include "ParticlePassSetup.h"
#include "xrCore/xrCore.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    namespace ng {
        class RenderDevice;
        class RenderContext;
    }
}

namespace xray::render::RENDER_NAMESPACE::passes {

struct ParticleCullParams {
    Fmatrix viewProj;
    Fvector4 frustumPlanes[6];
    Fvector4 cameraPos;
    Fvector4 cameraTop;
    Fvector4 cameraRight;
    u32 particleCount;
    u32 hiZWidth;
    u32 hiZHeight;
    u32 hiZMipLevels;
};
static_assert(sizeof(ParticleCullParams) == 224, "ParticleCullParams must be 224 bytes");

struct ParticleBillboardParams {
    Fvector4 cameraTop;
    Fvector4 cameraRight;
    u32 visibleCount;
    u32 padding[3];
};
static_assert(sizeof(ParticleBillboardParams) == 48, "ParticleBillboardParams must be 48 bytes");

class ParticleGPUCullingManager {
public:
    ParticleGPUCullingManager() = default;
    ~ParticleGPUCullingManager();

    bool Initialize(ng::RenderDevice* device, u32 maxParticles);

    void Shutdown();

    bool IsReady() const { return m_initialized; }

    u32 GetMaxParticles() const { return m_maxParticles; }

    void UploadParticleData(
        nvrhi::ICommandList* cmdList,
        const xr_vector<GPUParticleData>& particles);

    void DispatchCulling(
        nvrhi::ICommandList* cmdList,
        nvrhi::ITexture* hiZPyramid,
        const Fmatrix& viewProj,
        const Fvector4* frustumPlanes,
        const Fvector& cameraPos,
        const Fvector& cameraTop,
        const Fvector& cameraRight,
        u32 particleCount,
        u32 hiZWidth,
        u32 hiZHeight,
        u32 hiZMipLevels);

    void DispatchBillboardGeneration(
        nvrhi::ICommandList* cmdList,
        const Fvector& cameraTop,
        const Fvector& cameraRight,
        u32 maxVisibleCount);

    void ClearVisibleCount(nvrhi::ICommandList* cmdList);

    nvrhi::IBuffer* GetVertexBuffer() const { return m_vertexBuffer.Get(); }
    nvrhi::IBuffer* GetDrawArgsBuffer() const { return m_drawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetVisibleCountBuffer() const { return m_visibleCountBuffer.Get(); }
    nvrhi::IBuffer* GetParticleDataBuffer() const { return m_particleDataBuffer.Get(); }

private:
    bool CreateShaders();
    bool CreateBuffers();
    bool CreateBindingLayouts();
    bool CreatePipelines();

    ng::RenderDevice* m_device = nullptr;
    u32 m_maxParticles = 0;
    bool m_initialized = false;

    nvrhi::ShaderHandle m_cullShader;
    nvrhi::ShaderHandle m_billboardShader;

    nvrhi::ComputePipelineHandle m_cullPipeline;
    nvrhi::ComputePipelineHandle m_billboardPipeline;

    nvrhi::BindingLayoutHandle m_cullLayout;
    nvrhi::BindingLayoutHandle m_billboardLayout;

    nvrhi::BufferHandle m_particleDataBuffer;
    nvrhi::BufferHandle m_visibleIndicesBuffer;
    nvrhi::BufferHandle m_visibleCountBuffer;
    nvrhi::BufferHandle m_vertexBuffer;
    nvrhi::BufferHandle m_drawArgsBuffer;

    nvrhi::BufferHandle m_cullParamsCB;
    nvrhi::BufferHandle m_billboardParamsCB;

    nvrhi::SamplerHandle m_pointClampSampler;

    nvrhi::BindingSetHandle m_cullBindingSet;
    nvrhi::BindingSetHandle m_billboardBindingSet;
};

}
