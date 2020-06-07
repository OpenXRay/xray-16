#include "stdafx.h"
#pragma hdrstop

#include "Blender_Editor_Selection.h"

CBlender_Editor_Selection::CBlender_Editor_Selection()
{
    description.CLS = B_EDITOR_SEL;
    xr_strcpy(oT_Factor, "$null");
}

LPCSTR CBlender_Editor_Selection::getComment()
{
    return "EDITOR: selection";
}

BOOL CBlender_Editor_Selection::canBeLMAPped()
{
    return FALSE;
}

void CBlender_Editor_Selection::Save(IWriter& fs)
{
    IBlender::Save(fs);

    xrPWRITE_PROP(fs, "TFactor", xrPID_CONSTANT, oT_Factor);
}

void CBlender_Editor_Selection::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);

    xrPREAD_PROP(fs, xrPID_CONSTANT, oT_Factor);
}

void CBlender_Editor_Selection::CompileForEditor(CBlender_Compile& C)
{
    C.PassBegin();
    {
        C.PassSET_ZB(true, false);
        C.PassSET_Blend(true, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, false, 0);

        // Stage0 - Base texture
        C.StageBegin();
        C.StageSET_Address(D3DTADDRESS_CLAMP);
        C.StageSET_Color(D3DTA_TFACTOR, D3DTOP_MODULATE, D3DTA_DIFFUSE);
        C.StageSET_Alpha(D3DTA_TFACTOR, D3DTOP_MODULATE, D3DTA_DIFFUSE);
        C.Stage_Texture(oT_Name);
        C.Stage_Matrix(oT_xform, 0);
        C.Stage_Constant("$null");
        C.StageEnd();
    }
    C.PassEnd();
}

void CBlender_Editor_Selection::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (C.bEditor)
    {
        CompileForEditor(C);
        return;
    }

    C.PassBegin();
    {
        C.PassSET_VS("editor");
        C.PassSET_PS("simple_color");

        C.PassSET_LightFog(false, true);
        C.PassSET_ZB(true, false);
        C.PassSET_ablend_mode(true, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
    }
    C.PassEnd();
}
