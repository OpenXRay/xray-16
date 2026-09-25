#include "stdafx.h"
#pragma hdrstop

#include "Blender_BmmD.h"

namespace xray::render::fg
{
CBlender_BmmD::CBlender_BmmD()
{
    description.CLS = B_BmmD;
    xr_strcpy(oT2_Name, "$null");
    xr_strcpy(oT2_xform, "$null");
    description.version = 3;
    xr_strcpy(oR_Name, "detail" DELIMITER "detail_grnd_grass"); //"$null");
    xr_strcpy(oG_Name, "detail" DELIMITER "detail_grnd_asphalt"); //"$null");
    xr_strcpy(oB_Name, "detail" DELIMITER "detail_grnd_earth"); //"$null");
    xr_strcpy(oA_Name, "detail" DELIMITER "detail_grnd_yantar"); //"$null");
}

void CBlender_BmmD::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrPWRITE_MARKER(fs, "Detail map");
    xrPWRITE_PROP(fs, "Name", xrPID_TEXTURE, oT2_Name);
    xrPWRITE_PROP(fs, "Transform", xrPID_MATRIX, oT2_xform);
    xrPWRITE_PROP(fs, "R2-R", xrPID_TEXTURE, oR_Name);
    xrPWRITE_PROP(fs, "R2-G", xrPID_TEXTURE, oG_Name);
    xrPWRITE_PROP(fs, "R2-B", xrPID_TEXTURE, oB_Name);
    xrPWRITE_PROP(fs, "R2-A", xrPID_TEXTURE, oA_Name);
}

void CBlender_BmmD::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    if (version < 3)
    {
        xrPREAD_MARKER(fs);
        xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
        xrPREAD_PROP(fs, xrPID_MATRIX, oT2_xform);
    }
    else
    {
        xrPREAD_MARKER(fs);
        xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
        xrPREAD_PROP(fs, xrPID_MATRIX, oT2_xform);
        xrPREAD_PROP(fs, xrPID_TEXTURE, oR_Name);
        xrPREAD_PROP(fs, xrPID_TEXTURE, oG_Name);
        xrPREAD_PROP(fs, xrPID_TEXTURE, oB_Name);
        xrPREAD_PROP(fs, xrPID_TEXTURE, oA_Name);
    }
}

LPCSTR CBlender_BmmD::getComment()
{
    return "LEVEL: Implicit**detail";
}

BOOL CBlender_BmmD::canBeDetailed()
{
    return TRUE;
}

BOOL CBlender_BmmD::canBeLMAPped()
{
    return TRUE;
}

BOOL CBlender_BmmD::canUseSteepParallax()
{
    return TRUE;
}
} // namespace xray::render::fg
