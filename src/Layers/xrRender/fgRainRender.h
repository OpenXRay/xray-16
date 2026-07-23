#pragma once

#include "Include/xrRender/RainRender.h"
#include "xrEngine/Rain.h"
#include "Layers/xrRender/IRenderDetailModel.h"
#include "Layers/xrRender/FrameGraphPasses/RainGPUSimManager.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg
{
class FGRainRender : public IRainRender
{
public:
    struct Vertex
    {
        float x, y, z;
        u32   color;
        float u, v;
    };
    static_assert(sizeof(Vertex) == sizeof(IRender_DetailModel::fvfVertexOut));

    struct Batch
    {
        u32 indexOffset;
        u32 indexCount;
    };

    FGRainRender();
    ~FGRainRender() override;

    void Copy(IRainRender& _in) override;
    void Render(CEffect_Rain& owner) override;
    const Fsphere& GetDropBounds() const override;

    bool HasWork() const;
    void Clear();

    void SetRainShadowInputs(nvrhi::ITexture* rainSM, const Fmatrix& sampleVP, bool valid);

    void Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer,
              nvrhi::ITexture* sceneWorldPos,
              const passes::RainHeightmapInfo& heightmap);

private:
    void InitResources();
    void EnsureGeometryCapacity(size_t vertexCount, size_t indexCount);
    void DrawBatches(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer,
                     const xr_vector<Batch>& batches, nvrhi::ITexture* colorTex,
                     nvrhi::ITexture* sceneWorldPos);
    void AppendSplashParticles(CEffect_Rain& owner, u32 rainColor);
    void SimulateRainItems(CEffect_Rain& owner);
    void BuildCPUStreaks(CEffect_Rain& owner);
    void DrawGPUStreaks(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer,
                        nvrhi::ITexture* sceneWorldPos);

    IRender_DetailModel* m_dropModel = nullptr;

    xr_vector<Vertex>   m_vertices;
    xr_vector<u16>      m_indices;
    xr_vector<Batch>    m_quadBatches;
    xr_vector<Batch>    m_splashBatches;

    passes::RainGPUSimManager m_gpuSim;
    bool m_gpuSimReady = false;

    u32   m_desiredParticles = 0;
    u32   m_rainColor = 0;
    float m_factorVisual = 1.f;
    float m_rainDensity = 0.f;

    nvrhi::ITexture* m_rainShadow = nullptr;
    Fmatrix          m_rainSampleVP;
    bool             m_rainSMValid = false;

    nvrhi::IDevice*                  m_device = nullptr;
    nvrhi::TextureHandle             m_streakTexture;
    nvrhi::TextureHandle             m_splashTexture;
    nvrhi::ShaderHandle              m_vs;
    nvrhi::ShaderHandle              m_ps;
    nvrhi::InputLayoutHandle         m_inputLayout;
    nvrhi::SamplerHandle             m_sampler;
    nvrhi::BindingLayoutHandle       m_bindingLayout;
    nvrhi::BufferHandle              m_constantBuffer;
    nvrhi::BufferHandle              m_vertexBuffer;
    nvrhi::BufferHandle              m_indexBuffer;
    size_t                           m_vertexCapacity = 0;
    size_t                           m_indexCapacity  = 0;
    nvrhi::GraphicsPipelineHandle    m_pipeline;
};
}
