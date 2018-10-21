// Blender_Recorder.cpp: implementation of the CBlender_Compile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/ResourceManager.h"
#include "Blender_Recorder.h"
#include "Blender.h"

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

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Compile::CBlender_Compile() {}
CBlender_Compile::~CBlender_Compile() {}
void CBlender_Compile::_cpp_Compile(ShaderElement* _SH)
{
    SH = _SH;
    RS.Invalidate();

    //	TODO: Check if we need such wired system for
    //	base texture name detection. Perhapse it's done for
    //	optimization?

    // Analyze possibility to detail this shader
    detail_texture = nullptr;
    detail_scaler = nullptr;
    LPCSTR base = nullptr;
    if (bDetail && BT->canBeDetailed())
    {
        //
        sh_list& lst = L_textures;
        int id = ParseName(BT->oT_Name);
        base = BT->oT_Name;
        if (id >= 0)
        {
            if (id >= int(lst.size()))
                xrDebug::Fatal(DEBUG_INFO, "Not enought textures for shader. Base texture: '%s'.", *lst[0]);
            base = *lst[id];
        }
        if (!RImplementation.Resources->m_textures_description.GetDetailTexture(base, detail_texture, detail_scaler))
            bDetail = false;
    }
    else
    {
        ////////////////////
        //	Igor
        //	Need this to correct base to detect steep parallax.
        if (BT->canUseSteepParallax())
        {
            sh_list& lst = L_textures;
            int id = ParseName(BT->oT_Name);
            base = BT->oT_Name;
            if (id >= 0)
            {
                if (id >= int(lst.size()))
                    xrDebug::Fatal(DEBUG_INFO, "Not enought textures for shader. Base texture: '%s'.", *lst[0]);
                base = *lst[id];
            }
        }
        //	Igor
        ////////////////////

        bDetail = false;
    }

    // Validate for R1 or R2
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
        RImplementation.Resources->m_textures_description.GetTextureUsage(base, bDetail_Diffuse, bDetail_Bump);

#ifndef _EDITOR
#if RENDER != R_R1
        //	Detect the alowance of detail bump usage here.
        if (!(RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_DETAIL_BUMP)))
        {
            bDetail_Diffuse |= bDetail_Bump;
            bDetail_Bump = false;
        }
#endif
#endif
    }

    bUseSteepParallax =
        RImplementation.Resources->m_textures_description.UseSteepParallax(base) && BT->canUseSteepParallax();
/*
    if (DEV->m_textures_description.UseSteepParallax(base))
    {
        bool bSteep = BT->canUseSteepParallax();
        DEV->m_textures_description.UseSteepParallax(base);
        bUseSteepParallax = true;
    }
*/
#ifdef USE_DX11
    TessMethod = 0;
#endif

    // Compile
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
    // SH->Flags.bLighting		= FALSE;
}

//
void CBlender_Compile::PassBegin()
{
    RS.Invalidate();
    passTextures.clear();
    passMatrices.clear();
    passConstants.clear();
    xr_strcpy(pass_ps, "null");
    xr_strcpy(pass_vs, "null");
    dwStage = 0;
}

void CBlender_Compile::PassEnd()
{
    // Last Stage - disable
    RS.SetTSS(Stage(), D3DTSS_COLOROP, D3DTOP_DISABLE);
    RS.SetTSS(Stage(), D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    SPass proto;
    // Create pass
    proto.state = RImplementation.Resources->_CreateState(RS.GetContainer());
    proto.ps = RImplementation.Resources->_CreatePS(pass_ps);
    proto.vs = RImplementation.Resources->_CreateVS(pass_vs);
    ctable.merge(&proto.ps->constants);
    ctable.merge(&proto.vs->constants);
#ifndef USE_DX9
    proto.gs = RImplementation.Resources->_CreateGS(pass_gs);
    ctable.merge(&proto.gs->constants);
#ifdef USE_DX11
    proto.hs = RImplementation.Resources->_CreateHS(pass_hs);
    ctable.merge(&proto.hs->constants);
    proto.ds = RImplementation.Resources->_CreateDS(pass_ds);
    ctable.merge(&proto.ds->constants);
    proto.cs = RImplementation.Resources->_CreateCS(pass_cs);
    ctable.merge(&proto.cs->constants);
#endif
#endif //	USE_DX10
    SetMapping();
    proto.constants = RImplementation.Resources->_CreateConstantTable(ctable);
    proto.T = RImplementation.Resources->_CreateTextureList(passTextures);
#ifdef _EDITOR
    proto.M = RImplementation.Resources->_CreateMatrixList(passMatrices);
#endif
    proto.C = RImplementation.Resources->_CreateConstantList(passConstants);

    ref_pass _pass_ = RImplementation.Resources->_CreatePass(proto);
    SH->passes.push_back(_pass_);
}

void CBlender_Compile::PassSET_PS(LPCSTR name)
{
    xr_strcpy(pass_ps, name);
    xr_strlwr(pass_ps);
}

void CBlender_Compile::PassSET_VS(LPCSTR name)
{
    xr_strcpy(pass_vs, name);
    xr_strlwr(pass_vs);
}

void CBlender_Compile::PassSET_ZB(BOOL bZTest, BOOL bZWrite, BOOL bInvertZTest)
{
    if (Pass())
        bZWrite = FALSE;
    RS.SetRS(D3DRS_ZFUNC, bZTest ? (bInvertZTest ? D3DCMP_GREATER : D3DCMP_LESSEQUAL) : D3DCMP_ALWAYS);
    RS.SetRS(D3DRS_ZWRITEENABLE, BC(bZWrite));
    /*
    if (bZWrite || bZTest)				RS.SetRS	(D3DRS_ZENABLE,	D3DZB_TRUE);
    else								RS.SetRS	(D3DRS_ZENABLE,	D3DZB_FALSE);
    */
}

void CBlender_Compile::PassSET_ablend_mode(BOOL bABlend, u32 abSRC, u32 abDST)
{
    if (bABlend && D3DBLEND_ONE == abSRC && D3DBLEND_ZERO == abDST)
        bABlend = FALSE;
    RS.SetRS(D3DRS_ALPHABLENDENABLE, BC(bABlend));
    RS.SetRS(D3DRS_SRCBLEND, bABlend ? abSRC : D3DBLEND_ONE);
    RS.SetRS(D3DRS_DESTBLEND, bABlend ? abDST : D3DBLEND_ZERO);

#if defined(USE_DX10) || defined(USE_DX11)
    //	Since in our engine D3DRS_SEPARATEALPHABLENDENABLE state is
    //	always set to false and in DirectX 10 blend functions for
    //	color and alpha are always independent, assign blend options for
    //	alpha in DX10 identical to color.
    RS.SetRS(D3DRS_SRCBLENDALPHA, bABlend ? abSRC : D3DBLEND_ONE);
    RS.SetRS(D3DRS_DESTBLENDALPHA, bABlend ? abDST : D3DBLEND_ZERO);
#endif //	USE_DX10
}
void CBlender_Compile::PassSET_ablend_aref(BOOL bATest, u32 aRef)
{
    clamp(aRef, 0u, 255u);
    RS.SetRS(D3DRS_ALPHATESTENABLE, BC(bATest));
    if (bATest)
        RS.SetRS(D3DRS_ALPHAREF, u32(aRef));
}

void CBlender_Compile::PassSET_Blend(BOOL bABlend, u32 abSRC, u32 abDST, BOOL bATest, u32 aRef)
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

void CBlender_Compile::PassSET_LightFog(BOOL bLight, BOOL bFog)
{
    RS.SetRS(D3DRS_LIGHTING, BC(bLight));
    RS.SetRS(D3DRS_FOGENABLE, BC(bFog));
    // SH->Flags.bLighting				|= !!bLight;
}

//
void CBlender_Compile::StageBegin()
{
    StageSET_Address(D3DTADDRESS_WRAP); // Wrapping enabled by default
}
void CBlender_Compile::StageEnd() { dwStage++; }
void CBlender_Compile::StageSET_Address(u32 adr)
{
    RS.SetSAMP(Stage(), D3DSAMP_ADDRESSU, adr);
    RS.SetSAMP(Stage(), D3DSAMP_ADDRESSV, adr);
}
void CBlender_Compile::StageSET_XForm(u32 tf, u32 tc)
{
#ifdef _EDITOR
    RS.SetTSS(Stage(), D3DTSS_TEXTURETRANSFORMFLAGS, tf);
    RS.SetTSS(Stage(), D3DTSS_TEXCOORDINDEX, tc);
#else
    UNUSED(tf);
    UNUSED(tc);
#endif
}
void CBlender_Compile::StageSET_Color(u32 a1, u32 op, u32 a2) { RS.SetColor(Stage(), a1, op, a2); }
void CBlender_Compile::StageSET_Color3(u32 a1, u32 op, u32 a2, u32 a3) { RS.SetColor3(Stage(), a1, op, a2, a3); }
void CBlender_Compile::StageSET_Alpha(u32 a1, u32 op, u32 a2) { RS.SetAlpha(Stage(), a1, op, a2); }
void CBlender_Compile::StageSET_TMC(LPCSTR T, LPCSTR M, LPCSTR C, int UVW_channel)
{
    Stage_Texture(T);
    Stage_Matrix(M, UVW_channel);
    Stage_Constant(C);
}

void CBlender_Compile::StageTemplate_LMAP0()
{
    StageSET_Address(D3DTADDRESS_CLAMP);
    StageSET_Color(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
    StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
    StageSET_TMC("$base1", "$null", "$null", 1);
}

void CBlender_Compile::Stage_Texture(LPCSTR name, u32, u32 fmin, u32 fmip, u32 fmag)
{
    sh_list& lst = L_textures;
    int id = ParseName(name);
    LPCSTR N = name;
    if (id >= 0)
    {
        if (id >= int(lst.size()))
            xrDebug::Fatal(DEBUG_INFO, "Not enought textures for shader. Base texture: '%s'.", *lst[0]);
        N = *lst[id];
    }
    passTextures.push_back(std::make_pair(Stage(), ref_texture(RImplementation.Resources->_CreateTexture(N))));
    //	i_Address				(Stage(),address);
    i_Filter(Stage(), fmin, fmip, fmag);
}
void CBlender_Compile::Stage_Matrix(LPCSTR name, int iChannel)
{
    sh_list& lst = L_matrices;
    int id = ParseName(name);
    CMatrix* M = RImplementation.Resources->_CreateMatrix((id >= 0) ? *lst[id] : name);
    passMatrices.push_back(M);

    // Setup transform pipeline
    u32 ID = Stage();
    if (M)
    {
        switch (M->dwMode)
        {
        case CMatrix::modeProgrammable: StageSET_XForm(D3DTTFF_COUNT3, D3DTSS_TCI_CAMERASPACEPOSITION | ID); break;
        case CMatrix::modeTCM: StageSET_XForm(D3DTTFF_COUNT2, D3DTSS_TCI_PASSTHRU | iChannel); break;
        case CMatrix::modeC_refl: StageSET_XForm(D3DTTFF_COUNT3, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR | ID); break;
        case CMatrix::modeS_refl: StageSET_XForm(D3DTTFF_COUNT2, D3DTSS_TCI_CAMERASPACENORMAL | ID); break;
        default: StageSET_XForm(D3DTTFF_DISABLE, D3DTSS_TCI_PASSTHRU | iChannel); break;
        }
    }
    else
    {
        // No XForm at all
        StageSET_XForm(D3DTTFF_DISABLE, D3DTSS_TCI_PASSTHRU | iChannel);
    }
}
void CBlender_Compile::Stage_Constant(LPCSTR name)
{
    sh_list& lst = L_constants;
    int id = ParseName(name);
    passConstants.push_back(RImplementation.Resources->_CreateConstant((id >= 0) ? *lst[id] : name));
}
