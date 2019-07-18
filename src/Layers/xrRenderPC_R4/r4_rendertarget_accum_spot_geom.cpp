#include "stdafx.h"
#include "Layers/xrRender/du_cone.h"
#include "Layers/xrRenderDX10/dx10BufferUtils.h"

/*
Fvector du_cone_vertices[DU_CONE_NUMVERTEX]=
{
    0.0000f,    0.0000f, 0.0000f,
    0.5000f,   0.0000f,	 1.0000f,
    0.4619f,   0.1913f,	 1.0000f,
    0.3536f,   0.3536f,	 1.0000f,
    0.1913f,   0.4619f,	 1.0000f,
    -0.0000f,  0.5000f,	 1.0000f,
    -0.1913f,  0.4619f,	 1.0000f,
    -0.3536f,  0.3536f,	 1.0000f,
    -0.4619f,  0.1913f,	 1.0000f,
    -0.5000f,  -0.0000f, 1.0000f,
    -0.4619f,  -0.1913f, 1.0000f,
    -0.3536f,  -0.3536f, 1.0000f,
    -0.1913f,  -0.4619f, 1.0000f,
    0.0000f,   -0.5000f, 1.0000f,
    0.1913f,   -0.4619f, 1.0000f,
    0.3536f,   -0.3536f, 1.0000f,
    0.4619f,   -0.1913f, 1.0000f,
    0.0000f,   0.0000f,	 1.0000f+EPS_L
};
u16 du_cone_faces[DU_CONE_NUMFACES*3]=
{
    0,	2,	1,
    0,	3,	2,
    0,	4,	3,
    0,  5,	4,
    0,	6,	5,
    0,  7,  6,
    0,  8,  7,
    0,  9,  8,
    0, 10,  9,
    0, 11, 10,
    0, 12, 11,
    0, 13, 12,
    0, 14, 13,
    0, 15, 14,
    0, 16, 15,
    0,  1, 16,
    17, 1, 2,
    17, 2, 3,
    17, 3, 4,
    17, 4, 5,
    17, 5, 6,
    17, 6, 7,
    17, 7, 8,
    17, 8, 9,
    17, 9, 10,
    17,10, 11,
    17,11, 12,
    17,12, 13,
    17,13, 14,
    17,14, 15,
    17,15, 16,
    17,16, 1
};
*/

void CRenderTarget::accum_spot_geom_create()
{
    //	u32	dwUsage				= D3DUSAGE_WRITEONLY;

    // vertices
    {
        u32 vCount = DU_CONE_NUMVERTEX;
        u32 vSize = 3 * 4;
        // R_CHK	(HW.pDevice->CreateVertexBuffer(
        //	vCount*vSize,
        //	dwUsage,
        //	0,
        //	D3DPOOL_MANAGED,
        //	&g_accum_spot_vb,
        //	0));
        // BYTE*	pData				= 0;
        // R_CHK						(g_accum_spot_vb->Lock(0,0,(void**)&pData,0));
        // CopyMemory				(pData,du_cone_vertices,vCount*vSize);
        // g_accum_spot_vb->Unlock	();

        R_CHK(dx10BufferUtils::CreateVertexBuffer(&g_accum_spot_vb, du_cone_vertices, vCount * vSize));
        HW.stats_manager.increment_stats_vb(g_accum_spot_vb);
    }

    // Indices
    {
        u32 iCount = DU_CONE_NUMFACES * 3;

        // BYTE*	pData		= 0;
        // R_CHK
        // (HW.pDevice->CreateIndexBuffer(iCount*2,dwUsage,D3DFMT_INDEX16,D3DPOOL_MANAGED,&g_accum_spot_ib,0));
        // R_CHK				(g_accum_spot_ib->Lock(0,0,(void**)&pData,0));
        // CopyMemory		(pData,du_cone_faces,iCount*2);
        // g_accum_spot_ib->Unlock	();

        R_CHK(dx10BufferUtils::CreateIndexBuffer(&g_accum_spot_ib, du_cone_faces, iCount * 2));
        HW.stats_manager.increment_stats_ib(g_accum_spot_ib);
    }
}

void CRenderTarget::accum_spot_geom_destroy()
{
#ifdef DEBUG
    _SHOW_REF("g_accum_spot_ib", g_accum_spot_ib);
#endif
    HW.stats_manager.decrement_stats_ib(g_accum_spot_ib);
    _RELEASE(g_accum_spot_ib);

#ifdef DEBUG
    _SHOW_REF("g_accum_spot_vb", g_accum_spot_vb);
#endif
    HW.stats_manager.decrement_stats_vb(g_accum_spot_vb);
    _RELEASE(g_accum_spot_vb);
}

struct Slice
{
    Fvector m_Vert[4];
};

void CRenderTarget::accum_volumetric_geom_create()
{
    // u32	dwUsage				= D3DUSAGE_WRITEONLY;

    // vertices
    {
        //	VOLUMETRIC_SLICES quads
        static const u32 vCount = VOLUMETRIC_SLICES * 4;
        u32 vSize = 3 * 4;
        // R_CHK	(HW.pDevice->CreateVertexBuffer(
        //	vCount*vSize,
        //	dwUsage,
        //	0,
        //	D3DPOOL_MANAGED,
        //	&g_accum_volumetric_vb,
        //	0));
        // BYTE*	pData				= 0;
        // R_CHK						(g_accum_volumetric_vb->Lock(0,0,(void**)&pData,0));
        // Slice	*pSlice = (Slice*)pData;
        // float t=0;
        // float dt = 1.0f/(VOLUMETRIC_SLICES-1);
        // for ( int i=0; i<VOLUMETRIC_SLICES; ++i)
        //{
        // pSlice[i].m_Vert[0] = Fvector().set(0,0,t);
        //	pSlice[i].m_Vert[1] = Fvector().set(0,1,t);
        //	pSlice[i].m_Vert[2] = Fvector().set(1,0,t);
        //	pSlice[i].m_Vert[3] = Fvector().set(1,1,t);
        //	t += dt;
        //}

        Slice pSlice[VOLUMETRIC_SLICES];

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

        R_CHK(dx10BufferUtils::CreateVertexBuffer(&g_accum_volumetric_vb, &pSlice, vCount * vSize));
        HW.stats_manager.increment_stats_vb(g_accum_volumetric_vb);
    }

    // Indices
    {
        const u32 iCount = VOLUMETRIC_SLICES * 6;

        // BYTE*	pData		= 0;
        // R_CHK
        // (HW.pDevice->CreateIndexBuffer(iCount*2,dwUsage,D3DFMT_INDEX16,D3DPOOL_MANAGED,&g_accum_volumetric_ib,0));
        // R_CHK				(g_accum_volumetric_ib->Lock(0,0,(void**)&pData,0));
        // u16 *pInd = (u16*) pData;
        // for ( u16 i=0; i<VOLUMETRIC_SLICES; ++i, pInd+=6)
        //{
        //	u16 basevert = i*4;
        //	pInd[0] = basevert;
        //	pInd[1] = basevert+1;
        //	pInd[2] = basevert+2;
        //	pInd[3] = basevert+2;
        //	pInd[4] = basevert+1;
        //	pInd[5] = basevert+3;
        //}
        // g_accum_volumetric_ib->Unlock	();

        BYTE Datap[iCount * 2];

        u16* pInd = (u16*)Datap;
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

        R_CHK(dx10BufferUtils::CreateIndexBuffer(&g_accum_volumetric_ib, &Datap, iCount * 2));
        HW.stats_manager.increment_stats_ib(g_accum_volumetric_ib);

        //		R_CHK
        //(HW.pDevice->CreateIndexBuffer(iCount*2,dwUsage,D3DFMT_INDEX16,D3DPOOL_MANAGED,&g_accum_volumetric_ib,0));
    }
}

void CRenderTarget::accum_volumetric_geom_destroy()
{
#ifdef DEBUG
    _SHOW_REF("g_accum_volumetric_ib", g_accum_volumetric_ib);
#endif
    HW.stats_manager.decrement_stats_ib(g_accum_volumetric_ib);
    _RELEASE(g_accum_volumetric_ib);

#ifdef DEBUG
    _SHOW_REF("g_accum_volumetric_vb", g_accum_volumetric_vb);
#endif
    HW.stats_manager.decrement_stats_vb(g_accum_volumetric_vb);
    _RELEASE(g_accum_volumetric_vb);
}
