#include "stdafx.h"
#pragma hdrstop

#include "Blender_detail_still.h"

namespace xray::render::fg
{
CBlender_Detail_Still::CBlender_Detail_Still()
{
    description.CLS = B_DETAIL;
    description.version = 0;
}

void CBlender_Detail_Still::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
}

void CBlender_Detail_Still::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
}

LPCSTR CBlender_Detail_Still::getComment()
{
    return "LEVEL: detail objects";
}
} // namespace xray::render::fg
