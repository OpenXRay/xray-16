#pragma once

#include "tss_def.h"

namespace xray::render::fg
{
class CSimulator
{
public:
    SimulatorStates container;

    CSimulator() { Invalidate(); }
    void Invalidate() { container.clear(); }
    SimulatorStates& GetContainer() { return container; }

    void SetDepthEnable(bool b)                          { container.SetDepthEnable(b); }
    void SetDepthWrite(bool b)                           { container.SetDepthWrite(b); }
    void SetDepthFunc(nvrhi::ComparisonFunc f)           { container.SetDepthFunc(f); }

    void SetStencilEnable(bool b)                        { container.SetStencilEnable(b); }
    void SetStencilReadMask(u8 m)                        { container.SetStencilReadMask(m); }
    void SetStencilWriteMask(u8 m)                       { container.SetStencilWriteMask(m); }
    void SetStencilRef(u8 v)                             { container.SetStencilRef(v); }
    void SetFrontStencilFail(nvrhi::StencilOp op)        { container.SetFrontStencilFail(op); }
    void SetFrontStencilDepthFail(nvrhi::StencilOp op)   { container.SetFrontStencilDepthFail(op); }
    void SetFrontStencilPass(nvrhi::StencilOp op)        { container.SetFrontStencilPass(op); }
    void SetFrontStencilFunc(nvrhi::ComparisonFunc f)    { container.SetFrontStencilFunc(f); }
    void SetBackStencilFail(nvrhi::StencilOp op)         { container.SetBackStencilFail(op); }
    void SetBackStencilDepthFail(nvrhi::StencilOp op)    { container.SetBackStencilDepthFail(op); }
    void SetBackStencilPass(nvrhi::StencilOp op)         { container.SetBackStencilPass(op); }
    void SetBackStencilFunc(nvrhi::ComparisonFunc f)     { container.SetBackStencilFunc(f); }

    void SetAlphaToCoverage(bool b)                      { container.SetAlphaToCoverage(b); }
    void SetBlendEnable(bool b)                          { container.SetBlendEnable(b); }
    void SetSrcBlend(nvrhi::BlendFactor f)               { container.SetSrcBlend(f); }
    void SetDestBlend(nvrhi::BlendFactor f)              { container.SetDestBlend(f); }
    void SetBlendOp(nvrhi::BlendOp op)                   { container.SetBlendOp(op); }
    void SetSrcBlendAlpha(nvrhi::BlendFactor f)          { container.SetSrcBlendAlpha(f); }
    void SetDestBlendAlpha(nvrhi::BlendFactor f)         { container.SetDestBlendAlpha(f); }
    void SetBlendOpAlpha(nvrhi::BlendOp op)              { container.SetBlendOpAlpha(op); }
    void SetColorWriteMask(int rt, nvrhi::ColorMask m)   { container.SetColorWriteMask(rt, m); }

    void SetCullMode(nvrhi::RasterCullMode m)            { container.SetCullMode(m); }
    void SetFillMode(nvrhi::RasterFillMode m)            { container.SetFillMode(m); }
    void SetScissor(bool b)                              { container.SetScissor(b); }

    void SetAlphaTest(bool b)                            { container.SetAlphaTest(b); }
    void SetAlphaRef(u32 r)                              { container.SetAlphaRef(r); }

    void SetSamplerAddress(u32 slot, nvrhi::SamplerAddressMode mode)                                                                  { container.SetSamplerAddress(slot, mode); }
    void SetSamplerAddress(u32 slot, nvrhi::SamplerAddressMode u, nvrhi::SamplerAddressMode v, nvrhi::SamplerAddressMode w)           { container.SetSamplerAddress(slot, u, v, w); }
    void SetSamplerAddressU(u32 slot, nvrhi::SamplerAddressMode mode)                                                                 { container.SetSamplerAddressU(slot, mode); }
    void SetSamplerAddressV(u32 slot, nvrhi::SamplerAddressMode mode)                                                                 { container.SetSamplerAddressV(slot, mode); }
    void SetSamplerAddressW(u32 slot, nvrhi::SamplerAddressMode mode)                                                                 { container.SetSamplerAddressW(slot, mode); }
    void SetSamplerFilter(u32 slot, bool _min, bool _mip, bool _mag)                                                                  { container.SetSamplerFilter(slot, _min, _mip, _mag); }
    void SetSamplerFilterMin(u32 slot, bool linear)                                                                                   { container.SetSamplerFilterMin(slot, linear); }
    void SetSamplerFilterMip(u32 slot, bool linear)                                                                                   { container.SetSamplerFilterMip(slot, linear); }
    void SetSamplerFilterMag(u32 slot, bool linear)                                                                                   { container.SetSamplerFilterMag(slot, linear); }
    void SetSamplerAnisotropic(u32 slot, u32 level)                                                                                   { container.SetSamplerAnisotropic(slot, level); }
    void SetSamplerComparison(u32 slot, bool enable)                                                                                  { container.SetSamplerComparison(slot, enable); }
    void SetSamplerBorderColor(u32 slot, u32 packed)                                                                                  { container.SetSamplerBorderColor(slot, packed); }
    void SetSamplerMipLODBias(u32 slot, float bias)                                                                                   { container.SetSamplerMipLODBias(slot, bias); }
};
}
