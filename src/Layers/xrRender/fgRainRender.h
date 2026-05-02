#pragma once

#include "Include/xrRender/RainRender.h"
#include "xrEngine/Rain.h"
#include "Layers/xrRender/IRenderDetailModel.h"
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

    bool HasWork() const { return !m_quadBatches.empty(); }
    void Clear();

    void Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer);

private:
    void InitResources();
    void EnsureGeometryCapacity(size_t vertexCount, size_t indexCount);
    void DrawBatches(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer,
                     const xr_vector<Batch>& batches, nvrhi::ITexture* texture);

    IRender_DetailModel* m_dropModel = nullptr;

    xr_vector<Vertex>   m_vertices;
    xr_vector<u16>      m_indices;
    xr_vector<Batch>    m_quadBatches;

    nvrhi::IDevice*                  m_device = nullptr;
    nvrhi::TextureHandle             m_streakTexture;
    nvrhi::ShaderHandle              m_vs;
    nvrhi::ShaderHandle              m_ps;
    nvrhi::InputLayoutHandle         m_inputLayout;
    nvrhi::SamplerHandle             m_sampler;
    nvrhi::BindingLayoutHandle       m_bindingLayout;
    nvrhi::BindingSetHandle          m_bindingSet;
    nvrhi::BufferHandle              m_constantBuffer;
    nvrhi::BufferHandle              m_vertexBuffer;
    nvrhi::BufferHandle              m_indexBuffer;
    size_t                           m_vertexCapacity = 0;
    size_t                           m_indexCapacity  = 0;
    nvrhi::GraphicsPipelineHandle    m_pipeline;
};
}
