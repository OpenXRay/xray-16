// FVisual.cpp: implementation of the FVisual class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable:4995)
#include <d3dx9.h>
#pragma warning(default:4995)

#include "../../xrEngine/fmesh.h"
#include "fvisual.h"

#include "../xrRenderDX10/dx10BufferUtils.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Fvisual::Fvisual()  : dxRender_Visual()
{
	m_fast	=	0;
}

Fvisual::~Fvisual()
{
 	HW.stats_manager.decrement_stats_vb	(p_rm_Vertices);
 	HW.stats_manager.decrement_stats_ib	(p_rm_Indices);
	xr_delete	(m_fast);
}

void Fvisual::Release	()
{
	dxRender_Visual::Release			();
}

void Fvisual::Load		(const char* N, IReader *data, u32 dwFlags)
{
	dxRender_Visual::Load		(N,data,dwFlags);

	D3DVERTEXELEMENT9	dcl		[MAX_FVF_DECL_SIZE];
	D3DVERTEXELEMENT9*	vFormat	= 0;
	dwPrimitives				= 0;
	BOOL				loaded_v=false;

	if (data->find_chunk(OGF_GCONTAINER)) {
#ifndef _EDITOR
		// verts
		u32 ID				= data->r_u32					();
		vBase				= data->r_u32					();
		vCount				= data->r_u32					();

		VERIFY				(NULL==p_rm_Vertices);

		p_rm_Vertices		= RImplementation.getVB			(ID);
		p_rm_Vertices->AddRef	();
		vFormat				= RImplementation.getVB_Format	(ID);
		loaded_v			= true;

		// indices
		ID					= data->r_u32				();
		iBase				= data->r_u32				();
		iCount				= data->r_u32				();
		dwPrimitives		= iCount/3;

		VERIFY				(NULL==p_rm_Indices);
		p_rm_Indices		= RImplementation.getIB		(ID);
		p_rm_Indices->AddRef();
#endif
#if (RENDER==R_R2) || (RENDER==R_R3) || (RENDER==R_R4)
		// check for fast-vertices
		if (data->find_chunk(OGF_FASTPATH))		{
			destructor<IReader>	geomdef	(data->open_chunk		(OGF_FASTPATH));
			destructor<IReader>	def		(geomdef().open_chunk	(OGF_GCONTAINER));

			// we have fast-mesh
			m_fast						= xr_new<IRender_Mesh>	();

			// verts
			D3DVERTEXELEMENT9*	fmt		= 0;
			ID							= def().r_u32			();
			m_fast->vBase				= def().r_u32			();
			m_fast->vCount				= def().r_u32			();

			VERIFY						(NULL==m_fast->p_rm_Vertices);
			m_fast->p_rm_Vertices		= RImplementation.getVB	(ID,true);
			m_fast->p_rm_Vertices->AddRef();
			fmt							= RImplementation.getVB_Format(ID,true);

			// indices
			ID							= def().r_u32			();
			m_fast->iBase				= def().r_u32			();
			m_fast->iCount				= def().r_u32			();
			m_fast->dwPrimitives		= iCount/3;
		
			VERIFY						(NULL==m_fast->p_rm_Indices);
			m_fast->p_rm_Indices		= RImplementation.getIB	(ID,true);
			m_fast->p_rm_Indices->AddRef();

			// geom
			m_fast->rm_geom.create			(fmt,m_fast->p_rm_Vertices,m_fast->p_rm_Indices);
		}
#endif // (RENDER==R_R2) || (RENDER==R_R3) || (RENDER==R_R4)
	}

	// read vertices
	if (!loaded_v && (dwFlags&VLOAD_NOVERTICES)==0) {
		if (data->find_chunk(OGF_VCONTAINER)) {
			R_ASSERT2			(0,"pls notify andy about this.");
#ifndef _EDITOR
			u32 ID				= data->r_u32				();
			vBase				= data->r_u32				();
			vCount				= data->r_u32				();
			VERIFY				(NULL==p_rm_Vertices);
			p_rm_Vertices		= RImplementation.getVB			(ID);
			p_rm_Vertices->AddRef();
			vFormat				= RImplementation.getVB_Format	(ID);
#endif
		} else {
			R_ASSERT			(data->find_chunk(OGF_VERTICES));
			vBase				= 0;
			u32 fvf				= data->r_u32				();
			CHK_DX				(D3DXDeclaratorFromFVF(fvf,dcl));
			vFormat				= dcl;
			vCount				= data->r_u32				();
			u32 vStride			= D3DXGetFVFVertexSize		(fvf);

#if defined(USE_DX10) || defined(USE_DX11)
			VERIFY				(NULL==p_rm_Vertices);
			R_CHK				(dx10BufferUtils::CreateVertexBuffer(&p_rm_Vertices, data->pointer(), vCount*vStride));
			HW.stats_manager.increment_stats_vb						(p_rm_Vertices);
#else	//	USE_DX10
			BOOL	bSoft		= HW.Caps.geometry.bSoftware;
			u32		dwUsage		= D3DUSAGE_WRITEONLY | (bSoft?D3DUSAGE_SOFTWAREPROCESSING:0);
			BYTE*	bytes		= 0;
			VERIFY				(NULL==p_rm_Vertices);
			R_CHK				(HW.pDevice->CreateVertexBuffer	(vCount*vStride,dwUsage,0,D3DPOOL_MANAGED,&p_rm_Vertices,0));
			HW.stats_manager.increment_stats_vb					(p_rm_Vertices);
			R_CHK				(p_rm_Vertices->Lock(0,0,(void**)&bytes,0));
			CopyMemory			(bytes, data->pointer(), vCount*vStride);
			p_rm_Vertices->Unlock	();
#endif	//	USE_DX10
		}
	}

	// indices
	if (!loaded_v) {
		dwPrimitives = 0;
		if (data->find_chunk(OGF_ICONTAINER)) {
			R_ASSERT2			(0,"pls notify andy about this.");
#ifndef _EDITOR
			u32 ID				= data->r_u32			();
			iBase				= data->r_u32			();
			iCount				= data->r_u32			();
			dwPrimitives		= iCount/3;
			VERIFY				(NULL==p_rm_Indices);
			p_rm_Indices		= RImplementation.getIB	(ID);
			p_rm_Indices->AddRef	();
#endif
		} else {
			R_ASSERT			(data->find_chunk(OGF_INDICES));
			iBase				= 0;
			iCount				= data->r_u32();
			dwPrimitives		= iCount/3;

#if defined(USE_DX10) || defined(USE_DX11)
			//BOOL	bSoft		= HW.Caps.geometry.bSoftware || (dwFlags&VLOAD_FORCESOFTWARE);
			//u32		dwUsage		= /*D3DUSAGE_WRITEONLY |*/ (bSoft?D3DUSAGE_SOFTWAREPROCESSING:0);	// indices are read in model-wallmarks code
			//BYTE*	bytes		= 0;

			//VERIFY				(NULL==p_rm_Indices);
			//R_CHK				(HW.pDevice->CreateIndexBuffer(iCount*2,dwUsage,D3DFMT_INDEX16,D3DPOOL_MANAGED,&p_rm_Indices,0));
			//R_CHK				(p_rm_Indices->Lock(0,0,(void**)&bytes,0));
			//CopyMemory		(bytes, data->pointer(), iCount*2);

			VERIFY				(NULL==p_rm_Indices);
			R_CHK				(dx10BufferUtils::CreateIndexBuffer(&p_rm_Indices, data->pointer(), iCount*2));
			HW.stats_manager.increment_stats_ib		( p_rm_Indices);
#else	//	USE_DX10
			BOOL	bSoft		= HW.Caps.geometry.bSoftware;
			u32		dwUsage		= /*D3DUSAGE_WRITEONLY |*/ (bSoft?D3DUSAGE_SOFTWAREPROCESSING:0);	// indices are read in model-wallmarks code
			BYTE*	bytes		= 0;

			VERIFY				(NULL==p_rm_Indices);
			R_CHK				(HW.pDevice->CreateIndexBuffer(iCount*2,dwUsage,D3DFMT_INDEX16,D3DPOOL_MANAGED,&p_rm_Indices,0));
			HW.stats_manager.increment_stats_ib		( p_rm_Indices);
			R_CHK				(p_rm_Indices->Lock(0,0,(void**)&bytes,0));
			CopyMemory		(bytes, data->pointer(), iCount*2);
			p_rm_Indices->Unlock	();
#endif	//	USE_DX10
		}
	}

	if (dwFlags&VLOAD_NOVERTICES)	
		return;
	else	
		rm_geom.create		(vFormat,p_rm_Vertices,p_rm_Indices);
}

void Fvisual::Render		(float )
{
#if (RENDER==R_R2) || (RENDER==R_R3) || (RENDER==R_R4)
	if (m_fast && RImplementation.phase==CRender::PHASE_SMAP && !RCache.is_TessEnabled())
	{
		RCache.set_Geometry		(m_fast->rm_geom);
		RCache.Render			(D3DPT_TRIANGLELIST,m_fast->vBase,0,m_fast->vCount,m_fast->iBase,m_fast->dwPrimitives);
		RCache.stat.r.s_static.add	(m_fast->vCount);
	} else {
		RCache.set_Geometry		(rm_geom);
		RCache.Render			(D3DPT_TRIANGLELIST,vBase,0,vCount,iBase,dwPrimitives);
		RCache.stat.r.s_static.add	(vCount);
	}
#else // (RENDER==R_R2) || (RENDER==R_R3) || (RENDER==R_R4)
	RCache.set_Geometry			(rm_geom);
	RCache.Render				(D3DPT_TRIANGLELIST,vBase,0,vCount,iBase,dwPrimitives);
	RCache.stat.r.s_static.add	(vCount);
#endif // (RENDER==R_R2) || (RENDER==R_R3) || (RENDER==R_R4)
}

#define PCOPY(a)	a = pFrom->a
void	Fvisual::Copy			(dxRender_Visual *pSrc)
{
	dxRender_Visual::Copy		(pSrc);

	Fvisual	*pFrom				= dynamic_cast<Fvisual*> (pSrc);

	PCOPY	(rm_geom);

	PCOPY	(p_rm_Vertices); if (p_rm_Vertices) p_rm_Vertices->AddRef();
	PCOPY	(vBase);
	PCOPY	(vCount);

	PCOPY	(p_rm_Indices); if (p_rm_Indices) p_rm_Indices->AddRef();
	PCOPY	(iBase);
	PCOPY	(iCount);
	PCOPY	(dwPrimitives);

	PCOPY	(m_fast);
}
