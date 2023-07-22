#include "stdafx.h"

#include "Blender_light.h"

#if RENDER != R_R1
#error "The blender can't be used in this renderer generation"
#endif

CBlender_LIGHT::CBlender_LIGHT()
{
    description.CLS = B_LIGHT;
}

LPCSTR CBlender_LIGHT::getComment()
{
    return "INTERNAL: lighting effect";
}

BOOL CBlender_LIGHT::canBeLMAPped()
{
    return FALSE;
}

void CBlender_LIGHT::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);
    C.PassBegin();
    {
        C.PassSET_ZB(true, false);
        C.PassSET_Blend(true, D3DBLEND_ONE, D3DBLEND_ONE, true, 0);
        C.PassSET_LightFog(false, false);

        // Stage0 - 2D map
        C.StageBegin();
        C.StageSET_Address(D3DTADDRESS_CLAMP);
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_TFACTOR);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_TFACTOR);
        C.Stage_Texture("$base0");
        C.Stage_Matrix("$null", 0);
        C.Stage_Constant("$null");
        C.StageEnd();

        // Stage1 - 1D map
        C.StageBegin();
        C.StageSET_Address(D3DTADDRESS_CLAMP);
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_CURRENT);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_CURRENT);
        C.Stage_Texture("$base1");
        C.Stage_Matrix("$null", 1);
        C.Stage_Constant("$null");
        C.StageEnd();
    }
    C.PassEnd();
}
