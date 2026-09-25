#include "stdafx.h"
#pragma hdrstop

#include "blender_deffer_model.h"

namespace xray::render::fg
{
CBlender_deffer_model::CBlender_deffer_model()
{
    description.CLS = B_MODEL;
    description.version = 2;
    oTessellation.Count = 4;
    oTessellation.IDselected = 0;
    oAREF.value = 32;
    oAREF.min = 0;
    oAREF.max = 255;
    oBlend.value = FALSE;
}
CBlender_deffer_model::~CBlender_deffer_model() {}
void CBlender_deffer_model::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrPWRITE_PROP(fs, "Use alpha-channel", xrPID_BOOL, oBlend);
    xrPWRITE_PROP(fs, "Alpha ref", xrPID_INTEGER, oAREF);
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
void CBlender_deffer_model::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);

    switch (version)
    {
    case 0:
        oAREF.value = 32;
        oAREF.min = 0;
        oAREF.max = 255;
        oBlend.value = FALSE;
        break;
    case 1:
    default:
        xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
        xrPREAD_PROP(fs, xrPID_INTEGER, oAREF);
        break;
    }
    if (version > 1)
    {
        xrPREAD_PROP(fs, xrPID_TOKEN, oTessellation);
    }
}
} // namespace xray::render::fg
