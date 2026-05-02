#pragma once

#include <nvrhi/nvrhi.h>
#include "Include/xrRender/LensFlareRender.h"
#include "xrEngine/xr_efflensflare.h"

namespace xray::render::fg
{
class FGLensFlareRender : public ILensFlareRender
{
public:
    struct Vertex
    {
        float x, y, z;
        u32 color;
        float u, v;
    };

    struct Batch
    {
        u32 indexOffset;
        u32 indexCount;
        nvrhi::ITexture* texture;
    };

    FGLensFlareRender();
    ~FGLensFlareRender() override;

    void Copy(ILensFlareRender& _in) override;
    void Render(CLensFlare& owner, BOOL bSun, BOOL bFlares, BOOL bGradient) override;

    void OnDeviceCreate() override {}

    void OnDeviceDestroy() override {}

    bool HasWork() const
    {
        return !m_batches.empty();
    }

    void Clear();

    void Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer);

private:
    void InitResources();
    void EnsureGeometryCapacity(size_t vertexCount, size_t indexCount);
    nvrhi::ITexture* ResolveTexture(const shared_str& name);
    void PushQuad(const Fvector& center, const Fvector& vecX, const Fvector& vecY, u32 color, nvrhi::ITexture* tex);

    xr_vector<Vertex> m_vertices;
    xr_vector<u16> m_indices;
    xr_vector<Batch> m_batches;
    xr_map<shared_str, nvrhi::TextureHandle> m_textureCache;
    xr_unordered_map<nvrhi::ITexture*, nvrhi::BindingSetHandle> m_bindingSetCache;

    nvrhi::IDevice* m_device = nullptr;
    nvrhi::ShaderHandle m_vs;
    nvrhi::ShaderHandle m_ps;
    nvrhi::InputLayoutHandle m_inputLayout;
    nvrhi::SamplerHandle m_sampler;
    nvrhi::BindingLayoutHandle m_bindingLayout;
    nvrhi::BufferHandle m_constantBuffer;
    nvrhi::BufferHandle m_vertexBuffer;
    nvrhi::BufferHandle m_indexBuffer;
    size_t m_vertexCapacity = 0;
    size_t m_indexCapacity = 0;
    nvrhi::GraphicsPipelineHandle m_pipeline;
};
} // namespace xray::render::fg
