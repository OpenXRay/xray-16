#include "stdafx.h"

#include "Layers/xrRender/du_cone.h"

void CRenderTarget::accum_spot_geom_create()
{
    // vertices
    {
        u32 vCount = DU_CONE_NUMVERTEX;
        u32 vSize = 3 * 4;

        g_accum_spot_vb.Create(vCount * vSize);
        BYTE* pData = static_cast<BYTE*>(g_accum_spot_vb.GetHostPointer());
        CopyMemory(pData, du_cone_vertices, vCount * vSize);
        g_accum_spot_vb.Flush();
    }

    // Indices
    {
        u32 iCount = DU_CONE_NUMFACES * 3;

        g_accum_spot_ib.Create(iCount * 2);
        BYTE* pData = static_cast<BYTE*>(g_accum_spot_ib.GetHostPointer());
        CopyMemory(pData, du_cone_faces, iCount * 2);
        g_accum_spot_ib.Flush();
    }
}

void CRenderTarget::accum_spot_geom_destroy()
{
#ifdef DEBUG
    _SHOW_REF("g_accum_spot_ib", &g_accum_spot_ib);
#endif // DEBUG
    g_accum_spot_ib.Destroy();
#ifdef DEBUG
    _SHOW_REF("g_accum_spot_vb", &g_accum_spot_vb);
#endif // DEBUG
    g_accum_spot_vb.Destroy();
}

struct Slice
{
    Fvector m_Vert[4];
};

void CRenderTarget::accum_volumetric_geom_create()
{
    // vertices
    {
        //	VOLUMETRIC_SLICES quads
        const u32 vCount = VOLUMETRIC_SLICES * 4;
        u32 vSize = 3 * 4;
        g_accum_volumetric_vb.Create(vCount * vSize);

        BYTE* pData = static_cast<BYTE*>(g_accum_volumetric_vb.GetHostPointer());
        Slice* pSlice = (Slice*)pData;
        float t = 0;
        float dt = 1.0f / (VOLUMETRIC_SLICES - 1);
        for (int i = 0; i < VOLUMETRIC_SLICES; ++i)
        {
            pSlice[i].m_Vert[0] = Fvector().set(0, 0, t);
            pSlice[i].m_Vert[1] = Fvector().set(0, 1, t);
            pSlice[i].m_Vert[2] = Fvector().set(1, 0, t);
            pSlice[i].m_Vert[3] = Fvector().set(1, 1, t);
            t += dt;
        }
        g_accum_volumetric_vb.Flush();
    }

    // Indices
    {
        const u32 iCount = VOLUMETRIC_SLICES * 6;

        g_accum_volumetric_ib.Create(iCount * 2);
        u16* pInd = static_cast<u16*>(g_accum_volumetric_ib.GetHostPointer());
        for (u16 i = 0; i < VOLUMETRIC_SLICES; ++i, pInd += 6)
        {
            u16 basevert = i * 4;
            pInd[0] = basevert;
            pInd[1] = basevert + 1;
            pInd[2] = basevert + 2;
            pInd[3] = basevert + 2;
            pInd[4] = basevert + 1;
            pInd[5] = basevert + 3;
        }
        g_accum_volumetric_ib.Flush();
    }
}

void CRenderTarget::accum_volumetric_geom_destroy()
{
#ifdef DEBUG
    _SHOW_REF("g_accum_volumetric_ib", &g_accum_volumetric_ib);
#endif // DEBUG
    g_accum_volumetric_ib.Destroy();

#ifdef DEBUG
    _SHOW_REF("g_accum_volumetric_vb", &g_accum_volumetric_vb);
#endif // DEBUG
    g_accum_volumetric_vb.Destroy();
}
