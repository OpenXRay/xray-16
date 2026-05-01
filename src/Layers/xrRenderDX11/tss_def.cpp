#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/tss_def.h"

#if defined(USE_DX11)
#include "Layers/xrRenderDX11/dx11StateUtils.h"
#elif defined(USE_OGL)
#include "../xrRenderGL/glState.h"
#endif

namespace xray::render::fg
{
namespace
{
D3D_TEXTURE_ADDRESS_MODE NvrhiToD3DAddress(nvrhi::SamplerAddressMode m)
{
    switch (m)
    {
    case nvrhi::SamplerAddressMode::Clamp:      return D3D11_TEXTURE_ADDRESS_CLAMP;
    case nvrhi::SamplerAddressMode::Wrap:       return D3D11_TEXTURE_ADDRESS_WRAP;
    case nvrhi::SamplerAddressMode::Border:     return D3D11_TEXTURE_ADDRESS_BORDER;
    case nvrhi::SamplerAddressMode::Mirror:     return D3D11_TEXTURE_ADDRESS_MIRROR;
    case nvrhi::SamplerAddressMode::MirrorOnce: return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
    }
    return D3D11_TEXTURE_ADDRESS_WRAP;
}
}

void SimulatorStates::SetSamplerAddress(u32 slot, nvrhi::SamplerAddressMode u, nvrhi::SamplerAddressMode v, nvrhi::SamplerAddressMode w)
{
    samplerUsed[slot] = true;
    samplers[slot].addressU = u;
    samplers[slot].addressV = v;
    samplers[slot].addressW = w;
}

void SimulatorStates::SetSamplerAddress(u32 slot, nvrhi::SamplerAddressMode mode)
{
    SetSamplerAddress(slot, mode, mode, mode);
}

void SimulatorStates::SetSamplerAddressU(u32 slot, nvrhi::SamplerAddressMode mode)
{
    samplerUsed[slot] = true;
    samplers[slot].addressU = mode;
}

void SimulatorStates::SetSamplerAddressV(u32 slot, nvrhi::SamplerAddressMode mode)
{
    samplerUsed[slot] = true;
    samplers[slot].addressV = mode;
}

void SimulatorStates::SetSamplerAddressW(u32 slot, nvrhi::SamplerAddressMode mode)
{
    samplerUsed[slot] = true;
    samplers[slot].addressW = mode;
}

void SimulatorStates::SetSamplerFilter(u32 slot, bool _min, bool _mip, bool _mag)
{
    samplerUsed[slot] = true;
    samplers[slot].minFilter = _min;
    samplers[slot].mipFilter = _mip;
    samplers[slot].magFilter = _mag;
}

void SimulatorStates::SetSamplerFilterMin(u32 slot, bool linear)
{
    samplerUsed[slot] = true;
    samplers[slot].minFilter = linear;
}

void SimulatorStates::SetSamplerFilterMip(u32 slot, bool linear)
{
    samplerUsed[slot] = true;
    samplers[slot].mipFilter = linear;
}

void SimulatorStates::SetSamplerFilterMag(u32 slot, bool linear)
{
    samplerUsed[slot] = true;
    samplers[slot].magFilter = linear;
}

void SimulatorStates::SetSamplerAnisotropic(u32 slot, u32 level)
{
    samplerUsed[slot] = true;
    samplers[slot].maxAnisotropy = static_cast<float>(level);
}

void SimulatorStates::SetSamplerComparison(u32 slot, bool enable)
{
    samplerUsed[slot] = true;
    samplers[slot].reductionType = enable ? nvrhi::SamplerReductionType::Comparison : nvrhi::SamplerReductionType::Standard;
}

void SimulatorStates::SetSamplerBorderColor(u32 slot, u32 packed_argb)
{
    samplerUsed[slot] = true;
    samplers[slot].borderColor.r = ((packed_argb >> 16) & 0xff) / 255.0f;
    samplers[slot].borderColor.g = ((packed_argb >> 8)  & 0xff) / 255.0f;
    samplers[slot].borderColor.b = ((packed_argb)       & 0xff) / 255.0f;
    samplers[slot].borderColor.a = ((packed_argb >> 24) & 0xff) / 255.0f;
}

void SimulatorStates::SetSamplerMipLODBias(u32 slot, float bias)
{
    samplerUsed[slot] = true;
    samplers[slot].mipBias = bias;
}

void SimulatorStates::record(ID3DState*& state)
{
#if defined(USE_DX11)
    state = ID3DState::Create(*this);
#elif defined(USE_OGL)
    state = ID3DState::Create();
#else
#   error No graphics API selected or enabled!
#endif
}

void SimulatorStates::clear() { *this = SimulatorStates{}; }

BOOL SimulatorStates::equal(const SimulatorStates& other) const
{
    return std::memcmp(this, &other, sizeof(*this)) == 0;
}

#if defined(USE_DX11)
void SimulatorStates::UpdateState(dx11State& state) const
{
    state.UpdateStencilRef(depthStencil.stencilRefValue);
    state.UpdateAlphaRef(alphaRef);
}

void SimulatorStates::UpdateDesc(D3D_RASTERIZER_DESC& desc) const
{
    desc.FillMode      = (raster.fillMode == nvrhi::RasterFillMode::Wireframe) ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    switch (raster.cullMode)
    {
    case nvrhi::RasterCullMode::None:  desc.CullMode = D3D11_CULL_NONE;  break;
    case nvrhi::RasterCullMode::Front: desc.CullMode = D3D11_CULL_FRONT; break;
    case nvrhi::RasterCullMode::Back:  desc.CullMode = D3D11_CULL_BACK;  break;
    }
    desc.ScissorEnable = raster.scissorEnable;
}

void SimulatorStates::UpdateDesc(D3D_DEPTH_STENCIL_DESC& desc) const
{
    desc.DepthEnable    = depthStencil.depthTestEnable ? 1 : 0;
    desc.DepthWriteMask = depthStencil.depthWriteEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc      = static_cast<D3D11_COMPARISON_FUNC>(depthStencil.depthFunc);

    desc.StencilEnable    = depthStencil.stencilEnable ? 1 : 0;
    desc.StencilReadMask  = depthStencil.stencilReadMask;
    desc.StencilWriteMask = depthStencil.stencilWriteMask;

    desc.FrontFace.StencilFailOp      = static_cast<D3D11_STENCIL_OP>(depthStencil.frontFaceStencil.failOp);
    desc.FrontFace.StencilDepthFailOp = static_cast<D3D11_STENCIL_OP>(depthStencil.frontFaceStencil.depthFailOp);
    desc.FrontFace.StencilPassOp      = static_cast<D3D11_STENCIL_OP>(depthStencil.frontFaceStencil.passOp);
    desc.FrontFace.StencilFunc        = static_cast<D3D11_COMPARISON_FUNC>(depthStencil.frontFaceStencil.stencilFunc);

    desc.BackFace.StencilFailOp      = static_cast<D3D11_STENCIL_OP>(depthStencil.backFaceStencil.failOp);
    desc.BackFace.StencilDepthFailOp = static_cast<D3D11_STENCIL_OP>(depthStencil.backFaceStencil.depthFailOp);
    desc.BackFace.StencilPassOp      = static_cast<D3D11_STENCIL_OP>(depthStencil.backFaceStencil.passOp);
    desc.BackFace.StencilFunc        = static_cast<D3D11_COMPARISON_FUNC>(depthStencil.backFaceStencil.stencilFunc);
}

void SimulatorStates::UpdateDesc(D3D_BLEND_DESC& desc) const
{
    desc.AlphaToCoverageEnable = blend.alphaToCoverageEnable ? 1 : 0;
    for (int i = 0; i < 8; ++i)
    {
        const auto& rt = blend.targets[i];
        desc.RenderTarget[i].BlendEnable           = rt.blendEnable ? 1 : 0;
        desc.RenderTarget[i].SrcBlend              = static_cast<D3D11_BLEND>(rt.srcBlend);
        desc.RenderTarget[i].DestBlend             = static_cast<D3D11_BLEND>(rt.destBlend);
        desc.RenderTarget[i].BlendOp               = static_cast<D3D11_BLEND_OP>(rt.blendOp);
        desc.RenderTarget[i].SrcBlendAlpha         = static_cast<D3D11_BLEND>(rt.srcBlendAlpha);
        desc.RenderTarget[i].DestBlendAlpha        = static_cast<D3D11_BLEND>(rt.destBlendAlpha);
        desc.RenderTarget[i].BlendOpAlpha          = static_cast<D3D11_BLEND_OP>(rt.blendOpAlpha);
        desc.RenderTarget[i].RenderTargetWriteMask = static_cast<u8>(rt.colorWriteMask);
    }
}

void SimulatorStates::UpdateDesc(D3D_SAMPLER_DESC descArray[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT],
    bool SamplerUsed[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT], int iBaseSamplerIndex) const
{
    for (int slot = 0; slot < 16; ++slot)
    {
        if (!samplerUsed[slot])
            continue;

        int outIndex = slot - iBaseSamplerIndex;
        if (outIndex < 0 || outIndex >= D3D_COMMONSHADER_SAMPLER_SLOT_COUNT)
            continue;

        const nvrhi::SamplerDesc& src = samplers[slot];
        D3D_SAMPLER_DESC& desc = descArray[outIndex];
        SamplerUsed[outIndex] = true;

        constexpr int FilterMipLinear   = 0x01;
        constexpr int FilterMagLinear   = 0x04;
        constexpr int FilterMinLinear   = 0x10;
        constexpr int FilterAnisotropic = 0x40;
        constexpr int FilterComparison  = 0x80;

        int filter = 0;
        if (src.minFilter) filter |= FilterMinLinear;
        if (src.mipFilter) filter |= FilterMipLinear;
        if (src.magFilter) filter |= FilterMagLinear;
        if (src.maxAnisotropy > 1.f) filter |= FilterAnisotropic | FilterMinLinear | FilterMipLinear | FilterMagLinear;
        if (src.reductionType == nvrhi::SamplerReductionType::Comparison) filter |= FilterComparison;
        desc.Filter = static_cast<D3D11_FILTER>(filter);

        desc.AddressU      = NvrhiToD3DAddress(src.addressU);
        desc.AddressV      = NvrhiToD3DAddress(src.addressV);
        desc.AddressW      = NvrhiToD3DAddress(src.addressW);
        desc.MipLODBias    = src.mipBias;
        desc.MaxAnisotropy = static_cast<u32>(src.maxAnisotropy);

        desc.BorderColor[0] = src.borderColor.r;
        desc.BorderColor[1] = src.borderColor.g;
        desc.BorderColor[2] = src.borderColor.b;
        desc.BorderColor[3] = src.borderColor.a;

        if (desc.MinLOD > desc.MaxLOD)
            desc.MaxLOD = desc.MinLOD;
    }
}
#endif
}
