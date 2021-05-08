#include "stdafx.h"

#include "Layers/xrRender/du_sphere.h"

void CRenderTarget::accum_point_geom_create()
{
    // vertices
    {
        u32 vCount = DU_SPHERE_NUMVERTEX;
        u32 vSize = 3 * 4;

        g_accum_point_vb.Create(vCount * vSize);
        u8* pData = static_cast<u8*>(g_accum_point_vb.Map());
        CopyMemory(pData, du_sphere_vertices, vCount * vSize);
        g_accum_point_vb.Unmap(true); // upload vertex data
    }

    // Indices
    {
        u32 iCount = DU_SPHERE_NUMFACES * 3;

        g_accum_point_ib.Create(iCount * 2);
        u8* pData = static_cast<u8*>(g_accum_point_ib.Map());
        CopyMemory(pData, du_sphere_faces, iCount * 2);
        g_accum_point_ib.Unmap(true); // upload index data
    }
}

void CRenderTarget::accum_point_geom_destroy()
{
#ifdef DEBUG
    _SHOW_REF("g_accum_point_ib", &g_accum_point_ib);
    _SHOW_REF("g_accum_point_vb", &g_accum_point_vb);
#endif
    g_accum_point_ib.Release();
    g_accum_point_vb.Release();
}
