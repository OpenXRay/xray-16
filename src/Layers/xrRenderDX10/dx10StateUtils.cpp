#include "stdafx.h"
#include "dx10StateUtils.h"
#include "Layers/xrRender/Utils/dxHashHelper.h"

namespace dx10StateUtils
{
D3D_CULL_MODE ConvertCullMode(D3DCULL Mode)
{
    switch (Mode)
    {
    case D3DCULL_NONE: return D3D_CULL_NONE;
    case D3DCULL_CW: return D3D_CULL_FRONT;
    case D3DCULL_CCW: return D3D_CULL_BACK;
    default: VERIFY(!"Unexpected cull mode!"); return D3D_CULL_NONE;
    }
}

D3D_COMPARISON_FUNC ConvertCmpFunction(D3DCMPFUNC Func)
{
    switch (Func)
    {
    case D3DCMP_NEVER: return D3D_COMPARISON_NEVER;
    case D3DCMP_LESS: return D3D_COMPARISON_LESS;
    case D3DCMP_EQUAL: return D3D_COMPARISON_EQUAL;
    case D3DCMP_LESSEQUAL: return D3D_COMPARISON_LESS_EQUAL;
    case D3DCMP_GREATER: return D3D_COMPARISON_GREATER;
    case D3DCMP_NOTEQUAL: return D3D_COMPARISON_NOT_EQUAL;
    case D3DCMP_GREATEREQUAL: return D3D_COMPARISON_GREATER_EQUAL;
    case D3DCMP_ALWAYS: return D3D_COMPARISON_ALWAYS;
    default: VERIFY(!"ConvertCmpFunction can't convert argument!"); return D3D_COMPARISON_ALWAYS;
    }
}

D3D_STENCIL_OP ConvertStencilOp(D3DSTENCILOP Op)
{
    switch (Op)
    {
    case D3DSTENCILOP_KEEP: return D3D_STENCIL_OP_KEEP;
    case D3DSTENCILOP_ZERO: return D3D_STENCIL_OP_ZERO;
    case D3DSTENCILOP_REPLACE: return D3D_STENCIL_OP_REPLACE;
    case D3DSTENCILOP_INCRSAT: return D3D_STENCIL_OP_INCR_SAT;
    case D3DSTENCILOP_DECRSAT: return D3D_STENCIL_OP_DECR_SAT;
    case D3DSTENCILOP_INVERT: return D3D_STENCIL_OP_INVERT;
    case D3DSTENCILOP_INCR: return D3D_STENCIL_OP_INCR;
    case D3DSTENCILOP_DECR: return D3D_STENCIL_OP_DECR;
    default: VERIFY(!"ConvertStencilOp can't convert argument!"); return D3D_STENCIL_OP_KEEP;
    }
}

D3D_BLEND ConvertBlendArg(D3DBLEND Arg)
{
    switch (Arg)
    {
    case D3DBLEND_ZERO: return D3D_BLEND_ZERO;
    case D3DBLEND_ONE: return D3D_BLEND_ONE;
    case D3DBLEND_SRCCOLOR: return D3D_BLEND_SRC_COLOR;
    case D3DBLEND_INVSRCCOLOR: return D3D_BLEND_INV_SRC_COLOR;
    case D3DBLEND_SRCALPHA: return D3D_BLEND_SRC_ALPHA;
    case D3DBLEND_INVSRCALPHA: return D3D_BLEND_INV_SRC_ALPHA;
    case D3DBLEND_DESTALPHA: return D3D_BLEND_DEST_ALPHA;
    case D3DBLEND_INVDESTALPHA: return D3D_BLEND_INV_DEST_ALPHA;
    case D3DBLEND_DESTCOLOR: return D3D_BLEND_DEST_COLOR;
    case D3DBLEND_INVDESTCOLOR: return D3D_BLEND_INV_DEST_COLOR;
    case D3DBLEND_SRCALPHASAT:
        return D3D_BLEND_SRC_ALPHA_SAT;
    // case D3DBLEND_BOTHSRCALPHA:
    //	return ;
    // case D3DBLEND_BOTHINVSRCALPHA:
    //	return ;
    case D3DBLEND_BLENDFACTOR: return D3D_BLEND_BLEND_FACTOR;
    case D3DBLEND_INVBLENDFACTOR: return D3D_BLEND_INV_BLEND_FACTOR;
    case D3DBLEND_SRCCOLOR2: return D3D_BLEND_SRC1_COLOR;
    case D3DBLEND_INVSRCCOLOR2: return D3D_BLEND_INV_SRC1_COLOR;
    default: VERIFY(!"ConvertBlendArg can't convert argument!"); return D3D_BLEND_ONE;
    }
}

D3D_BLEND_OP ConvertBlendOp(D3DBLENDOP Op)
{
    switch (Op)
    {
    case D3DBLENDOP_ADD: return D3D_BLEND_OP_ADD;
    case D3DBLENDOP_SUBTRACT: return D3D_BLEND_OP_SUBTRACT;
    case D3DBLENDOP_REVSUBTRACT: return D3D_BLEND_OP_REV_SUBTRACT;
    case D3DBLENDOP_MIN: return D3D_BLEND_OP_MIN;
    case D3DBLENDOP_MAX: return D3D_BLEND_OP_MAX;
    default: VERIFY(!"ConvertBlendOp can't convert argument!"); return D3D_BLEND_OP_ADD;
    }
}

D3D_TEXTURE_ADDRESS_MODE ConvertTextureAddressMode(D3DTEXTUREADDRESS Mode)
{
    switch (Mode)
    {
    case D3DTADDRESS_WRAP: return D3D_TEXTURE_ADDRESS_WRAP;
    case D3DTADDRESS_MIRROR: return D3D_TEXTURE_ADDRESS_MIRROR;
    case D3DTADDRESS_CLAMP: return D3D_TEXTURE_ADDRESS_CLAMP;
    case D3DTADDRESS_BORDER: return D3D_TEXTURE_ADDRESS_BORDER;
    case D3DTADDRESS_MIRRORONCE: return D3D_TEXTURE_ADDRESS_MIRROR_ONCE;
    default: VERIFY(!"ConvertTextureAddressMode can't convert argument!"); return D3D_TEXTURE_ADDRESS_CLAMP;
    }
}

void ResetDescription(D3D_RASTERIZER_DESC& desc)
{
    ZeroMemory(&desc, sizeof(desc));
    desc.FillMode = D3D_FILL_SOLID;
    desc.CullMode = D3D_CULL_BACK;
    desc.FrontCounterClockwise = FALSE;
    desc.DepthBias = 0;
    desc.DepthBiasClamp = 0.0f;
    desc.SlopeScaledDepthBias = 0.0f;
    desc.DepthClipEnable = TRUE;
    desc.ScissorEnable = FALSE;
    if (RImplementation.o.dx10_msaa)
        desc.MultisampleEnable = TRUE;
    else
        desc.MultisampleEnable = FALSE;
    desc.AntialiasedLineEnable = FALSE;
}

void ResetDescription(D3D_DEPTH_STENCIL_DESC& desc)
{
    ZeroMemory(&desc, sizeof(desc));
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D_COMPARISON_LESS;
    desc.StencilEnable = TRUE;
    if (!RImplementation.o.dx10_msaa)
    {
        desc.StencilReadMask = 0xFF;
        desc.StencilWriteMask = 0xFF;
    }
    else
    {
        desc.StencilReadMask = 0x7F;
        desc.StencilWriteMask = 0x7F;
    }

    desc.FrontFace.StencilFailOp = D3D_STENCIL_OP_KEEP;
    desc.FrontFace.StencilDepthFailOp = D3D_STENCIL_OP_KEEP;
    desc.FrontFace.StencilPassOp = D3D_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFunc = D3D_COMPARISON_ALWAYS;

    desc.BackFace.StencilFailOp = D3D_STENCIL_OP_KEEP;
    desc.BackFace.StencilDepthFailOp = D3D_STENCIL_OP_KEEP;
    desc.BackFace.StencilPassOp = D3D_STENCIL_OP_KEEP;
    desc.BackFace.StencilFunc = D3D_COMPARISON_ALWAYS;
}

#ifdef USE_DX11
void ResetDescription(D3D_BLEND_DESC& desc)
{
    ZeroMemory(&desc, sizeof(desc));

    desc.AlphaToCoverageEnable = FALSE;
    desc.IndependentBlendEnable = FALSE;

    for (int i = 0; i < 8; ++i)
    {
        desc.RenderTarget[i].SrcBlend = D3D_BLEND_ONE;
        desc.RenderTarget[i].DestBlend = D3D_BLEND_ZERO;
        desc.RenderTarget[i].BlendOp = D3D_BLEND_OP_ADD;
        desc.RenderTarget[i].SrcBlendAlpha = D3D_BLEND_ONE;
        desc.RenderTarget[i].DestBlendAlpha = D3D_BLEND_ZERO;
        desc.RenderTarget[i].BlendOpAlpha = D3D_BLEND_OP_ADD;
        desc.RenderTarget[i].BlendEnable = FALSE;
        desc.RenderTarget[i].RenderTargetWriteMask = D3D_COLOR_WRITE_ENABLE_ALL;
    }
}
#else
void ResetDescription(D3D_BLEND_DESC& desc)
{
    ZeroMemory(&desc, sizeof(desc));

    desc.AlphaToCoverageEnable = FALSE;
    desc.SrcBlend = D3D_BLEND_ONE;
    desc.DestBlend = D3D_BLEND_ZERO;
    desc.BlendOp = D3D_BLEND_OP_ADD;
    desc.SrcBlendAlpha = D3D_BLEND_ONE;
    desc.DestBlendAlpha = D3D_BLEND_ZERO;
    desc.BlendOpAlpha = D3D_BLEND_OP_ADD;

    for (int i = 0; i < 8; ++i)
    {
        desc.BlendEnable[i] = FALSE;
        desc.RenderTargetWriteMask[i] = D3D_COLOR_WRITE_ENABLE_ALL;
    }
}
#endif

void ResetDescription(D3D_SAMPLER_DESC& desc)
{
    ZeroMemory(&desc, sizeof(desc));

    desc.Filter = D3D_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = D3D_TEXTURE_ADDRESS_CLAMP;
    desc.AddressV = D3D_TEXTURE_ADDRESS_CLAMP;
    desc.AddressW = D3D_TEXTURE_ADDRESS_CLAMP;
    desc.MipLODBias = 0;
    desc.MaxAnisotropy = 1;
    desc.ComparisonFunc = D3D_COMPARISON_NEVER;
    desc.BorderColor[0] = 1.0f;
    desc.BorderColor[1] = 1.0f;
    desc.BorderColor[2] = 1.0f;
    desc.BorderColor[3] = 1.0f;
    desc.MinLOD = -FLT_MAX;
    desc.MaxLOD = FLT_MAX;
}

bool operator==(const D3D_RASTERIZER_DESC& desc1, const D3D_RASTERIZER_DESC& desc2)
{
    if (desc1.FillMode != desc2.FillMode)
        return false;
    if (desc1.CullMode != desc2.CullMode)
        return false;
    if (desc1.FrontCounterClockwise != desc2.FrontCounterClockwise)
        return false;
    if (desc1.DepthBias != desc2.DepthBias)
        return false;
    if (desc1.DepthBiasClamp != desc2.DepthBiasClamp)
        return false;
    if (desc1.SlopeScaledDepthBias != desc2.SlopeScaledDepthBias)
        return false;
    if (desc1.DepthClipEnable != desc2.DepthClipEnable)
        return false;
    if (desc1.ScissorEnable != desc2.ScissorEnable)
        return false;
    if (desc1.MultisampleEnable != desc2.MultisampleEnable)
        return false;
    if (desc1.AntialiasedLineEnable != desc2.AntialiasedLineEnable)
        return false;

    return true;
}

bool operator==(const D3D_DEPTH_STENCIL_DESC& desc1, const D3D_DEPTH_STENCIL_DESC& desc2)
{
    if (desc1.DepthEnable != desc2.DepthEnable)
        return false;
    if (desc1.DepthWriteMask != desc2.DepthWriteMask)
        return false;
    if (desc1.DepthFunc != desc2.DepthFunc)
        return false;
    if (desc1.StencilEnable != desc2.StencilEnable)
        return false;
    if (desc1.StencilReadMask != desc2.StencilReadMask)
        return false;
    if (desc1.StencilWriteMask != desc2.StencilWriteMask)
        return false;

    if (desc1.FrontFace.StencilFailOp != desc2.FrontFace.StencilFailOp)
        return false;
    if (desc1.FrontFace.StencilDepthFailOp != desc2.FrontFace.StencilDepthFailOp)
        return false;
    if (desc1.FrontFace.StencilPassOp != desc2.FrontFace.StencilPassOp)
        return false;
    if (desc1.FrontFace.StencilFunc != desc2.FrontFace.StencilFunc)
        return false;

    if (desc1.BackFace.StencilFailOp != desc2.BackFace.StencilFailOp)
        return false;
    if (desc1.BackFace.StencilDepthFailOp != desc2.BackFace.StencilDepthFailOp)
        return false;
    if (desc1.BackFace.StencilPassOp != desc2.BackFace.StencilPassOp)
        return false;
    if (desc1.BackFace.StencilFunc != desc2.BackFace.StencilFunc)
        return false;

    return true;
}

#ifdef USE_DX11
bool operator==(const D3D_BLEND_DESC& desc1, const D3D_BLEND_DESC& desc2)
{
    if (desc1.AlphaToCoverageEnable != desc2.AlphaToCoverageEnable)
        return false;
    if (desc1.IndependentBlendEnable != desc2.IndependentBlendEnable)
        return false;

    for (int i = 0; i < 4 /*8*/ /*It's quick fix for dx11 port because of mispatch of desc*/; ++i)
    {
        if (desc1.RenderTarget[i].SrcBlend != desc2.RenderTarget[i].SrcBlend)
            return false;
        if (desc1.RenderTarget[i].DestBlend != desc2.RenderTarget[i].DestBlend)
            return false;
        if (desc1.RenderTarget[i].BlendOp != desc2.RenderTarget[i].BlendOp)
            return false;
        if (desc1.RenderTarget[i].SrcBlendAlpha != desc2.RenderTarget[i].SrcBlendAlpha)
            return false;
        if (desc1.RenderTarget[i].DestBlendAlpha != desc2.RenderTarget[i].DestBlendAlpha)
            return false;
        if (desc1.RenderTarget[i].BlendOpAlpha != desc2.RenderTarget[i].BlendOpAlpha)
            return false;
        if (desc1.RenderTarget[i].BlendEnable != desc2.RenderTarget[i].BlendEnable)
            return false;
        if (desc1.RenderTarget[i].RenderTargetWriteMask != desc2.RenderTarget[i].RenderTargetWriteMask)
            return false;
    }

    return true;
}
#else
bool operator==(const D3D_BLEND_DESC& desc1, const D3D_BLEND_DESC& desc2)
{
    if (desc1.AlphaToCoverageEnable != desc2.AlphaToCoverageEnable)
        return false;
    if (desc1.SrcBlend != desc2.SrcBlend)
        return false;
    if (desc1.DestBlend != desc2.DestBlend)
        return false;
    if (desc1.BlendOp != desc2.BlendOp)
        return false;
    if (desc1.SrcBlendAlpha != desc2.SrcBlendAlpha)
        return false;
    if (desc1.DestBlendAlpha != desc2.DestBlendAlpha)
        return false;
    if (desc1.BlendOpAlpha != desc2.BlendOpAlpha)
        return false;

    for (int i = 0; i < 8; ++i)
    {
        if (desc1.BlendEnable[i] != desc2.BlendEnable[i])
            return false;
        if (desc1.RenderTargetWriteMask[i] != desc2.RenderTargetWriteMask[i])
            return false;
    }

    return true;
}
#endif

bool operator==(const D3D_SAMPLER_DESC& desc1, const D3D_SAMPLER_DESC& desc2)
{
    if (desc1.Filter != desc2.Filter)
        return false;
    if (desc1.AddressU != desc2.AddressU)
        return false;
    if (desc1.AddressV != desc2.AddressV)
        return false;
    if (desc1.AddressW != desc2.AddressW)
        return false;
    //  RZ
    //  if (desc1.MipLODBias != desc2.MipLODBias)
    //    return false;
    //	Ignore anisotropy since it's set up automatically by the manager
    //	if( desc1.MaxAnisotropy != desc2.MaxAnisotropy) return false;
    if (desc1.ComparisonFunc != desc2.ComparisonFunc)
        return false;
    if (desc1.BorderColor[0] != desc2.BorderColor[0])
        return false;
    if (desc1.BorderColor[1] != desc2.BorderColor[1])
        return false;
    if (desc1.BorderColor[2] != desc2.BorderColor[2])
        return false;
    if (desc1.BorderColor[3] != desc2.BorderColor[3])
        return false;
    if (desc1.MinLOD != desc2.MinLOD)
        return false;
    if (desc1.MaxLOD != desc2.MaxLOD)
        return false;

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

#ifdef USE_DX11
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
#else
u32 GetHash(const D3D_BLEND_DESC& desc)
{
    dxHashHelper Hash;

    Hash.AddData(&desc.AlphaToCoverageEnable, sizeof(desc.AlphaToCoverageEnable));
    Hash.AddData(&desc.SrcBlend, sizeof(desc.SrcBlend));
    Hash.AddData(&desc.DestBlend, sizeof(desc.DestBlend));
    Hash.AddData(&desc.BlendOp, sizeof(desc.BlendOp));
    Hash.AddData(&desc.SrcBlendAlpha, sizeof(desc.SrcBlendAlpha));
    Hash.AddData(&desc.DestBlendAlpha, sizeof(desc.DestBlendAlpha));
    Hash.AddData(&desc.BlendOpAlpha, sizeof(desc.BlendOpAlpha));

    for (int i = 0; i < 8; ++i)
    {
        Hash.AddData(&desc.BlendEnable[i], sizeof(desc.BlendEnable[i]));
        Hash.AddData(&desc.RenderTargetWriteMask[i], sizeof(desc.RenderTargetWriteMask[i]));
    }

    return Hash.GetHash();
}
#endif

u32 GetHash(const D3D_SAMPLER_DESC& desc)
{
    dxHashHelper Hash;

    Hash.AddData(&desc.Filter, sizeof(desc.Filter));
    Hash.AddData(&desc.AddressU, sizeof(desc.AddressU));
    Hash.AddData(&desc.AddressV, sizeof(desc.AddressV));
    Hash.AddData(&desc.AddressW, sizeof(desc.AddressW));
    //  RZ
    //  Hash.AddData(&desc.MipLODBias, sizeof(desc.MipLODBias));
    //	Ignore anisotropy since it's set up automatically by the manager
    //	Hash.AddData( &desc.MaxAnisotropy, sizeof(desc.MaxAnisotropy) );
    Hash.AddData(&desc.ComparisonFunc, sizeof(desc.ComparisonFunc));
    Hash.AddData(&desc.BorderColor[0], sizeof(desc.BorderColor[0]));
    Hash.AddData(&desc.BorderColor[1], sizeof(desc.BorderColor[1]));
    Hash.AddData(&desc.BorderColor[2], sizeof(desc.BorderColor[2]));
    Hash.AddData(&desc.BorderColor[3], sizeof(desc.BorderColor[3]));
    Hash.AddData(&desc.MinLOD, sizeof(desc.MinLOD));
    Hash.AddData(&desc.MaxLOD, sizeof(desc.MaxLOD));

    return Hash.GetHash();
}

void ValidateState(D3D_RASTERIZER_DESC& desc) {}
void ValidateState(D3D_DEPTH_STENCIL_DESC& desc)
{
    VERIFY((desc.DepthEnable == 0) || (desc.DepthEnable == 1));
    VERIFY((desc.StencilEnable == 0) || (desc.StencilEnable == 1));

    if (!desc.DepthEnable)
    {
        desc.DepthWriteMask = D3D_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D_COMPARISON_LESS;
    }

    if (!desc.StencilEnable)
    {
        desc.StencilReadMask = 0xFF;
        desc.StencilWriteMask = 0xFF;

        desc.FrontFace.StencilFailOp = D3D_STENCIL_OP_KEEP;
        desc.FrontFace.StencilDepthFailOp = D3D_STENCIL_OP_KEEP;
        desc.FrontFace.StencilPassOp = D3D_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D_COMPARISON_ALWAYS;

        desc.BackFace.StencilFailOp = D3D_STENCIL_OP_KEEP;
        desc.BackFace.StencilDepthFailOp = D3D_STENCIL_OP_KEEP;
        desc.BackFace.StencilPassOp = D3D_STENCIL_OP_KEEP;
        desc.BackFace.StencilFunc = D3D_COMPARISON_ALWAYS;
    }
}

#ifdef USE_DX11
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
            desc.RenderTarget[i].SrcBlend = D3D_BLEND_ONE;
            desc.RenderTarget[i].DestBlend = D3D_BLEND_ZERO;
            desc.RenderTarget[i].BlendOp = D3D_BLEND_OP_ADD;
            desc.RenderTarget[i].SrcBlendAlpha = D3D_BLEND_ONE;
            desc.RenderTarget[i].DestBlendAlpha = D3D_BLEND_ZERO;
            desc.RenderTarget[i].BlendOpAlpha = D3D_BLEND_OP_ADD;
        }
        else
        {
            switch (desc.RenderTarget[i].SrcBlendAlpha)
            {
            case D3D_BLEND_SRC_COLOR: desc.RenderTarget[i].SrcBlendAlpha = D3D_BLEND_SRC_ALPHA; break;
            case D3D_BLEND_INV_SRC_COLOR: desc.RenderTarget[i].SrcBlendAlpha = D3D_BLEND_INV_SRC_ALPHA; break;
            case D3D_BLEND_DEST_COLOR: desc.RenderTarget[i].SrcBlendAlpha = D3D_BLEND_DEST_ALPHA; break;
            case D3D_BLEND_INV_DEST_COLOR: desc.RenderTarget[i].SrcBlendAlpha = D3D_BLEND_INV_DEST_ALPHA; break;
            case D3D_BLEND_SRC1_COLOR: desc.RenderTarget[i].SrcBlendAlpha = D3D_BLEND_SRC1_ALPHA; break;
            case D3D_BLEND_INV_SRC1_COLOR: desc.RenderTarget[i].SrcBlendAlpha = D3D_BLEND_INV_SRC1_ALPHA; break;
            }

            switch (desc.RenderTarget[i].DestBlendAlpha)
            {
            case D3D_BLEND_SRC_COLOR: desc.RenderTarget[i].DestBlendAlpha = D3D_BLEND_SRC_ALPHA; break;
            case D3D_BLEND_INV_SRC_COLOR: desc.RenderTarget[i].DestBlendAlpha = D3D_BLEND_INV_SRC_ALPHA; break;
            case D3D_BLEND_DEST_COLOR: desc.RenderTarget[i].DestBlendAlpha = D3D_BLEND_DEST_ALPHA; break;
            case D3D_BLEND_INV_DEST_COLOR: desc.RenderTarget[i].DestBlendAlpha = D3D_BLEND_INV_DEST_ALPHA; break;
            case D3D_BLEND_SRC1_COLOR: desc.RenderTarget[i].DestBlendAlpha = D3D_BLEND_SRC1_ALPHA; break;
            case D3D_BLEND_INV_SRC1_COLOR: desc.RenderTarget[i].DestBlendAlpha = D3D_BLEND_INV_SRC1_ALPHA; break;
            }
        }
    }
}
#else
void ValidateState(D3D_BLEND_DESC& desc)
{
    BOOL bBlendEnable = FALSE;

    for (int i = 0; i < 8; ++i)
    {
        VERIFY((desc.BlendEnable[i] == 0) || (desc.BlendEnable[i] == 1));
        bBlendEnable |= desc.BlendEnable[i];
    }

    if (!bBlendEnable)
    {
        desc.SrcBlend = D3D_BLEND_ONE;
        desc.DestBlend = D3D_BLEND_ZERO;
        desc.BlendOp = D3D_BLEND_OP_ADD;
        desc.SrcBlendAlpha = D3D_BLEND_ONE;
        desc.DestBlendAlpha = D3D_BLEND_ZERO;
        desc.BlendOpAlpha = D3D_BLEND_OP_ADD;
    }
    else
    {
        switch (desc.SrcBlendAlpha)
        {
        case D3D_BLEND_SRC_COLOR: desc.SrcBlendAlpha = D3D_BLEND_SRC_ALPHA; break;
        case D3D_BLEND_INV_SRC_COLOR: desc.SrcBlendAlpha = D3D_BLEND_INV_SRC_ALPHA; break;
        case D3D_BLEND_DEST_COLOR: desc.SrcBlendAlpha = D3D_BLEND_DEST_ALPHA; break;
        case D3D_BLEND_INV_DEST_COLOR: desc.SrcBlendAlpha = D3D_BLEND_INV_DEST_ALPHA; break;
        case D3D_BLEND_SRC1_COLOR: desc.SrcBlendAlpha = D3D_BLEND_SRC1_ALPHA; break;
        case D3D_BLEND_INV_SRC1_COLOR: desc.SrcBlendAlpha = D3D_BLEND_INV_SRC1_ALPHA; break;
        }

        switch (desc.DestBlendAlpha)
        {
        case D3D_BLEND_SRC_COLOR: desc.DestBlendAlpha = D3D_BLEND_SRC_ALPHA; break;
        case D3D_BLEND_INV_SRC_COLOR: desc.DestBlendAlpha = D3D_BLEND_INV_SRC_ALPHA; break;
        case D3D_BLEND_DEST_COLOR: desc.DestBlendAlpha = D3D_BLEND_DEST_ALPHA; break;
        case D3D_BLEND_INV_DEST_COLOR: desc.DestBlendAlpha = D3D_BLEND_INV_DEST_ALPHA; break;
        case D3D_BLEND_SRC1_COLOR: desc.DestBlendAlpha = D3D_BLEND_SRC1_ALPHA; break;
        case D3D_BLEND_INV_SRC1_COLOR: desc.DestBlendAlpha = D3D_BLEND_INV_SRC1_ALPHA; break;
        }
    }
}
#endif

void ValidateState(D3D_SAMPLER_DESC& desc)
{
    if ((desc.AddressU != D3D_TEXTURE_ADDRESS_BORDER) && (desc.AddressV != D3D_TEXTURE_ADDRESS_BORDER) &&
        (desc.AddressW != D3D_TEXTURE_ADDRESS_BORDER))
    {
        for (int i = 0; i < 4; ++i)
        {
            desc.BorderColor[i] = 0.0f;
        }
    }

    if ((desc.Filter != D3D_FILTER_ANISOTROPIC) && (desc.Filter != D3D_FILTER_COMPARISON_ANISOTROPIC))
    {
        desc.MaxAnisotropy = 1;
    }
}
};
