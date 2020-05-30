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

void CBlender_Editor_Selection::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

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
