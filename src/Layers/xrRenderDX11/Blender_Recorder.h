// Blender_Recorder.h: interface for the CBlender_Recorder class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Layers/xrRender/tss.h"

#pragma pack(push, 4)

namespace xray::render::fg
{
class CBlender_Compile
{
public:
    static constexpr auto InvalidStage = std::numeric_limits<u32>::max();

    sh_list L_textures;
    sh_list L_constants;
    sh_list L_matrices;

    LPCSTR detail_texture;
    R_constant_setup* detail_scaler;

    bool bFFP;
    bool bDetail;
    bool bDetail_Diffuse;
    bool bDetail_Bump;
    BOOL bUseSteepParallax;
    int iElement;

public:
    SimulatorStates RS;
    IBlender* BT;
    ShaderElement* SH;
#ifdef USE_DX11
    enum
    {
        NO_TESS = 0,
        TESS_PN = 1,
        TESS_HM = 2,
        TESS_PN_HM = 3
    };
    u32 TessMethod;
#endif

private:
    SPass dest;
    R_constant_table ctable;

    STextureList passTextures;
    SMatrixList passMatrices;
    SConstantList passConstants;
    u32 dwStage;

private:
    inline u32 BC(BOOL v) const { return v ? 1 : 0; }
    void SetupSampler(u32 stage, pcstr sampler);

public:
    u32 SampledImage(pcstr sampler, pcstr image, shared_str texture);

    SimulatorStates& R() { return RS; }
    void SetParams(int iPriority, bool bStrictB2F);
    void SetMapping();

    // R1-compiler
    void PassBegin();
    u32 Pass() { return SH->passes.size(); }
    void PassSET_ZB(BOOL bZTest, BOOL bZWrite, BOOL bInvertZTest = FALSE);
    void PassSET_ablend_mode(BOOL bABlend, nvrhi::BlendFactor abSRC, nvrhi::BlendFactor abDST);
    void PassSET_ablend_aref(BOOL aTest, u32 aRef);
    void PassSET_Blend(BOOL bABlend, nvrhi::BlendFactor abSRC, nvrhi::BlendFactor abDST, BOOL aTest, u32 aRef);
    void PassSET_Blend_BLEND(BOOL bAref = FALSE, u32 ref = 0)
    {
        PassSET_Blend(TRUE, nvrhi::BlendFactor::SrcAlpha, nvrhi::BlendFactor::InvSrcAlpha, bAref, ref);
    }
    void PassSET_Blend_SET(BOOL bAref = FALSE, u32 ref = 0)
    {
        PassSET_Blend(FALSE, nvrhi::BlendFactor::One, nvrhi::BlendFactor::Zero, bAref, ref);
    }
    void PassSET_Blend_ADD(BOOL bAref = FALSE, u32 ref = 0)
    {
        PassSET_Blend(TRUE, nvrhi::BlendFactor::One, nvrhi::BlendFactor::One, bAref, ref);
    }
    void PassSET_Blend_MUL(BOOL bAref = FALSE, u32 ref = 0)
    {
        PassSET_Blend(TRUE, nvrhi::BlendFactor::DstColor, nvrhi::BlendFactor::Zero, bAref, ref);
    }
    void PassSET_Blend_MUL2X(BOOL bAref = FALSE, u32 ref = 0)
    {
        PassSET_Blend(TRUE, nvrhi::BlendFactor::DstColor, nvrhi::BlendFactor::SrcColor, bAref, ref);
    }
    void PassSET_LightFog(BOOL bLight, BOOL bFog);
    void PassSET_Shaders(pcstr _vs, pcstr _ps, pcstr _gs = "null", pcstr _hs = "null", pcstr _ds = "null");
    void PassEnd();

    void StageBegin();
    u32 Stage() { return dwStage; }
    void StageSET_Address(nvrhi::SamplerAddressMode adr);
    void StageSET_TMC(LPCSTR T, LPCSTR M, LPCSTR C, int UVW_channel);
    void Stage_Texture(LPCSTR name, nvrhi::SamplerAddressMode address = nvrhi::SamplerAddressMode::Wrap,
        SamplerFilter fmin = SamplerFilter::Linear, SamplerFilter fmip = SamplerFilter::Linear, SamplerFilter fmag = SamplerFilter::Linear);
    void StageTemplate_LMAP0();
    void Stage_Matrix(LPCSTR name, int UVW_channel);
    void Stage_Constant(LPCSTR name);
    void StageEnd();

    // R1/R2-compiler	[programmable]
    u32 i_Sampler(LPCSTR name) const;
    void i_Texture(u32 s, LPCSTR name);
    void i_Projective(u32 s, bool b);
    void i_Address(u32 s, nvrhi::SamplerAddressMode address);
    void i_Filter_Min(u32 s, SamplerFilter f);
    void i_Filter_Mip(u32 s, SamplerFilter f);
    void i_Filter_Mag(u32 s, SamplerFilter f);
    void i_Filter_Aniso(u32 s, u32 level);
#if defined(USE_DX11)
    void i_dx11FilterAnizo(u32 s, BOOL value);
#endif
    void i_Filter(u32 s, SamplerFilter _min, SamplerFilter _mip, SamplerFilter _mag);
    void i_BorderColor(u32 s, u32 color);

    // R1/R2-compiler	[programmable]		- templates
    void r_Pass(LPCSTR vs, LPCSTR ps, bool bFog, BOOL bZtest = TRUE, BOOL bZwrite = TRUE, BOOL bABlend = FALSE,
        nvrhi::BlendFactor abSRC = nvrhi::BlendFactor::One, nvrhi::BlendFactor abDST = nvrhi::BlendFactor::Zero, BOOL aTest = FALSE, u32 aRef = 0);

    void r_Constant(LPCSTR name, R_constant_setup* s);
    void r_Pass(LPCSTR vs, LPCSTR gs, LPCSTR ps, bool bFog, BOOL bZtest = TRUE, BOOL bZwrite = TRUE,
        BOOL bABlend = FALSE, nvrhi::BlendFactor abSRC = nvrhi::BlendFactor::One, nvrhi::BlendFactor abDST = nvrhi::BlendFactor::Zero, BOOL aTest = FALSE,
        u32 aRef = 0);
#ifdef USE_DX11
    void r_TessPass(LPCSTR vs, LPCSTR hs, LPCSTR ds, LPCSTR gs, LPCSTR ps, bool bFog, BOOL bZtest = TRUE,
        BOOL bZwrite = TRUE, BOOL bABlend = FALSE, nvrhi::BlendFactor abSRC = nvrhi::BlendFactor::One, nvrhi::BlendFactor abDST = nvrhi::BlendFactor::Zero,
        BOOL aTest = FALSE, u32 aRef = 0);
    void r_ComputePass(LPCSTR cs);
#endif
    void r_Stencil(BOOL Enable, nvrhi::ComparisonFunc Func = nvrhi::ComparisonFunc::Always, u32 Mask = 0x00, u32 WriteMask = 0x00,
        nvrhi::StencilOp Fail = nvrhi::StencilOp::Keep, nvrhi::StencilOp Pass = nvrhi::StencilOp::Keep, nvrhi::StencilOp ZFail = nvrhi::StencilOp::Keep);
    void r_StencilRef(u32 Ref);
    void r_CullMode(nvrhi::RasterCullMode Mode);

#if defined(USE_DX11)
    void r_dx11Texture(LPCSTR ResourceName, LPCSTR texture, bool recursive = false);
    void r_dx11Texture(LPCSTR ResourceName, shared_str texture, bool recursive = false)
    {
        return r_dx11Texture(ResourceName, texture.c_str(), recursive);
    };
    u32 r_dx11Sampler(LPCSTR ResourceName);
#endif // USE_DX11

    u32 r_Sampler(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide = false, nvrhi::SamplerAddressMode address = nvrhi::SamplerAddressMode::Wrap,
        SamplerFilter fmin = SamplerFilter::Linear, SamplerFilter fmip = SamplerFilter::Linear, SamplerFilter fmag = SamplerFilter::Linear);
    u32 r_Sampler(LPCSTR name, shared_str texture, bool b_ps1x_ProjectiveDivide = false, nvrhi::SamplerAddressMode address = nvrhi::SamplerAddressMode::Wrap,
        SamplerFilter fmin = SamplerFilter::Linear, SamplerFilter fmip = SamplerFilter::Linear, SamplerFilter fmag = SamplerFilter::Linear)
    {
        return r_Sampler(name, texture.c_str(), b_ps1x_ProjectiveDivide, address, fmin, fmip, fmag);
    }
    void r_Sampler_rtf(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide = false);
    void r_Sampler_clf(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide = false);
    void r_Sampler_clw(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide = false);

#ifdef USE_OGL
    void i_Comparison(u32 s, u32 func);
    void r_Sampler_cmp(pcstr name, pcstr texture, bool b_ps1x_ProjectiveDivide = false);
#endif // USE_OGL

    void r_ColorWriteEnable(bool cR = true, bool cG = true, bool cB = true, bool cA = true);
    void r_End();

    //

    CBlender_Compile();
    ~CBlender_Compile();

    void _cpp_Compile(ShaderElement* _SH);
    ShaderElement* _lua_Compile(LPCSTR namesp, LPCSTR name);
};
#pragma pack(pop)
} // namespace xray::render::fg
