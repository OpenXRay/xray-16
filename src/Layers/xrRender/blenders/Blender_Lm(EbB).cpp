#include "stdafx.h"
#pragma hdrstop

#include "Blender_Lm(EbB).h"

namespace xray::render::fg
{
CBlender_LmEbB::CBlender_LmEbB()
{
    description.CLS = B_LmEbB;
    description.version = 0x1;
    xr_strcpy(oT2_Name, "$null");
    xr_strcpy(oT2_xform, "$null");
    oBlend.value = FALSE;
}

void CBlender_LmEbB::Save(IWriter& fs)
{
    description.version = 0x1;
    IBlender::Save(fs);
    xrPWRITE_MARKER(fs, "Environment map");
    xrPWRITE_PROP(fs, "Name", xrPID_TEXTURE, oT2_Name);
    xrPWRITE_PROP(fs, "Transform", xrPID_MATRIX, oT2_xform);
    xrPWRITE_PROP(fs, "Alpha-Blend", xrPID_BOOL, oBlend);
}

void CBlender_LmEbB::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    xrPREAD_MARKER(fs);
    xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
    xrPREAD_PROP(fs, xrPID_MATRIX, oT2_xform);
    if (version >= 0x1)
    {
        xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
    }
}

LPCSTR CBlender_LmEbB::getComment()
{
    return "LEVEL: lmap*(env^base)";
}

BOOL CBlender_LmEbB::canBeLMAPped()
{
    return TRUE;
}
} // namespace xray::render::fg
