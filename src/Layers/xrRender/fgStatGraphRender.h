#pragma once

#include "Include/xrRender/StatGraphRender.h"
#include "xrEngine/StatGraph.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg
{
class FGStatGraphRender : public IStatGraphRender
{
public:
    struct Vertex
    {
        float x, y, z;
        u32   color;
    };

    struct DrawItem
    {
        nvrhi::PrimitiveType topology;
        u32 vertexOffset;
        u32 vertexCount;
    };

    FGStatGraphRender();
    ~FGStatGraphRender() override;

    void Copy(IStatGraphRender& _in) override;
    void OnDeviceCreate() override;
    void OnDeviceDestroy() override;
    void OnRender(CStatGraph& owner) override;

    bool HasWork() const { return !m_drawItems.empty(); }
    void Clear();

    void Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, u32 screenWidth, u32 screenHeight);

private:
    void RenderBack(CStatGraph& owner);
    void RenderBars(CStatGraph& owner, CStatGraph::ElementsDeq& pelements);
    void RenderBarLines(CStatGraph& owner, CStatGraph::ElementsDeq& pelements);
    void RenderLines(CStatGraph& owner, CStatGraph::ElementsDeq& pelements);
    void RenderMarkers(CStatGraph& owner, CStatGraph::MarkersDeq& pmarkers);

    void Begin(nvrhi::PrimitiveType topology);
    void Push(float x, float y, u32 color);

    bool EnsurePipelines(nvrhi::IDevice* device, nvrhi::IFramebuffer* framebuffer);
    void EnsureGeometryCapacity(nvrhi::IDevice* device, size_t vertexCount);
    nvrhi::GraphicsPipelineHandle GetPipeline(nvrhi::PrimitiveType topology, nvrhi::IDevice* device, nvrhi::IFramebuffer* framebuffer);

    xr_vector<Vertex>   m_vertices;
    xr_vector<DrawItem> m_drawItems;
    DrawItem            m_currentItem{};
    bool                m_inItem = false;

    nvrhi::ShaderHandle              m_vs;
    nvrhi::ShaderHandle              m_ps;
    nvrhi::InputLayoutHandle         m_inputLayout;
    nvrhi::BindingLayoutHandle       m_bindingLayout;
    nvrhi::BindingSetHandle          m_bindingSet;
    nvrhi::BufferHandle              m_constantBuffer;
    nvrhi::BufferHandle              m_vertexBuffer;
    size_t                           m_vertexBufferCapacity = 0;
    nvrhi::GraphicsPipelineHandle    m_pipelineTri;
    nvrhi::GraphicsPipelineHandle    m_pipelineLine;
};
}
