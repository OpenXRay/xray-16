#include "stdafx.h"
#pragma hdrstop

#include "Blender_Editor_Selection.h"

namespace xray::render::fg
{
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
} // namespace xray::render::fg
