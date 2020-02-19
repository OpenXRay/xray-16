// Blender_Recorder.h: interface for the CBlender_Recorder class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BLENDER_RECORDER_H__1F549674_8674_4EB2_95E6_E6BC19218A6C__INCLUDED_)
#define AFX_BLENDER_RECORDER_H__1F549674_8674_4EB2_95E6_E6BC19218A6C__INCLUDED_
#pragma once

#include "Layers/xrRender/tss.h"

#pragma pack(push, 4)

class CBlender_Compile
{
public:
    sh_list L_textures;
    sh_list L_constants;
    sh_list L_matrices;

    const char* detail_texture;
    R_constant_setup* detail_scaler;

    bool bEditor;
    bool bDetail;
    bool bDetail_Diffuse;
    bool bDetail_Bump;
    bool bUseSteepParallax;
    int iElement;

public:
    CSimulator RS;
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

    string128 pass_vs;
    string128 pass_ps;
#ifndef USE_DX9
    string128 pass_gs;
#if defined(USE_DX11) || defined(USE_DX12)
    string128 pass_hs;
    string128 pass_ds;
    string128 pass_cs;
#endif
#endif //	USE_DX10

    u32 BC(bool v) { return v ? 0x01 : 0; }

public:
    CSimulator& R() { return RS; }
    void SetParams(int iPriority, bool bStrictB2F);
    void SetMapping();

    // R1-compiler
    void PassBegin();
    u32 Pass() { return SH->passes.size(); }
    void PassSET_ZB(bool bZTest, bool bZWrite, bool bInvertZTest = FALSE);
    void PassSET_ablend_mode(bool bABlend, u32 abSRC, u32 abDST);
    void PassSET_ablend_aref(bool aTest, u32 aRef);
    void PassSET_Blend(bool bABlend, u32 abSRC, u32 abDST, bool aTest, u32 aRef);
    void PassSET_Blend_BLEND(bool bAref = FALSE, u32 ref = 0)
    {
        PassSET_Blend(TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, bAref, ref);
    }
    void PassSET_Blend_SET(bool bAref = FALSE, u32 ref = 0)
    {
        PassSET_Blend(FALSE, D3DBLEND_ONE, D3DBLEND_ZERO, bAref, ref);
    }
    void PassSET_Blend_ADD(bool bAref = FALSE, u32 ref = 0)
    {
        PassSET_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_ONE, bAref, ref);
    }
    void PassSET_Blend_MUL(bool bAref = FALSE, u32 ref = 0)
    {
        PassSET_Blend(TRUE, D3DBLEND_DESTCOLOR, D3DBLEND_ZERO, bAref, ref);
    }
    void PassSET_Blend_MUL2X(bool bAref = FALSE, u32 ref = 0)
    {
        PassSET_Blend(TRUE, D3DBLEND_DESTCOLOR, D3DBLEND_SRCCOLOR, bAref, ref);
    }
    void PassSET_LightFog(bool bLight, bool bFog);
    void PassSET_PS(const char* name);
    void PassSET_VS(const char* name);
    void PassEnd();

    void StageBegin();
    u32 Stage() { return dwStage; }
    void StageSET_Address(u32 adr);
    void StageSET_XForm(u32 tf, u32 tc);
    void StageSET_Color(u32 a1, u32 op, u32 a2);
    void StageSET_Color3(u32 a1, u32 op, u32 a2, u32 a3);
    void StageSET_Alpha(u32 a1, u32 op, u32 a2);
    void StageSET_TMC(const char* T, const char* M, const char* C, int UVW_channel);
    void Stage_Texture(const char* name, u32 address = D3DTADDRESS_WRAP, u32 fmin = D3DTEXF_LINEAR,
        u32 fmip = D3DTEXF_LINEAR, u32 fmag = D3DTEXF_LINEAR);
    void StageTemplate_LMAP0();
    void Stage_Matrix(const char* name, int UVW_channel);
    void Stage_Constant(const char* name);
    void StageEnd();

    // R1/R2-compiler	[programmable]
    u32 i_Sampler(const char* name);
    void i_Texture(u32 s, const char* name);
    void i_Projective(u32 s, bool b);
    void i_Address(u32 s, u32 address);
    void i_Filter_Min(u32 s, u32 f);
    void i_Filter_Mip(u32 s, u32 f);
    void i_Filter_Mag(u32 s, u32 f);
#if defined(USE_DX10) || defined(USE_DX11)
    void i_dx10FilterAnizo(u32 s, bool value);
#endif
    void i_Filter(u32 s, u32 _min, u32 _mip, u32 _mag);
    void i_BorderColor(u32 s, u32 color);

    // R1/R2-compiler	[programmable]		- templates
    void r_Pass(const char* vs, const char* ps, bool bFog, BOOL bZtest = TRUE, BOOL bZwrite = TRUE, BOOL bABlend = FALSE,
        D3DBLEND abSRC = D3DBLEND_ONE, D3DBLEND abDST = D3DBLEND_ZERO, BOOL aTest = FALSE, u32 aRef = 0)
    {
        r_Pass({vs, nullptr}, ps, bFog, bZtest, bZwrite, bABlend, abSRC, abDST, aTest, aRef);
    }

    void r_Pass(std::pair<cpcstr, cpcstr> vs, const char* ps, bool bFog, BOOL bZtest = TRUE, BOOL bZwrite = TRUE,
        BOOL bABlend = FALSE, D3DBLEND abSRC = D3DBLEND_ONE, D3DBLEND abDST = D3DBLEND_ZERO, BOOL aTest = FALSE,
        u32 aRef = 0);

    void r_Constant(const char* name, R_constant_setup* s);
#ifndef USE_DX9
    void r_Pass(const char* vs, const char* gs, const char* ps, bool bFog, BOOL bZtest = TRUE, BOOL bZwrite = TRUE,
        BOOL bABlend = FALSE, D3DBLEND abSRC = D3DBLEND_ONE, D3DBLEND abDST = D3DBLEND_ZERO, BOOL aTest = FALSE,
        u32 aRef = 0);
#ifdef USE_DX11
    void r_TessPass(const char* vs, const char* hs, const char* ds, const char* gs, const char* ps, bool bFog, bool bZtest = TRUE,
        bool bZwrite = TRUE, bool bABlend = FALSE, D3DBLEND abSRC = D3DBLEND_ONE, D3DBLEND abDST = D3DBLEND_ZERO,
        bool aTest = FALSE, u32 aRef = 0);
    void r_ComputePass(const char* cs);
#endif
    void r_Stencil(bool Enable, u32 Func = D3DCMP_ALWAYS, u32 Mask = 0x00, u32 WriteMask = 0x00,
        u32 Fail = D3DSTENCILOP_KEEP, u32 Pass = D3DSTENCILOP_KEEP, u32 ZFail = D3DSTENCILOP_KEEP);
    void r_StencilRef(u32 Ref);
    void r_CullMode(D3DCULL Mode);
#endif

#if defined(USE_DX10) || defined(USE_DX11)
    void r_dx10Texture(const char* ResourceName, const char* texture, bool recursive = false);
    void r_dx10Texture(const char* ResourceName, shared_str texture, bool recursive = false)
    {
        return r_dx10Texture(ResourceName, texture.c_str(), recursive);
    };
    u32 r_dx10Sampler(const char* ResourceName);
#endif //	USE_DX10

    u32 r_Sampler(const char* name, const char* texture, bool b_ps1x_ProjectiveDivide = false, u32 address = D3DTADDRESS_WRAP,
        u32 fmin = D3DTEXF_LINEAR, u32 fmip = D3DTEXF_LINEAR, u32 fmag = D3DTEXF_LINEAR);
    u32 r_Sampler(const char* name, shared_str texture, bool b_ps1x_ProjectiveDivide = false, u32 address = D3DTADDRESS_WRAP,
        u32 fmin = D3DTEXF_LINEAR, u32 fmip = D3DTEXF_LINEAR, u32 fmag = D3DTEXF_LINEAR)
    {
        return r_Sampler(name, texture.c_str(), b_ps1x_ProjectiveDivide, address, fmin, fmip, fmag);
    }
    void r_Sampler_rtf(const char* name, const char* texture, bool b_ps1x_ProjectiveDivide = false);
    void r_Sampler_clf(const char* name, const char* texture, bool b_ps1x_ProjectiveDivide = false);
    void r_Sampler_clw(const char* name, const char* texture, bool b_ps1x_ProjectiveDivide = false);

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
    ShaderElement* _lua_Compile(const char* namesp, const char* name);
};
#pragma pack(pop)

#endif // !defined(AFX_BLENDER_RECORDER_H__1F549674_8674_4EB2_95E6_E6BC19218A6C__INCLUDED_)
