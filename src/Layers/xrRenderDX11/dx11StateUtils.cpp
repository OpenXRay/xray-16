#include "stdafx.h"
#include "dx11StateUtils.h"
#include "Layers/xrRender/Utils/dxHashHelper.h"

namespace xray::render::fg
{
namespace dx11StateUtils
{
void ResetDescription(D3D_RASTERIZER_DESC& desc)
{
    ZeroMemory(&desc, sizeof(desc));
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_BACK;
    desc.FrontCounterClockwise = FALSE;
    desc.DepthBias = 0;
    desc.DepthBiasClamp = 0.0f;
    desc.SlopeScaledDepthBias = 0.0f;
    desc.DepthClipEnable = TRUE;
    desc.ScissorEnable = FALSE;
    if (RImplementation.o.msaa)
        desc.MultisampleEnable = TRUE;
    else
        desc.MultisampleEnable = FALSE;
    desc.AntialiasedLineEnable = FALSE;
}

void ResetDescription(D3D_DEPTH_STENCIL_DESC& desc)
{
    ZeroMemory(&desc, sizeof(desc));
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_LESS;
    desc.StencilEnable = TRUE;
    if (!RImplementation.o.msaa)
    {
        desc.StencilReadMask = 0xFF;
        desc.StencilWriteMask = 0xFF;
    }
    else
    {
        desc.StencilReadMask = 0x7F;
        desc.StencilWriteMask = 0x7F;
    }

    desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
}

void ResetDescription(D3D_BLEND_DESC& desc)
{
    ZeroMemory(&desc, sizeof(desc));

    desc.AlphaToCoverageEnable = FALSE;
    desc.IndependentBlendEnable = FALSE;

    for (int i = 0; i < 8; ++i)
    {
        desc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
        desc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[i].BlendEnable = FALSE;
        desc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    }
}

void ResetDescription(D3D_SAMPLER_DESC& desc)
{
    ZeroMemory(&desc, sizeof(desc));

    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.MipLODBias = 0;
    desc.MaxAnisotropy = 1;
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    desc.BorderColor[0] = 1.0f;
    desc.BorderColor[1] = 1.0f;
    desc.BorderColor[2] = 1.0f;
    desc.BorderColor[3] = 1.0f;
    desc.MinLOD = -FLT_MAX;
    desc.MaxLOD = FLT_MAX;
}

bool operator==(const D3D_RASTERIZER_DESC& desc1, const D3D_RASTERIZER_DESC& desc2)
{
    if (desc1.FillMode != desc2.FillMode) return false;
    if (desc1.CullMode != desc2.CullMode) return false;
    if (desc1.FrontCounterClockwise != desc2.FrontCounterClockwise) return false;
    if (desc1.DepthBias != desc2.DepthBias) return false;
    if (desc1.DepthBiasClamp != desc2.DepthBiasClamp) return false;
    if (desc1.SlopeScaledDepthBias != desc2.SlopeScaledDepthBias) return false;
    if (desc1.DepthClipEnable != desc2.DepthClipEnable) return false;
    if (desc1.ScissorEnable != desc2.ScissorEnable) return false;
    if (desc1.MultisampleEnable != desc2.MultisampleEnable) return false;
    if (desc1.AntialiasedLineEnable != desc2.AntialiasedLineEnable) return false;
    return true;
}

bool operator==(const D3D_DEPTH_STENCIL_DESC& desc1, const D3D_DEPTH_STENCIL_DESC& desc2)
{
    if (desc1.DepthEnable != desc2.DepthEnable) return false;
    if (desc1.DepthEnable)
    {
        if (desc1.DepthWriteMask != desc2.DepthWriteMask) return false;
        if (desc1.DepthFunc != desc2.DepthFunc) return false;
    }
    if (desc1.StencilEnable != desc2.StencilEnable) return false;
    if (desc1.StencilEnable)
    {
        if (desc1.StencilReadMask != desc2.StencilReadMask) return false;
        if (desc1.StencilWriteMask != desc2.StencilWriteMask) return false;
        if (desc1.FrontFace.StencilFailOp != desc2.FrontFace.StencilFailOp) return false;
        if (desc1.FrontFace.StencilDepthFailOp != desc2.FrontFace.StencilDepthFailOp) return false;
        if (desc1.FrontFace.StencilPassOp != desc2.FrontFace.StencilPassOp) return false;
        if (desc1.FrontFace.StencilFunc != desc2.FrontFace.StencilFunc) return false;
        if (desc1.BackFace.StencilFailOp != desc2.BackFace.StencilFailOp) return false;
        if (desc1.BackFace.StencilDepthFailOp != desc2.BackFace.StencilDepthFailOp) return false;
        if (desc1.BackFace.StencilPassOp != desc2.BackFace.StencilPassOp) return false;
        if (desc1.BackFace.StencilFunc != desc2.BackFace.StencilFunc) return false;
    }
    return true;
}

bool operator==(const D3D_BLEND_DESC& desc1, const D3D_BLEND_DESC& desc2)
{
    if (desc1.AlphaToCoverageEnable != desc2.AlphaToCoverageEnable) return false;
    if (desc1.IndependentBlendEnable != desc2.IndependentBlendEnable) return false;
    for (int i = 0; i < 4; ++i)
    {
        if (desc1.RenderTarget[i].SrcBlend != desc2.RenderTarget[i].SrcBlend) return false;
        if (desc1.RenderTarget[i].DestBlend != desc2.RenderTarget[i].DestBlend) return false;
        if (desc1.RenderTarget[i].BlendOp != desc2.RenderTarget[i].BlendOp) return false;
        if (desc1.RenderTarget[i].SrcBlendAlpha != desc2.RenderTarget[i].SrcBlendAlpha) return false;
        if (desc1.RenderTarget[i].DestBlendAlpha != desc2.RenderTarget[i].DestBlendAlpha) return false;
        if (desc1.RenderTarget[i].BlendOpAlpha != desc2.RenderTarget[i].BlendOpAlpha) return false;
        if (desc1.RenderTarget[i].BlendEnable != desc2.RenderTarget[i].BlendEnable) return false;
        if (desc1.RenderTarget[i].RenderTargetWriteMask != desc2.RenderTarget[i].RenderTargetWriteMask) return false;
    }
    return true;
}

bool operator==(const D3D_SAMPLER_DESC& desc1, const D3D_SAMPLER_DESC& desc2)
{
    if (desc1.Filter != desc2.Filter) return false;
    if (desc1.AddressU != desc2.AddressU) return false;
    if (desc1.AddressV != desc2.AddressV) return false;
    if (desc1.AddressW != desc2.AddressW) return false;
    if (desc1.ComparisonFunc != desc2.ComparisonFunc) return false;
    if (desc1.BorderColor[0] != desc2.BorderColor[0]) return false;
    if (desc1.BorderColor[1] != desc2.BorderColor[1]) return false;
    if (desc1.BorderColor[2] != desc2.BorderColor[2]) return false;
    if (desc1.BorderColor[3] != desc2.BorderColor[3]) return false;
    if (desc1.MinLOD != desc2.MinLOD) return false;
    if (desc1.MaxLOD != desc2.MaxLOD) return false;
    return true;
}

u32 GetHash(const D3D_RASTERIZER_DESC& desc)
{
    dxHashHelper Hash;
    Hash.AddData(&desc.FillMode, sizeof(desc.FillMode));
    Hash.AddData(&desc.CullMode, sizeof(desc.CullMode));
    Hash.AddData(&desc.FrontCounterClockwise, sizeof(desc.FrontCounterClockwise));
    Hash.AddData(&desc.DepthBias, sizeof(desc.DepthBias));
    Hash.AddData(&desc.DepthBiasClamp, sizeof(desc.DepthBiasClamp));
    Hash.AddData(&desc.SlopeScaledDepthBias, sizeof(desc.SlopeScaledDepthBias));
    Hash.AddData(&desc.DepthClipEnable, sizeof(desc.DepthClipEnable));
    Hash.AddData(&desc.ScissorEnable, sizeof(desc.ScissorEnable));
    Hash.AddData(&desc.MultisampleEnable, sizeof(desc.MultisampleEnable));
    Hash.AddData(&desc.AntialiasedLineEnable, sizeof(desc.AntialiasedLineEnable));
    return Hash.GetHash();
}

u32 GetHash(const D3D_DEPTH_STENCIL_DESC& desc)
{
    dxHashHelper Hash;
    Hash.AddData(&desc.DepthEnable, sizeof(desc.DepthEnable));
    Hash.AddData(&desc.DepthWriteMask, sizeof(desc.DepthWriteMask));
    Hash.AddData(&desc.DepthFunc, sizeof(desc.DepthFunc));
    Hash.AddData(&desc.StencilEnable, sizeof(desc.StencilEnable));
    Hash.AddData(&desc.StencilReadMask, sizeof(desc.StencilReadMask));
    Hash.AddData(&desc.StencilWriteMask, sizeof(desc.StencilWriteMask));
    Hash.AddData(&desc.FrontFace.StencilFailOp, sizeof(desc.FrontFace.StencilFailOp));
    Hash.AddData(&desc.FrontFace.StencilDepthFailOp, sizeof(desc.FrontFace.StencilDepthFailOp));
    Hash.AddData(&desc.FrontFace.StencilPassOp, sizeof(desc.FrontFace.StencilPassOp));
    Hash.AddData(&desc.FrontFace.StencilFunc, sizeof(desc.FrontFace.StencilFunc));
    Hash.AddData(&desc.BackFace.StencilFailOp, sizeof(desc.BackFace.StencilFailOp));
    Hash.AddData(&desc.BackFace.StencilDepthFailOp, sizeof(desc.BackFace.StencilDepthFailOp));
    Hash.AddData(&desc.BackFace.StencilPassOp, sizeof(desc.BackFace.StencilPassOp));
    Hash.AddData(&desc.BackFace.StencilFunc, sizeof(desc.BackFace.StencilFunc));
    return Hash.GetHash();
}

u32 GetHash(const D3D_BLEND_DESC& desc)
{
    dxHashHelper Hash;
    Hash.AddData(&desc.AlphaToCoverageEnable, sizeof(desc.AlphaToCoverageEnable));
    Hash.AddData(&desc.IndependentBlendEnable, sizeof(desc.IndependentBlendEnable));
    for (int i = 0; i < 8; ++i)
    {
        Hash.AddData(&desc.RenderTarget[i].SrcBlend, sizeof(desc.RenderTarget[i].SrcBlend));
        Hash.AddData(&desc.RenderTarget[i].DestBlend, sizeof(desc.RenderTarget[i].DestBlend));
        Hash.AddData(&desc.RenderTarget[i].BlendOp, sizeof(desc.RenderTarget[i].BlendOp));
        Hash.AddData(&desc.RenderTarget[i].SrcBlendAlpha, sizeof(desc.RenderTarget[i].SrcBlendAlpha));
        Hash.AddData(&desc.RenderTarget[i].DestBlendAlpha, sizeof(desc.RenderTarget[i].DestBlendAlpha));
        Hash.AddData(&desc.RenderTarget[i].BlendOpAlpha, sizeof(desc.RenderTarget[i].BlendOpAlpha));
        Hash.AddData(&desc.RenderTarget[i].BlendEnable, sizeof(desc.RenderTarget[i].BlendEnable));
        Hash.AddData(&desc.RenderTarget[i].RenderTargetWriteMask, sizeof(desc.RenderTarget[i].RenderTargetWriteMask));
    }
    return Hash.GetHash();
}

u32 GetHash(const D3D_SAMPLER_DESC& desc)
{
    dxHashHelper Hash;
    Hash.AddData(&desc.Filter, sizeof(desc.Filter));
    Hash.AddData(&desc.AddressU, sizeof(desc.AddressU));
    Hash.AddData(&desc.AddressV, sizeof(desc.AddressV));
    Hash.AddData(&desc.AddressW, sizeof(desc.AddressW));
    Hash.AddData(&desc.ComparisonFunc, sizeof(desc.ComparisonFunc));
    Hash.AddData(&desc.BorderColor[0], sizeof(desc.BorderColor[0]));
    Hash.AddData(&desc.BorderColor[1], sizeof(desc.BorderColor[1]));
    Hash.AddData(&desc.BorderColor[2], sizeof(desc.BorderColor[2]));
    Hash.AddData(&desc.BorderColor[3], sizeof(desc.BorderColor[3]));
    Hash.AddData(&desc.MinLOD, sizeof(desc.MinLOD));
    Hash.AddData(&desc.MaxLOD, sizeof(desc.MaxLOD));
    return Hash.GetHash();
}

void ValidateState(D3D_RASTERIZER_DESC&) {}

void ValidateState(D3D_DEPTH_STENCIL_DESC& desc)
{
    VERIFY((desc.DepthEnable == 0) || (desc.DepthEnable == 1));
    VERIFY((desc.StencilEnable == 0) || (desc.StencilEnable == 1));
    if (!desc.DepthEnable)
    {
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_LESS;
    }
    if (!desc.StencilEnable)
    {
        desc.StencilReadMask = 0xFF;
        desc.StencilWriteMask = 0xFF;
        desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    }
}

void ValidateState(D3D_BLEND_DESC& desc)
{
    BOOL bBlendEnable = FALSE;
    for (int i = 0; i < 8; ++i)
    {
        VERIFY((desc.RenderTarget[i].BlendEnable == 0) || (desc.RenderTarget[i].BlendEnable == 1));
        bBlendEnable |= desc.RenderTarget[i].BlendEnable;
    }
    for (int i = 0; i < 8; ++i)
    {
        if (!bBlendEnable)
        {
            desc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
            desc.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
            desc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
            desc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
            desc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        }
        else
        {
            switch (desc.RenderTarget[i].SrcBlendAlpha)
            {
            case D3D11_BLEND_SRC_COLOR:    desc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA; break;
            case D3D11_BLEND_INV_SRC_COLOR:desc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA; break;
            case D3D11_BLEND_DEST_COLOR:   desc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_DEST_ALPHA; break;
            case D3D11_BLEND_INV_DEST_COLOR:desc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA; break;
            case D3D11_BLEND_SRC1_COLOR:   desc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_SRC1_ALPHA; break;
            case D3D11_BLEND_INV_SRC1_COLOR:desc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_INV_SRC1_ALPHA; break;
            default: break;
            }
            switch (desc.RenderTarget[i].DestBlendAlpha)
            {
            case D3D11_BLEND_SRC_COLOR:    desc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA; break;
            case D3D11_BLEND_INV_SRC_COLOR:desc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA; break;
            case D3D11_BLEND_DEST_COLOR:   desc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA; break;
            case D3D11_BLEND_INV_DEST_COLOR:desc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA; break;
            case D3D11_BLEND_SRC1_COLOR:   desc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_SRC1_ALPHA; break;
            case D3D11_BLEND_INV_SRC1_COLOR:desc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_INV_SRC1_ALPHA; break;
            default: break;
            }
        }
    }
}

void ValidateState(D3D_SAMPLER_DESC& desc)
{
    if ((desc.AddressU != D3D11_TEXTURE_ADDRESS_BORDER) && (desc.AddressV != D3D11_TEXTURE_ADDRESS_BORDER) &&
        (desc.AddressW != D3D11_TEXTURE_ADDRESS_BORDER))
    {
        for (int i = 0; i < 4; ++i)
            desc.BorderColor[i] = 0.0f;
    }
    if ((desc.Filter != D3D11_FILTER_ANISOTROPIC) && (desc.Filter != D3D11_FILTER_COMPARISON_ANISOTROPIC))
        desc.MaxAnisotropy = 1;
}
}
}
