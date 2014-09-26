#include "stdafx.h"
#include "../xrRender/du_sphere_part.h"

#include "../xrRenderDX10/dx10BufferUtils.h"

void CRenderTarget::accum_omnip_geom_create		()
{
//	u32	dwUsage				= D3DUSAGE_WRITEONLY;

	// vertices
	{
		u32		vCount		= DU_SPHERE_PART_NUMVERTEX;
		u32		vSize		= 3*4;
//		R_CHK	(HW.pDevice->CreateVertexBuffer(
//			vCount*vSize,
//			dwUsage,
//			0,
//			D3DPOOL_MANAGED,
//			&g_accum_omnip_vb,
//			0));
//		BYTE*	pData				= 0;
//		R_CHK						(g_accum_omnip_vb->Lock(0,0,(void**)&pData,0));
//		CopyMemory				(pData,du_sphere_part_vertices,vCount*vSize);
//		g_accum_omnip_vb->Unlock	();

		R_CHK(dx10BufferUtils::CreateVertexBuffer( &g_accum_omnip_vb, du_sphere_part_vertices, vCount*vSize ));
	}

	// Indices
	{
		u32		iCount		= DU_SPHERE_PART_NUMFACES*3;

//		BYTE*	pData		= 0;
//		R_CHK				(HW.pDevice->CreateIndexBuffer(iCount*2,dwUsage,D3DFMT_INDEX16,D3DPOOL_MANAGED,&g_accum_omnip_ib,0));
//		R_CHK				(g_accum_omnip_ib->Lock(0,0,(void**)&pData,0));
//		CopyMemory		(pData,du_sphere_part_faces,iCount*2);
//		g_accum_omnip_ib->Unlock	();

		R_CHK( dx10BufferUtils::CreateIndexBuffer( &g_accum_omnip_ib, du_sphere_part_faces, iCount*2 ));
	}
}

void CRenderTarget::accum_omnip_geom_destroy()
{
#ifdef DEBUG
	_SHOW_REF("g_accum_omnip_ib",g_accum_omnip_ib);
#endif // DEBUG
	_RELEASE(g_accum_omnip_ib);
#ifdef DEBUG
	_SHOW_REF("g_accum_omnip_vb",g_accum_omnip_vb);
#endif // DEBUG
	_RELEASE(g_accum_omnip_vb);
}
