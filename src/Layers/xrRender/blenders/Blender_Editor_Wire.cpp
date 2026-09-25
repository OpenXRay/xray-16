#include "stdafx.h"
#pragma hdrstop

#include "Blender_Editor_Wire.h"

namespace xray::render::fg
{
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
} // namespace xray::render::fg
