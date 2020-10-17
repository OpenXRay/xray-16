#include "stdafx.h"

#include "Layers/xrRender/blenders/uber_deffer.h"

#include "Layers/xrRender/blenders/Blender_BmmD.h"
#include "Layers/xrRender/blenders/blender_deffer_flat.h"
#include "Layers/xrRender/blenders/blender_deffer_model.h"
#include "Layers/xrRender/blenders/blender_deffer_aref.h"
#include "Layers/xrRender/blenders/Blender_Screen_SET.h"
#include "Layers/xrRender/blenders/Blender_Editor_Wire.h"
#include "Layers/xrRender/blenders/Blender_Editor_Selection.h"
#include "Layers/xrRender/blenders/Blender_tree.h"
#include "Layers/xrRender/blenders/Blender_detail_still.h"
#include "Layers/xrRender/blenders/Blender_Particle.h"
#include "Layers/xrRender/blenders/Blender_Model_EbB.h"
#include "Layers/xrRender/blenders/Blender_Lm(EbB).h"

IBlender* CRender::blender_create(CLASS_ID cls)
{
    switch (cls)
    {
    case B_DEFAULT: return xr_new<CBlender_deffer_flat>();
    case B_DEFAULT_AREF: return xr_new<CBlender_deffer_aref>(true);
    case B_VERT: return xr_new<CBlender_deffer_flat>();
    case B_VERT_AREF: return xr_new<CBlender_deffer_aref>(false);
    case B_SCREEN_SET: return xr_new<CBlender_Screen_SET>();
    case B_SCREEN_GRAY: return nullptr;
    case B_EDITOR_WIRE: return xr_new<CBlender_Editor_Wire>();
    case B_EDITOR_SEL: return xr_new<CBlender_Editor_Selection>();
    case B_LIGHT: return nullptr;
    case B_LmBmmD: return xr_new<CBlender_BmmD>();
    case B_LaEmB: return nullptr;
    case B_LmEbB: return xr_new<CBlender_LmEbB>();
    case B_B: return nullptr;
    case B_BmmD: return xr_new<CBlender_BmmD>();
    case B_SHADOW_TEX: return nullptr;
    case B_SHADOW_WORLD: return nullptr;
    case B_BLUR: return nullptr;
    case B_MODEL: return xr_new<CBlender_deffer_model>();
    case B_MODEL_EbB: return xr_new<CBlender_Model_EbB>();
    case B_DETAIL: return xr_new<CBlender_Detail_Still>();
    case B_TREE: return xr_new<CBlender_Tree>();
    case B_PARTICLE: return xr_new<CBlender_Particle>();
    }
    return nullptr;
}

void CRender::blender_destroy(IBlender*& B)
{
    xr_delete(B);
}
