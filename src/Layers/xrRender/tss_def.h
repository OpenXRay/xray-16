#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::fg
{
enum class SamplerFilter : u8
{
    Point        = 0,
    Linear       = 1,
    Anisotropic  = 2,
};

class SimulatorStates
{
public:
    nvrhi::BlendState        blend;
    nvrhi::DepthStencilState depthStencil;
    nvrhi::RasterState       raster;
    nvrhi::SamplerDesc       samplers[16];
    bool                     samplerUsed[16] = {};
    u32                      alphaRef = 0;
    bool                     alphaTestEnable = false;
    bool                     alphaBlendEnable = false;

    SimulatorStates() = default;

    void SetDepthEnable(bool b)                          { depthStencil.depthTestEnable = b; }
    void SetDepthWrite(bool b)                           { depthStencil.depthWriteEnable = b; }
    void SetDepthFunc(nvrhi::ComparisonFunc f)           { depthStencil.depthFunc = f; }

    void SetStencilEnable(bool b)                        { depthStencil.stencilEnable = b; }
    void SetStencilReadMask(u8 m)                        { depthStencil.stencilReadMask = m; }
    void SetStencilWriteMask(u8 m)                       { depthStencil.stencilWriteMask = m; }
    void SetStencilRef(u8 v)                             { depthStencil.stencilRefValue = v; }
    void SetFrontStencilFail(nvrhi::StencilOp op)        { depthStencil.frontFaceStencil.failOp = op; }
    void SetFrontStencilDepthFail(nvrhi::StencilOp op)   { depthStencil.frontFaceStencil.depthFailOp = op; }
    void SetFrontStencilPass(nvrhi::StencilOp op)        { depthStencil.frontFaceStencil.passOp = op; }
    void SetFrontStencilFunc(nvrhi::ComparisonFunc f)    { depthStencil.frontFaceStencil.stencilFunc = f; }
    void SetBackStencilFail(nvrhi::StencilOp op)         { depthStencil.backFaceStencil.failOp = op; }
    void SetBackStencilDepthFail(nvrhi::StencilOp op)    { depthStencil.backFaceStencil.depthFailOp = op; }
    void SetBackStencilPass(nvrhi::StencilOp op)         { depthStencil.backFaceStencil.passOp = op; }
    void SetBackStencilFunc(nvrhi::ComparisonFunc f)     { depthStencil.backFaceStencil.stencilFunc = f; }

    void SetAlphaToCoverage(bool b)                      { blend.alphaToCoverageEnable = b; }
    void SetBlendEnable(bool b)                          { alphaBlendEnable = b; for (auto& rt : blend.targets) rt.blendEnable = b; }
    void SetSrcBlend(nvrhi::BlendFactor f)               { for (auto& rt : blend.targets) rt.srcBlend = f; }
    void SetDestBlend(nvrhi::BlendFactor f)              { for (auto& rt : blend.targets) rt.destBlend = f; }
    void SetBlendOp(nvrhi::BlendOp op)                   { for (auto& rt : blend.targets) rt.blendOp = op; }
    void SetSrcBlendAlpha(nvrhi::BlendFactor f)          { for (auto& rt : blend.targets) rt.srcBlendAlpha = f; }
    void SetDestBlendAlpha(nvrhi::BlendFactor f)         { for (auto& rt : blend.targets) rt.destBlendAlpha = f; }
    void SetBlendOpAlpha(nvrhi::BlendOp op)              { for (auto& rt : blend.targets) rt.blendOpAlpha = op; }
    void SetColorWriteMask(int rt, nvrhi::ColorMask m)   { blend.targets[rt].colorWriteMask = m; }

    void SetCullMode(nvrhi::RasterCullMode m)            { raster.cullMode = m; }
    void SetFillMode(nvrhi::RasterFillMode m)            { raster.fillMode = m; }
    void SetScissor(bool b)                              { raster.scissorEnable = b; }

    void SetAlphaTest(bool b)                            { alphaTestEnable = b; }
    void SetAlphaRef(u32 r)                              { alphaRef = r; }

    void SetSamplerAddress(u32 slot, nvrhi::SamplerAddressMode mode);
    void SetSamplerAddress(u32 slot, nvrhi::SamplerAddressMode u, nvrhi::SamplerAddressMode v, nvrhi::SamplerAddressMode w);
    void SetSamplerAddressU(u32 slot, nvrhi::SamplerAddressMode mode);
    void SetSamplerAddressV(u32 slot, nvrhi::SamplerAddressMode mode);
    void SetSamplerAddressW(u32 slot, nvrhi::SamplerAddressMode mode);
    void SetSamplerFilter(u32 slot, bool _min, bool _mip, bool _mag);
    void SetSamplerFilterMin(u32 slot, bool linear);
    void SetSamplerFilterMip(u32 slot, bool linear);
    void SetSamplerFilterMag(u32 slot, bool linear);
    void SetSamplerAnisotropic(u32 slot, u32 level);
    void SetSamplerComparison(u32 slot, bool enable);

    void SetSamplerBorderColor(u32 slot, u32 packed_argb);
    void SetSamplerMipLODBias(u32 slot, float bias);

    bool IsAlphaTestEnabled() const                      { return alphaTestEnable; }
    bool IsAlphaBlendEnabled() const                     { return alphaBlendEnable; }

    BOOL equal(const SimulatorStates& other) const;
    void clear();
    void Invalidate() { clear(); }
    void record(void*& state);
};
}
