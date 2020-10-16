#include "stdafx.h"

#include "Layers/xrRender/du_cone.h"

void CRenderTarget::accum_spot_geom_create()
{
    // Vertices
    {
        constexpr size_t vCount = DU_CONE_NUMVERTEX;
        constexpr size_t vSize = 3 * 4;

        g_accum_spot_vb.Create(vCount * vSize);
        u8* pData = static_cast<u8*>(g_accum_spot_vb.Map());
        CopyMemory(pData, du_cone_vertices, vCount * vSize);
        g_accum_spot_vb.Unmap(true); // upload vertex data
    }

    // Indices
    {
        constexpr size_t iCount = DU_CONE_NUMFACES * 3;

        g_accum_spot_ib.Create(iCount * 2);
        u8* pData = static_cast<u8*>(g_accum_spot_ib.Map());
        CopyMemory(pData, du_cone_faces, iCount * 2);
        g_accum_spot_ib.Unmap(true); // upload index data
    }
}

void CRenderTarget::accum_spot_geom_destroy()
{
#ifdef DEBUG
    _SHOW_REF("g_accum_spot_ib", &g_accum_spot_ib);
#endif
    g_accum_spot_ib.Release();

#ifdef DEBUG
    _SHOW_REF("g_accum_spot_vb", &g_accum_spot_vb);
#endif
    g_accum_spot_vb.Release();
}

struct Slice
{
    Fvector m_Vert[4];
};

void CRenderTarget::accum_volumetric_geom_create()
{
    // Vertices
    {
        //	VOLUMETRIC_SLICES quads
        constexpr size_t vCount = VOLUMETRIC_SLICES * 4;
        constexpr size_t vSize = 3 * 4;
        constexpr float dt = 1.0f / static_cast<float>(VOLUMETRIC_SLICES - 1);

        g_accum_volumetric_vb.Create(vCount * vSize);
        Slice* pSlice = static_cast<Slice*>(g_accum_volumetric_vb.Map());
        float t = 0;
        for (int i = 0; i < VOLUMETRIC_SLICES; ++i)
        {
            pSlice[i].m_Vert[0] = Fvector().set(0, 0, t);
            pSlice[i].m_Vert[1] = Fvector().set(0, 1, t);
            pSlice[i].m_Vert[2] = Fvector().set(1, 0, t);
            pSlice[i].m_Vert[3] = Fvector().set(1, 1, t);
            t += dt;
        }
        g_accum_volumetric_vb.Unmap(true); // upload vertex data
    }

    // Indices
    {
        constexpr size_t iCount = VOLUMETRIC_SLICES * 6;

        g_accum_volumetric_ib.Create(iCount * 2);
        u16* pInd = static_cast<u16*>(g_accum_volumetric_ib.Map());
        for (u16 i = 0; i < VOLUMETRIC_SLICES; ++i, pInd += 6)
        {
            const u16 basevert = i * 4;
            pInd[0] = basevert;
            pInd[1] = basevert + 1;
            pInd[2] = basevert + 2;
            pInd[3] = basevert + 2;
            pInd[4] = basevert + 1;
            pInd[5] = basevert + 3;
        }
        g_accum_volumetric_ib.Unmap(true); // upload index data
    }
}

void CRenderTarget::accum_volumetric_geom_destroy()
{
#ifdef DEBUG
    _SHOW_REF("g_accum_volumetric_ib", &g_accum_volumetric_ib);
    _SHOW_REF("g_accum_volumetric_vb", &g_accum_volumetric_vb);
#endif
    g_accum_volumetric_ib.Release();
    g_accum_volumetric_vb.Release();
}
