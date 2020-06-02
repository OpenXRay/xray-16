#include "stdafx.h"
#pragma hdrstop

#include "Blender_Editor_Wire.h"

CBlender_Editor_Wire::CBlender_Editor_Wire()
{
    description.CLS = B_EDITOR_WIRE;
    xr_strcpy(oT_Factor, "$null");
}

LPCSTR CBlender_Editor_Wire::getComment()
{
    return "EDITOR: wire";
}

BOOL CBlender_Editor_Wire::canBeLMAPped()
{
    return FALSE;
}

void CBlender_Editor_Wire::Save(IWriter& fs)
{
    IBlender::Save(fs);

    xrPWRITE_PROP(fs, "TFactor", xrPID_CONSTANT, oT_Factor);
}

void CBlender_Editor_Wire::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);

    xrPREAD_PROP(fs, xrPID_CONSTANT, oT_Factor);
}

void CBlender_Editor_Wire::CompileForEditor(CBlender_Compile& C)
{
    C.PassBegin();
    {
        // Stage0 - Base texture
        C.StageBegin();
        C.StageSET_Color(D3DTA_DIFFUSE, D3DTOP_MODULATE, D3DTA_TFACTOR);
        C.StageSET_Alpha(D3DTA_DIFFUSE, D3DTOP_MODULATE, D3DTA_TFACTOR);
        C.Stage_Texture("$null");
        C.Stage_Matrix("$null", 0);
        C.Stage_Constant("$null");
    }
    C.PassEnd();
}

void CBlender_Editor_Wire::Compile(CBlender_Compile& C)
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
    }
    C.PassEnd();
}
