#include "stdafx.h"
#pragma hdrstop

#include "blender_deffer_flat.h"

namespace xray::render::fg
{
CBlender_deffer_flat::CBlender_deffer_flat()
{
    description.CLS = B_DEFAULT;
    description.version = 1;
    oTessellation.Count = 4;
    oTessellation.IDselected = 0;
}

CBlender_deffer_flat::~CBlender_deffer_flat() {}
void CBlender_deffer_flat::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrP_TOKEN::Item I;
    xrPWRITE_PROP(fs, "Tessellation", xrPID_TOKEN, oTessellation);
    I.ID = 0;
    xr_strcpy(I.str, "NO_TESS");
    fs.w(&I, sizeof(I));
    I.ID = 1;
    xr_strcpy(I.str, "TESS_PN");
    fs.w(&I, sizeof(I));
    I.ID = 2;
    xr_strcpy(I.str, "TESS_HM");
    fs.w(&I, sizeof(I));
    I.ID = 3;
    xr_strcpy(I.str, "TESS_PN+HM");
    fs.w(&I, sizeof(I));
}
void CBlender_deffer_flat::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    if (version > 0)
    {
        xrPREAD_PROP(fs, xrPID_TOKEN, oTessellation);
        oTessellation.Count = 4;
    }
}
} // namespace xray::render::fg
