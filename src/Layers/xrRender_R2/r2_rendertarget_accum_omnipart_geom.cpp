#include "stdafx.h"

#include "Layers/xrRender/du_sphere_part.h"

void CRenderTarget::accum_omnip_geom_create()
{
    // vertices
    {
        u32 vCount = DU_SPHERE_PART_NUMVERTEX;
        u32 vSize = 3 * 4;

        g_accum_omnip_vb.Create(vCount * vSize);
        u8* pData = static_cast<u8*>(g_accum_omnip_vb.Map());
        CopyMemory(pData, du_sphere_part_vertices, vCount * vSize);
        g_accum_omnip_vb.Unmap(true); // upload vertex data
    }

    // Indices
    {
        u32 iCount = DU_SPHERE_PART_NUMFACES * 3;

        g_accum_omnip_ib.Create(iCount * 2);
        u8* pData = static_cast<u8*>(g_accum_omnip_ib.Map());
        CopyMemory(pData, du_sphere_part_faces, iCount * 2);
        g_accum_omnip_ib.Unmap(true); // upload index data
    }
}

void CRenderTarget::accum_omnip_geom_destroy()
{
#ifdef DEBUG
    _SHOW_REF("g_accum_omnip_ib", &g_accum_omnip_ib);
    _SHOW_REF("g_accum_omnip_vb", &g_accum_omnip_vb);
#endif
    g_accum_omnip_ib.Release();
    g_accum_omnip_vb.Release();
}
