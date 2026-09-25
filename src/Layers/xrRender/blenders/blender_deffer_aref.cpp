#include "stdafx.h"
#pragma hdrstop

#include "blender_deffer_aref.h"

namespace xray::render::fg
{
CBlender_deffer_aref::CBlender_deffer_aref(bool _lmapped) : lmapped(_lmapped)
{
    description.CLS = B_DEFAULT_AREF;
    oAREF.value = 200;
    oAREF.min = 0;
    oAREF.max = 255;
    oBlend.value = FALSE;
    description.version = 1;
}
CBlender_deffer_aref::~CBlender_deffer_aref() {}
void CBlender_deffer_aref::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrPWRITE_PROP(fs, "Alpha ref", xrPID_INTEGER, oAREF);
    xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
}
void CBlender_deffer_aref::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    if (1 == version)
    {
        xrPREAD_PROP(fs, xrPID_INTEGER, oAREF);
        xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
    }
}
} // namespace xray::render::fg
