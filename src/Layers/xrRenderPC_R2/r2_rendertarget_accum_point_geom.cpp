#include "stdafx.h"

#include "Layers/xrRender/du_sphere.h"

void CRenderTarget::accum_point_geom_create()
{
    // vertices
    {
        u32 vCount = DU_SPHERE_NUMVERTEX;
        u32 vSize = 3 * 4;
        g_accum_point_vb.Create(vCount * vSize);

        BYTE* pData = static_cast<BYTE*>(g_accum_point_vb.GetHostPointer());
        CopyMemory(pData, du_sphere_vertices, vCount * vSize);
        g_accum_point_vb.Flush();
    }

    // Indices
    {
        u32 iCount = DU_SPHERE_NUMFACES * 3;

        g_accum_point_ib.Create(iCount * 2);
        BYTE* pData = static_cast<BYTE*>(g_accum_point_ib.GetHostPointer());
        CopyMemory(pData, du_sphere_faces, iCount * 2);
        g_accum_point_ib.Flush();
    }
}

void CRenderTarget::accum_point_geom_destroy()
{
#ifdef DEBUG
    _SHOW_REF("g_accum_point_ib", &g_accum_point_ib);
#endif // DEBUG
    g_accum_point_ib.Destroy();
#ifdef DEBUG
    _SHOW_REF("g_accum_point_vb", &g_accum_point_vb);
#endif // DEBUG
    g_accum_point_vb.Destroy();
}
