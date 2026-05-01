#include "stdafx.h"
#pragma hdrstop

#include "Blender_Blur.h"

namespace xray::render::fg
{
CBlender_Blur::CBlender_Blur()
{
    description.CLS = B_BLUR;
}

LPCSTR CBlender_Blur::getComment()
{
    return "INTERNAL: blur";
}

void CBlender_Blur::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    C.PassBegin();
    {
        C.PassSET_ZB(FALSE, FALSE);
        C.PassSET_Blend_SET();
        C.PassSET_LightFog(FALSE, FALSE);

        C.StageBegin();
        C.Stage_Texture("$base0");
        C.Stage_Matrix("$null", 0);
        C.Stage_Constant("$null");
        C.StageEnd();

        C.StageBegin();
        C.Stage_Texture("$base1");
        C.Stage_Matrix("$null", 1);
        C.Stage_Constant("$null");
        C.StageEnd();

    }
    C.PassEnd();
}
