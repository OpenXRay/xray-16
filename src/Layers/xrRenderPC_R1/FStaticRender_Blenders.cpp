#include "stdafx.h"
#include "Layers/xrRender/blenders/blenderdefault.h"
#include "Layers/xrRender/blenders/blender_default_aref.h"
#include "Layers/xrRender/blenders/blender_vertex.h"
#include "Layers/xrRender/blenders/blender_vertex_aref.h"
#include "Layers/xrRender/blenders/blender_screen_set.h"
#include "Layers/xrRender/blenders/blender_screen_gray.h"
#include "Layers/xrRender/blenders/blender_editor_wire.h"
#include "Layers/xrRender/blenders/blender_editor_selection.h"
#include "Layers/xrRender/blenders/blender_LaEmB.h"
#include "Layers/xrRender/blenders/blender_Lm(EbB).h"
#include "Layers/xrRender/blenders/blender_BmmD.h"
#include "Layers/xrRender/blenders/blender_shadow_world.h"
#include "Layers/xrRender/blenders/blender_blur.h"
#include "Layers/xrRender/blenders/blender_model.h"
#include "Layers/xrRender/blenders/Blender_Model_EbB.h"
#include "Layers/xrRender/blenders/Blender_detail_still.h"
#include "Layers/xrRender/blenders/Blender_tree.h"
#include "Layers/xrRender/blenders/Blender_Particle.h"

IBlender* CRender::blender_create(CLASS_ID cls)
{
    switch (cls)
    {
    case B_DEFAULT: return xr_new<CBlender_default>();
    case B_DEFAULT_AREF: return xr_new<CBlender_default_aref>();
    case B_VERT: return xr_new<CBlender_Vertex>();
    case B_VERT_AREF: return xr_new<CBlender_Vertex_aref>();
    case B_SCREEN_SET: return xr_new<CBlender_Screen_SET>();
    case B_SCREEN_GRAY: return xr_new<CBlender_Screen_GRAY>();
    case B_EDITOR_WIRE: return xr_new<CBlender_Editor_Wire>();
    case B_EDITOR_SEL: return xr_new<CBlender_Editor_Selection>();
    case B_LaEmB: return xr_new<CBlender_LaEmB>();
    case B_LmEbB: return xr_new<CBlender_LmEbB>();
    case B_BmmD: return xr_new<CBlender_BmmD>();
    case B_SHADOW_WORLD: return xr_new<CBlender_ShWorld>();
    case B_BLUR: return xr_new<CBlender_Blur>();
    case B_MODEL: return xr_new<CBlender_Model>();
    case B_MODEL_EbB: return xr_new<CBlender_Model_EbB>();
    case B_DETAIL: return xr_new<CBlender_Detail_Still>();
    case B_TREE: return xr_new<CBlender_Tree>();
    case B_PARTICLE: return xr_new<CBlender_Particle>();
    }
    return nullptr;
}

void CRender::blender_destroy(IBlender*& B) { xr_delete(B); }
