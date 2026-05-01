#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRenderDX11/ResourceManager.h"
#include "Blender_Recorder.h"
#include "Blender.h"

#ifdef USE_DX11
#include "Layers/xrRender/FrameGraph/ShaderCache.h"
#endif

namespace xray::render::fg
{
void fix_texture_name(pstr);

static int ParseName(LPCSTR N)
{
    if (0 == xr_strcmp(N, "$null"))
        return -1;
    if (0 == xr_strcmp(N, "$base0"))
        return 0;
    if (0 == xr_strcmp(N, "$base1"))
        return 1;
    if (0 == xr_strcmp(N, "$base2"))
        return 2;
    if (0 == xr_strcmp(N, "$base3"))
        return 3;
    if (0 == xr_strcmp(N, "$base4"))
        return 4;
    if (0 == xr_strcmp(N, "$base5"))
        return 5;
    if (0 == xr_strcmp(N, "$base6"))
        return 6;
    if (0 == xr_strcmp(N, "$base7"))
        return 7;
    return -1;
}

CBlender_Compile::CBlender_Compile() {}
CBlender_Compile::~CBlender_Compile() {}
void CBlender_Compile::_cpp_Compile(ShaderElement* _SH)
{
    SH = _SH;
    RS.Invalidate();

    detail_texture = nullptr;
    detail_scaler = nullptr;
    LPCSTR base = nullptr;
    if (bDetail && BT->canBeDetailed())
    {
        sh_list& lst = L_textures;
        int id = ParseName(BT->oT_Name);
        base = BT->oT_Name;
        if (id >= 0)
        {
            if (id >= int(lst.size()))
                xrDebug::Fatal(DEBUG_INFO, "Not enought textures for shader. Base texture: '%s'.", lst[0].c_str());
            base = lst[id].c_str();
        }
        if (!TextureDescr.GetDetailTexture(base, detail_texture, detail_scaler))
            bDetail = false;
    }
    else
    {
        if (BT->canUseSteepParallax())
        {
            sh_list& lst = L_textures;
            int id = ParseName(BT->oT_Name);
            base = BT->oT_Name;
            if (id >= 0)
            {
                if (id >= int(lst.size()))
                    xrDebug::Fatal(DEBUG_INFO, "Not enought textures for shader. Base texture: '%s'.", lst[0].c_str());
                base = lst[id].c_str();
            }
        }

        bDetail = false;
    }

    bDetail_Diffuse = false;
    bDetail_Bump = false;

#ifndef _EDITOR
#if RENDER == R_R1
    if (RImplementation.o.no_detail_textures)
        bDetail = false;
#endif
#endif

    if (bDetail)
    {
        TextureDescr.GetTextureUsage(base, bDetail_Diffuse, bDetail_Bump);

#ifndef _EDITOR
#if RENDER != R_R1
        if (!(RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_DETAIL_BUMP)))
        {
            bDetail_Diffuse |= bDetail_Bump;
            bDetail_Bump = false;
        }
#endif
#endif
    }

    bUseSteepParallax =
        TextureDescr.UseSteepParallax(base) && BT->canUseSteepParallax();
#ifdef USE_DX11
    TessMethod = 0;
#endif

    BT->Compile(*this);
}

void CBlender_Compile::SetParams(int iPriority, bool bStrictB2F)
{
    SH->flags.iPriority = iPriority;
    SH->flags.bStrictB2F = bStrictB2F;
    if (bStrictB2F)
    {
#ifdef _EDITOR
        if (1 != (SH->flags.iPriority / 2))
        {
            Log("!If StrictB2F true then Priority must div 2.");
            SH->flags.bStrictB2F = FALSE;
        }
#else
        VERIFY(1 == (SH->flags.iPriority / 2));
#endif
    }
}

void CBlender_Compile::PassBegin()
{
    RS.Invalidate();
    passTextures.clear();
    passMatrices.clear();
    passConstants.clear();
    ctable.clear();
    dwStage = 0;

    PassSET_ZB(true, true);
    PassSET_Blend(false, nvrhi::BlendFactor::One, nvrhi::BlendFactor::Zero, false, 0);
    PassSET_LightFog(false, false);
}

void CBlender_Compile::PassEnd()
{
    if (!dest.vs)
    {
        dest.vs = RImplementation.Resources->_CreateVS("null");
    }

    if (!dest.ps)
    {
        dest.ps = RImplementation.Resources->_CreatePS("null");
    }

    SetMapping();
    dest.state = RImplementation.Resources->_CreateState(RS);
    dest.constants = RImplementation.Resources->_CreateConstantTable(ctable);

    dest.T = RImplementation.Resources->_CreateTextureList(passTextures);
    dest.M = RImplementation.Resources->_CreateMatrixList(passMatrices);
    dest.C = RImplementation.Resources->_CreateConstantList(passConstants);

    ref_pass _pass_ = RImplementation.Resources->_CreatePass(dest);
    SH->passes.push_back(_pass_);
}

void CBlender_Compile::PassSET_Shaders(pcstr _vs, pcstr _ps, pcstr _gs , pcstr _hs , pcstr _ds )
{
    dest.ps = RImplementation.Resources->_CreatePS(_ps);
    u32 flags = 0;
    if (dest.ps->constants.dx9compatibility)
        flags |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
    dest.vs = RImplementation.Resources->_CreateVS(_vs, flags);
    dest.gs = RImplementation.Resources->_CreateGS(_gs);
    dest.hs = RImplementation.Resources->_CreateHS(_hs);
    dest.ds = RImplementation.Resources->_CreateDS(_ds);
    dest.cs = RImplementation.Resources->_CreateCS("null");
}

void CBlender_Compile::PassSET_ZB(BOOL bZTest, BOOL bZWrite, BOOL bInvertZTest)
{
    RS.SetDepthFunc(bZTest ? (bInvertZTest ? nvrhi::ComparisonFunc::Greater : nvrhi::ComparisonFunc::LessOrEqual) : nvrhi::ComparisonFunc::Always);
    RS.SetDepthWrite(bZWrite);
    RS.SetDepthEnable(bZWrite || bZTest);
}

void CBlender_Compile::PassSET_ablend_mode(BOOL bABlend, nvrhi::BlendFactor abSRC, nvrhi::BlendFactor abDST)
{
    if (bABlend && abSRC == nvrhi::BlendFactor::One && abDST == nvrhi::BlendFactor::Zero)
        bABlend = FALSE;
    RS.SetBlendEnable(bABlend);
    RS.SetSrcBlend(bABlend ? abSRC : nvrhi::BlendFactor::One);
    RS.SetDestBlend(bABlend ? abDST : nvrhi::BlendFactor::Zero);
    RS.SetSrcBlendAlpha(bABlend ? abSRC : nvrhi::BlendFactor::One);
    RS.SetDestBlendAlpha(bABlend ? abDST : nvrhi::BlendFactor::Zero);
}
void CBlender_Compile::PassSET_ablend_aref(BOOL bATest, u32 aRef)
{
    clamp(aRef, 0u, 255u);
    RS.SetAlphaTest(bATest);
    if (bATest)
        RS.SetAlphaRef(aRef);
}

void CBlender_Compile::PassSET_Blend(BOOL bABlend, nvrhi::BlendFactor abSRC, nvrhi::BlendFactor abDST, BOOL bATest, u32 aRef)
{
    PassSET_ablend_mode(bABlend, abSRC, abDST);
#ifdef DEBUG
    if (strstr(Core.Params, "-noaref"))
    {
        bATest = FALSE;
        aRef = 0;
    }
#endif
    PassSET_ablend_aref(bATest, aRef);
}

void CBlender_Compile::PassSET_LightFog(BOOL, BOOL) {}

void CBlender_Compile::StageBegin()
{
    StageSET_Address(nvrhi::SamplerAddressMode::Wrap);
}
void CBlender_Compile::StageEnd() { dwStage++; }
void CBlender_Compile::StageSET_Address(nvrhi::SamplerAddressMode adr)
{
    RS.SetSamplerAddressU(Stage(), adr);
    RS.SetSamplerAddressV(Stage(), adr);
}
void CBlender_Compile::StageSET_TMC(LPCSTR T, LPCSTR M, LPCSTR C, int UVW_channel)
{
    Stage_Texture(T);
    Stage_Matrix(M, UVW_channel);
    Stage_Constant(C);
}

void CBlender_Compile::StageTemplate_LMAP0()
{
    StageSET_Address(nvrhi::SamplerAddressMode::Clamp);
    StageSET_TMC("$base1", "$null", "$null", 1);
}

void CBlender_Compile::Stage_Texture(LPCSTR name, nvrhi::SamplerAddressMode, SamplerFilter fmin, SamplerFilter fmip, SamplerFilter fmag)
{
    sh_list& lst = L_textures;
    int id = ParseName(name);
    LPCSTR N = name;
    if (id >= 0)
    {
        if (id >= int(lst.size()))
            xrDebug::Fatal(DEBUG_INFO, "Not enought textures for shader. Base texture: '%s'.", lst[0].c_str());
        N = lst[id].c_str();
    }
    passTextures.emplace_back(Stage(), shared_str(N));
    i_Filter(Stage(), fmin, fmip, fmag);
}
void CBlender_Compile::Stage_Matrix(LPCSTR name, int iChannel)
{
    sh_list& lst = L_matrices;
    int id = ParseName(name);
    CMatrix* M = RImplementation.Resources->_CreateMatrix((id >= 0) ? lst[id].c_str() : name);
    passMatrices.push_back(M);

    (void)M;
    (void)iChannel;
}
void CBlender_Compile::Stage_Constant(LPCSTR name)
{
    sh_list& lst = L_constants;
    int id = ParseName(name);
    passConstants.push_back(RImplementation.Resources->_CreateConstant((id >= 0) ? lst[id].c_str() : name));
}

void CBlender_Compile::SetupSampler(u32 stage, pcstr sampler)
{
    VERIFY(stage != InvalidStage);

    SamplerFilter minFilter = SamplerFilter::Linear;
    SamplerFilter mipFilter = SamplerFilter::Linear;
    SamplerFilter magFilter = SamplerFilter::Linear;
    nvrhi::SamplerAddressMode addressMode = nvrhi::SamplerAddressMode::Wrap;

    if (xr_strcmp(sampler, "smp_nofilter") == 0)
    {
        addressMode = nvrhi::SamplerAddressMode::Clamp;
        minFilter   = SamplerFilter::Point;
        mipFilter   = SamplerFilter::Point;
        magFilter   = SamplerFilter::Point;
    }
    else if (xr_strcmp(sampler, "smp_rtlinear") == 0)
    {
        addressMode = nvrhi::SamplerAddressMode::Clamp;
        mipFilter   = SamplerFilter::Point;
    }
    else if ((xr_strcmp(sampler, "s_detail") == 0) || (xr_strcmp(sampler, "s_base") == 0))
    {
        minFilter = SamplerFilter::Anisotropic;
        magFilter = SamplerFilter::Anisotropic;
    }
    else if (xr_strcmp(sampler, "smp_material") == 0)
    {
        addressMode = nvrhi::SamplerAddressMode::Clamp;
        mipFilter   = SamplerFilter::Point;
        RS.SetSamplerAddressW(stage, nvrhi::SamplerAddressMode::Wrap);
    }

    i_Address(stage, addressMode);
    i_Filter(stage, minFilter, mipFilter, magFilter);

    if (stage < 4)
        i_Projective(stage, false);
}

u32 CBlender_Compile::SampledImage(pcstr sampler, pcstr image, shared_str texture)
{
    const auto& findSamplerResource = [&](pcstr name) -> u32
    {
        ref_constant C = ctable.get(name, RC_sampler);
        if (!C)
        {
            return InvalidStage;
        }

        R_ASSERT(C->type == RC_sampler);
        return C->samp.index;
    };

    const auto& findTextureSlot = [&](pcstr name) -> u32
    {
#ifdef USE_DX11
        SPS* ps = dest.ps._get();
        if (ps && ps->reflection)
        {
            const auto& inputTextures = ps->reflection->rtBindings.inputTextures;
            for (const auto& tex : inputTextures)
            {
                if (0 == xr_strcmp(tex.name.c_str(), name))
                {
                    return tex.slot + CTexture::rstPixel;
                }
            }
        }
#else
        ref_constant C = ctable.get(name, RC_dx11texture);
        if (C)
        {
            R_ASSERT(C->type == RC_dx11texture);
            return C->samp.index;
        }
#endif
        return InvalidStage;
    };

    bool useCombinedSamplers = GEnv.Backend->GetCapabilities().useCombinedSamplers;
    auto samplerName = useCombinedSamplers ? image : sampler;
    const u32 samplerStage = findSamplerResource(samplerName);
    if (samplerStage != InvalidStage)
    {
        SetupSampler(samplerStage, sampler);
    }

    const u32 textureStage = useCombinedSamplers ? samplerStage : findTextureSlot(image);
    if (textureStage != InvalidStage && texture.size() != 0)
    {
        string256 name;
        xr_strcpy(name, texture.c_str());

        if (!name[0])
        {
            return samplerStage;
        }

        fix_texture_name(name);

        passTextures.emplace_back(textureStage, shared_str(name));
    }
    else if (textureStage == InvalidStage)
    {
        Msg("! [SampledImage] Texture resource '%s' not found in pixel shader reflection", image);
    }

    return samplerStage;
}
}
