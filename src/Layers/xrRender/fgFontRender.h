#pragma once

#include "Include/xrRender/FontRender.h"
#include "xrEngine/GameFont.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg
{
class FGFontRender : public IFontRender
{
public:
    struct Vertex
    {
        float x, y, z, w;
        u32   color;
        float u, v;
    };

    FGFontRender();
    ~FGFontRender() override;

    void Initialize(cpcstr cShader, cpcstr cTexture) override;
    void OnRender(CGameFont& owner) override;

    bool HasWork() const { return !m_vertices.empty(); }
    void Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer);

private:
    void InitResources();
    void EnsureVertexCapacity(size_t vertexCount);
    void EnsurePipeline(nvrhi::IFramebuffer* framebuffer);
    void BuildGeometry(CGameFont& owner);

    nvrhi::IDevice*               m_device = nullptr;
    nvrhi::TextureHandle          m_texture;
    nvrhi::ShaderHandle           m_vs;
    nvrhi::ShaderHandle           m_ps;
    nvrhi::InputLayoutHandle      m_inputLayout;
    nvrhi::SamplerHandle          m_sampler;
    nvrhi::BindingLayoutHandle    m_bindingLayout;
    nvrhi::BindingSetHandle       m_bindingSet;
    nvrhi::BufferHandle           m_constantBuffer;
    nvrhi::BufferHandle           m_vertexBuffer;
    nvrhi::BufferHandle           m_indexBuffer;
    size_t                        m_vertexCapacity = 0;
    nvrhi::GraphicsPipelineHandle m_pipeline;

    xr_vector<Vertex>             m_vertices;
    Fvector2                      m_textureSize{ 0.f, 0.f };
};
}
