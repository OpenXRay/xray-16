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
        bool depthTested;
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

    void DispatchVisibility(nvrhi::ICommandList* cmdList, nvrhi::ITexture* depth);
    void Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer);

private:
    void InitResources();
    void EnsureGeometryCapacity(size_t vertexCount, size_t indexCount);
    nvrhi::ITexture* ResolveTexture(const shared_str& name);
    void PushQuad(const Fvector& center, const Fvector& vecX, const Fvector& vecY, u32 color, nvrhi::ITexture* tex,
        bool depthTested = false);

    xr_vector<Vertex> m_vertices;
    xr_vector<u16> m_indices;
    xr_vector<Batch> m_batches;
    xr_map<shared_str, nvrhi::TextureHandle> m_textureCache;
    xr_unordered_map<nvrhi::ITexture*, nvrhi::BindingSetHandle> m_sourceBindingSetCache;
    xr_unordered_map<nvrhi::ITexture*, nvrhi::BindingSetHandle> m_overlayBindingSetCache;
    xr_unordered_map<nvrhi::ITexture*, nvrhi::BindingSetHandle> m_visBindingSetCache;

    nvrhi::IDevice* m_device = nullptr;
    nvrhi::ShaderHandle m_vs;
    nvrhi::ShaderHandle m_ps;
    nvrhi::ShaderHandle m_vsOverlay;
    nvrhi::ShaderHandle m_visCS;
    nvrhi::InputLayoutHandle m_inputLayout;
    nvrhi::InputLayoutHandle m_inputLayoutOverlay;
    nvrhi::SamplerHandle m_sampler;
    nvrhi::BindingLayoutHandle m_bindingLayout;
    nvrhi::BindingLayoutHandle m_overlayBindingLayout;
    nvrhi::BindingLayoutHandle m_visBindingLayout;
    nvrhi::BufferHandle m_constantBuffer;
    nvrhi::BufferHandle m_visConstantBuffer;
    nvrhi::BufferHandle m_visBuffer;
    nvrhi::BufferHandle m_vertexBuffer;
    nvrhi::BufferHandle m_indexBuffer;
    size_t m_vertexCapacity = 0;
    size_t m_indexCapacity = 0;
    nvrhi::GraphicsPipelineHandle m_pipelineSource;
    nvrhi::GraphicsPipelineHandle m_pipelineOverlay;
    nvrhi::ComputePipelineHandle m_visPipeline;
    bool m_visInitialized = false;
    bool m_sunValid = false;
    Fvector2 m_sunPosPx{};
    float m_sunRadiusPx = 16.f;
};
} // namespace xray::render::fg
