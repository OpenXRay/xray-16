#include "stdafx.h"
#include "Layers/xrRender/du_sphere_part.h"
#include "Layers/xrRender/du_cone.h"
#include "Layers/xrRender/du_sphere.h"

void CRenderTarget::draw_volume(CBackend& cmd_list, light* L)
{
    switch (L->flags.type)
    {
    case IRender_Light::REFLECTED:
    case IRender_Light::POINT:
        cmd_list.set_Geometry(g_accum_point);
        cmd_list.Render(D3DPT_TRIANGLELIST, 0, 0, DU_SPHERE_NUMVERTEX, 0, DU_SPHERE_NUMFACES);
        break;
    case IRender_Light::SPOT:
        cmd_list.set_Geometry(g_accum_spot);
        cmd_list.Render(D3DPT_TRIANGLELIST, 0, 0, DU_CONE_NUMVERTEX, 0, DU_CONE_NUMFACES);
        break;
    case IRender_Light::OMNIPART:
        cmd_list.set_Geometry(g_accum_omnipart);
        cmd_list.Render(D3DPT_TRIANGLELIST, 0, 0, DU_SPHERE_PART_NUMVERTEX, 0, DU_SPHERE_PART_NUMFACES);
        break;
    default:
        break;
    }
}
