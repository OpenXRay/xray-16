#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRenderDX11/ResourceManager.h"
#include "Layers/xrRenderDX11/Blender_Recorder.h"
#include "Layers/xrRenderDX11/Blender.h"
#include "Layers/xrRender/tss.h"
#include "Layers/xrRender/FrameGraph/ShaderCache.h"

namespace xray::render::fg
{
void fix_texture_name(pstr fn);

void CBlender_Compile::r_Stencil(BOOL Enable, nvrhi::ComparisonFunc Func, u32 Mask, u32 WriteMask, nvrhi::StencilOp Fail, nvrhi::StencilOp Pass, nvrhi::StencilOp ZFail)
{
    RS.SetStencilEnable(BC(Enable));
    if (!Enable)
        return;
    RS.SetFrontStencilFunc(Func);
    RS.SetStencilReadMask((u8)(Mask));
    RS.SetStencilWriteMask((u8)(WriteMask));
    RS.SetFrontStencilFail(Fail);
    RS.SetFrontStencilPass(Pass);
    RS.SetFrontStencilDepthFail(ZFail);
    RS.SetBackStencilFunc(Func);
    RS.SetBackStencilFail(Fail);
    RS.SetBackStencilPass(Pass);
    RS.SetBackStencilDepthFail(ZFail);
}

void CBlender_Compile::r_StencilRef(u32 Ref) { RS.SetStencilRef((u8)(Ref)); }
void CBlender_Compile::r_CullMode(nvrhi::RasterCullMode Mode) { RS.SetCullMode(Mode); }
void CBlender_Compile::r_dx11Texture(LPCSTR ResourceName, LPCSTR texture, bool recursive /*= false*/)
{
    VERIFY(ResourceName);
    if (!texture || !texture[0])
    {
        return;
    }

    string256 TexName;
    xr_strcpy(TexName, texture);
    fix_texture_name(TexName);

    // Get binding slot directly from pixel shader reflection
    u32 stage = u32(-1);
    SPS* ps = dest.ps._get();
    if (ps && ps->reflection)
    {
        const auto& inputTextures = ps->reflection->rtBindings.inputTextures;
        for (const auto& tex : inputTextures)
        {
            if (0 == xr_strcmp(tex.name.c_str(), ResourceName))
            {
                // Pixel shader textures use rstPixel offset
                stage = tex.slot + CTexture::rstPixel;
                break;
            }
        }
    }

    if (stage == u32(-1))
    {
        return;
    }

    passTextures.emplace_back(stage, shared_str(TexName));
}

void CBlender_Compile::i_dx11FilterAnizo(u32 s, BOOL value)
{
    VERIFY(s != u32(-1));
    RS.SetSamplerAnisotropic(s, value ? 16 : 1);
}

u32 CBlender_Compile::r_dx11Sampler(LPCSTR ResourceName)
{
    // TODO: DX11: Check if we can use dwStage
    u32 stage = i_Sampler(ResourceName);

    if (stage == u32(-1))
        return u32(-1);

    //	init defaults here:

    //	Use nvrhi::SamplerAddressMode::Clamp,	SamplerFilter::Point,			SamplerFilter::Point,	SamplerFilter::Point
    if (0 == xr_strcmp(ResourceName, "smp_nofilter"))
    {
        i_Address(stage, nvrhi::SamplerAddressMode::Clamp);
        i_Filter(stage, SamplerFilter::Point, SamplerFilter::Point, SamplerFilter::Point);
    }

    //	Use nvrhi::SamplerAddressMode::Clamp,	SamplerFilter::Linear,			SamplerFilter::Point,	SamplerFilter::Linear
    else if (0 == xr_strcmp(ResourceName, "smp_rtlinear"))
    {
        i_Address(stage, nvrhi::SamplerAddressMode::Clamp);
        i_Filter(stage, SamplerFilter::Linear, SamplerFilter::Point, SamplerFilter::Linear);
    }

    //	Use	nvrhi::SamplerAddressMode::Wrap,	SamplerFilter::Linear,			SamplerFilter::Linear,	SamplerFilter::Linear
    else if (0 == xr_strcmp(ResourceName, "smp_linear"))
    {
        i_Address(stage, nvrhi::SamplerAddressMode::Wrap);
        i_Filter(stage, SamplerFilter::Linear, SamplerFilter::Linear, SamplerFilter::Linear);
    }

    //	Use nvrhi::SamplerAddressMode::Wrap,	SamplerFilter::Anisotropic, 	SamplerFilter::Linear,	SamplerFilter::Anisotropic
    else if (0 == xr_strcmp(ResourceName, "smp_base"))
    {
        i_Address(stage, nvrhi::SamplerAddressMode::Wrap);
        i_dx11FilterAnizo(stage, TRUE);
        // i_Filter(stage, SamplerFilter::Linear, SamplerFilter::Linear, SamplerFilter::Linear);
    }

    //	Use nvrhi::SamplerAddressMode::Clamp,	SamplerFilter::Linear,			SamplerFilter::Point,	SamplerFilter::Linear
    else if (0 == xr_strcmp(ResourceName, "smp_material"))
    {
        i_Address(stage, nvrhi::SamplerAddressMode::Clamp);
        i_Filter(stage, SamplerFilter::Linear, SamplerFilter::Point, SamplerFilter::Linear);
        RS.SetSamplerAddressW(stage, nvrhi::SamplerAddressMode::Wrap);
    }

    else if (0 == xr_strcmp(ResourceName, "smp_smap"))
    {
        i_Address(stage, nvrhi::SamplerAddressMode::Clamp);
        i_Filter(stage, SamplerFilter::Linear, SamplerFilter::Point, SamplerFilter::Linear);
        RS.SetSamplerComparison(stage, true);
    }

    else if (0 == xr_strcmp(ResourceName, "smp_jitter"))
    {
        i_Address(stage, nvrhi::SamplerAddressMode::Wrap);
        i_Filter(stage, SamplerFilter::Point, SamplerFilter::Point, SamplerFilter::Point);
    }

    return stage;
}

void CBlender_Compile::r_Pass(LPCSTR _vs, LPCSTR _gs, LPCSTR _ps, bool bFog, BOOL bZtest, BOOL bZwrite, BOOL bABlend,
                              nvrhi::BlendFactor abSRC, nvrhi::BlendFactor abDST, BOOL aTest, u32 aRef)
{
    RS.Invalidate();
    ctable.clear();
    passTextures.clear();
    passMatrices.clear();
    passConstants.clear();
    dwStage = 0;

    // Setup FF-units (Z-buffer, blender)
    PassSET_ZB(bZtest, bZwrite);
    PassSET_Blend(bABlend, abSRC, abDST, aTest, aRef);
    PassSET_LightFog(FALSE, bFog);

    // Create shaders
    SPS* ps = RImplementation.Resources->_CreatePS(_ps);
    u32 flags = 0;
    SVS* vs = RImplementation.Resources->_CreateVS(_vs, flags);
    SGS* gs = RImplementation.Resources->_CreateGS(_gs);
    dest.ps = ps;
    dest.vs = vs;
    dest.gs = gs;
    dest.hs = RImplementation.Resources->_CreateHS("null");
    dest.ds = RImplementation.Resources->_CreateDS("null");
    dest.cs = RImplementation.Resources->_CreateCS("null");

    // Last Stage - disable
    if (0 == xr_stricmp(_ps, "null"))
    {
    }
}

void CBlender_Compile::r_TessPass(LPCSTR vs, LPCSTR hs, LPCSTR ds, LPCSTR gs, LPCSTR ps, bool bFog, BOOL bZtest,
    BOOL bZwrite, BOOL bABlend, nvrhi::BlendFactor abSRC, nvrhi::BlendFactor abDST, BOOL aTest, u32 aRef)
{
    r_Pass(vs, gs, ps, bFog, bZtest, bZwrite, bABlend, abSRC, abDST, aTest, aRef);

    dest.hs = RImplementation.Resources->_CreateHS(hs);
    dest.ds = RImplementation.Resources->_CreateDS(ds);
}

void CBlender_Compile::r_ComputePass(LPCSTR cs)
{
    dest.cs = RImplementation.Resources->_CreateCS(cs);
}
} // namespace xray::render::fg
