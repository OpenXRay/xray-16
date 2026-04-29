// ParticleGPUCullingManager.h
// GPU-driven particle culling and billboard generation
#pragma once

#include "PassVertexFormats.h"
#include "xrCore/xrCore.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    namespace fg {
        class RenderDevice;
        class RenderContext;
    }
}

namespace xray::render::fg::passes {

// Constant buffer for culling parameters
struct ParticleCullParams {
    Fmatrix viewProj;           // 64 bytes
    Fvector4 frustumPlanes[6];  // 96 bytes
    Fvector4 cameraPos;         // 16 bytes (xyz + pad)
    Fvector4 cameraTop;         // 16 bytes (xyz + pad)
    Fvector4 cameraRight;       // 16 bytes (xyz + pad)
    u32 particleCount;          //  4 bytes
    u32 hiZWidth;               //  4 bytes
    u32 hiZHeight;              //  4 bytes
    u32 hiZMipLevels;           //  4 bytes
    // Total: 224 bytes
};
static_assert(sizeof(ParticleCullParams) == 224, "ParticleCullParams must be 224 bytes");

// Constant buffer for billboard generation
struct ParticleBillboardParams {
    Fvector4 cameraTop;     // 16 bytes (xyz + pad)
    Fvector4 cameraRight;   // 16 bytes (xyz + pad)
    u32 visibleCount;       //  4 bytes
    u32 padding[3];         // 12 bytes
    // Total: 48 bytes
};
static_assert(sizeof(ParticleBillboardParams) == 48, "ParticleBillboardParams must be 48 bytes");

class ParticleGPUCullingManager {
public:
    ParticleGPUCullingManager() = default;
    ~ParticleGPUCullingManager();

    // Initialize GPU resources
    bool Initialize(fg::RenderDevice* device, u32 maxParticles);

    // Cleanup
    void Shutdown();

    // Check if ready for use
    bool IsReady() const { return m_initialized; }

    // Get max particle capacity
    u32 GetMaxParticles() const { return m_maxParticles; }

    // Upload particle data from CPU (call before culling)
    void UploadParticleData(
        nvrhi::ICommandList* cmdList,
        const xr_vector<GPUParticleData>& particles);

    void DispatchCulling(
        nvrhi::ICommandList* cmdList,
        nvrhi::ITexture* hiZPyramid,
        u32 particleCount,
        u32 hiZWidth,
        u32 hiZHeight,
        u32 hiZMipLevels);

    void DispatchBillboardGeneration(
        nvrhi::ICommandList* cmdList,
        u32 maxVisibleCount);

    // Clear visible count buffer (call before culling)
    void ClearVisibleCount(nvrhi::ICommandList* cmdList);

    // Getters for rendering
    nvrhi::IBuffer* GetVertexBuffer() const { return m_vertexBuffer.Get(); }
    nvrhi::IBuffer* GetDrawArgsBuffer() const { return m_drawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetVisibleCountBuffer() const { return m_visibleCountBuffer.Get(); }
    nvrhi::IBuffer* GetParticleDataBuffer() const { return m_particleDataBuffer.Get(); }

private:
    bool CreateShaders();
    bool CreateBuffers();
    bool CreateBindingLayouts();
    bool CreatePipelines();

    fg::RenderDevice* m_device = nullptr;
    u32 m_maxParticles = 0;
    bool m_initialized = false;

    // Compute shaders
    nvrhi::ShaderHandle m_cullShader;
    nvrhi::ShaderHandle m_billboardShader;

    // Compute pipelines
    nvrhi::ComputePipelineHandle m_cullPipeline;
    nvrhi::ComputePipelineHandle m_billboardPipeline;

    // Binding layouts
    nvrhi::BindingLayoutHandle m_cullLayout;
    nvrhi::BindingLayoutHandle m_billboardLayout;

    // Buffers
    nvrhi::BufferHandle m_particleDataBuffer;    // Input: particle data (GPUParticleData[])
    nvrhi::BufferHandle m_visibleIndicesBuffer;  // Output: visible particle indices
    nvrhi::BufferHandle m_visibleCountBuffer;    // Output: visible count (single u32)
    nvrhi::BufferHandle m_vertexBuffer;          // Output: billboard vertices
    nvrhi::BufferHandle m_drawArgsBuffer;        // Output: indirect draw args

    // Samplers
    nvrhi::SamplerHandle m_pointClampSampler;

    // Binding sets (cached, recreated when needed)
    nvrhi::BindingSetHandle m_cullBindingSet;
    nvrhi::BindingSetHandle m_billboardBindingSet;
};

} // namespace xray::render::fg::passes
