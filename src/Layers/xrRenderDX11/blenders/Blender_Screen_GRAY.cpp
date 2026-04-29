#include "stdafx.h"
#pragma hdrstop

#include "Blender_Screen_GRAY.h"

#if RENDER != R_R1
#error "The blender can't be used in this renderer generation"
#endif

namespace xray::render::RENDER_NAMESPACE
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
        C.PassSET_Blend(FALSE, D3DBLEND_ONE, D3DBLEND_ZERO, FALSE, 0);
        C.PassSET_LightFog(FALSE, FALSE);

        C.R().SetRS(D3DRS_TEXTUREFACTOR, color_rgba(76 + 105, 150 + 105, 29 + 105, 0));

        C.StageBegin();
        {
            C.StageSET_Address(D3DTADDRESS_CLAMP);
            C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_DIFFUSE);
            C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_DIFFUSE);
            C.Stage_Texture(oT_Name);
            C.Stage_Matrix(oT_xform, 0);
            C.Stage_Constant("$null");
        }
        C.StageEnd();

        C.StageBegin();
        {
            C.StageSET_Address(D3DTADDRESS_CLAMP);
            C.StageSET_Color(D3DTA_CURRENT, D3DTOP_DOTPRODUCT3, D3DTA_TFACTOR);
            C.StageSET_Alpha(D3DTA_CURRENT, D3DTOP_DOTPRODUCT3, D3DTA_TFACTOR);
            C.Stage_Texture(oT_Name);
            C.Stage_Matrix(oT_xform, 0);
            C.Stage_Constant("$null");
        }
        C.StageEnd();
    }
    C.PassEnd();
}
}
