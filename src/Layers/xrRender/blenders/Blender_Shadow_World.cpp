#include "stdafx.h"
#pragma hdrstop

#include "Blender_Shadow_World.h"

#if RENDER != R_R1
#error "The blender can't be used in this renderer generation"
#endif

CBlender_ShWorld::CBlender_ShWorld()
{
    description.CLS = B_SHADOW_WORLD;
}

LPCSTR CBlender_ShWorld::getComment()
{
    return "INTERNAL: shadow projecting";
}

void CBlender_ShWorld::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    C.PassBegin();
    {
        C.PassSET_ZB(true, false);
        C.PassSET_Blend_MUL();

        // Stage0 - Base texture
        C.StageBegin();
        C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_DIFFUSE);
        C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_DIFFUSE);
        C.Stage_Texture("$base0");
        C.Stage_Matrix("$null", 0);
        C.Stage_Constant("$null");
        C.StageEnd();
    }
    C.PassEnd();
}
