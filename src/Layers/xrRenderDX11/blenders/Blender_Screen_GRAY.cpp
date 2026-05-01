#include "stdafx.h"
#pragma hdrstop

#include "Blender_Screen_GRAY.h"

#if RENDER != R_R1
#error "The blender can't be used in this renderer generation"
#endif

namespace xray::render::fg
{
CBlender_Screen_GRAY::CBlender_Screen_GRAY()
{
    description.CLS = B_SCREEN_GRAY;
}

LPCSTR CBlender_Screen_GRAY::getComment()
{
    return "INTERNAL: gray-scale effect";
}

void CBlender_Screen_GRAY::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    C.PassBegin();
    {
        C.PassSET_ZB(FALSE, FALSE);
        C.PassSET_Blend(FALSE, nvrhi::BlendFactor::One, nvrhi::BlendFactor::Zero, FALSE, 0);
        C.PassSET_LightFog(FALSE, FALSE);


        C.StageBegin();
        {
            C.StageSET_Address(nvrhi::SamplerAddressMode::Clamp);
            C.Stage_Texture(oT_Name);
            C.Stage_Matrix(oT_xform, 0);
            C.Stage_Constant("$null");
        }
        C.StageEnd();

        C.StageBegin();
        {
            C.StageSET_Address(nvrhi::SamplerAddressMode::Clamp);
            C.Stage_Texture(oT_Name);
            C.Stage_Matrix(oT_xform, 0);
            C.Stage_Constant("$null");
        }
        C.StageEnd();
    }
    C.PassEnd();
}
}
