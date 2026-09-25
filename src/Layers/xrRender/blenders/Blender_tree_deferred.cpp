#include "stdafx.h"
#pragma hdrstop

#include "Blender_tree.h"

namespace xray::render::fg
{
CBlender_Tree::CBlender_Tree()
{
    description.CLS = B_TREE;
    description.version = 1;
    oBlend.value = FALSE;
    oNotAnTree.value = FALSE;
}

void CBlender_Tree::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
    xrPWRITE_PROP(fs, "Object LOD", xrPID_BOOL, oNotAnTree);
}

void CBlender_Tree::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
    if (version >= 1)
    {
        xrPREAD_PROP(fs, xrPID_BOOL, oNotAnTree);
    }
}

LPCSTR CBlender_Tree::getComment()
{
    return "LEVEL: trees/bushes";
}

BOOL CBlender_Tree::canBeDetailed()
{
    return TRUE;
}
} // namespace xray::render::fg
