// FProgressive.cpp: implementation of the FProgressive class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "../../xrEngine/fmesh.h"
#include "FProgressive.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

FProgressive::FProgressive	() : Fvisual()
{
	xSWI			= 0;
	last_lod		= 0;
}

FProgressive::~FProgressive	()
{

}

void FProgressive::Release	()
{
	Fvisual::Release();
	xr_free			(nSWI.sw);
	if (xSWI)		{
		xr_free			(xSWI->sw);
		xr_delete		(xSWI);
	}
}

void FProgressive::Load		(const char* N, IReader *data, u32 dwFlags)
{
	Fvisual::Load	(N,data,dwFlags);

	// normal SWI
	destructor<IReader> lods (data->open_chunk	(OGF_SWIDATA));
    nSWI.reserved[0]	= lods().r_u32();	// reserved 16 bytes
    nSWI.reserved[1]	= lods().r_u32();
    nSWI.reserved[2]	= lods().r_u32();
    nSWI.reserved[3]	= lods().r_u32();
    nSWI.count			= lods().r_u32();
	VERIFY				(NULL==nSWI.sw);
    nSWI.sw				= xr_alloc<FSlideWindow>(nSWI.count);
	lods().r			(nSWI.sw,nSWI.count*sizeof(FSlideWindow));

	// fast
#if RENDER!=R_R1
	if (m_fast)			{
		destructor<IReader>	geomdef	(data->open_chunk		(OGF_FASTPATH));
		destructor<IReader>	def		(geomdef().open_chunk	(OGF_SWIDATA));

		xSWI				= xr_new<FSlideWindowItem>();
		xSWI->reserved[0]	= def().r_u32();	// reserved 16 bytes
		xSWI->reserved[1]	= def().r_u32();
		xSWI->reserved[2]	= def().r_u32();
		xSWI->reserved[3]	= def().r_u32();
		xSWI->count			= def().r_u32();
		VERIFY				(NULL==xSWI->sw);
		xSWI->sw			= xr_alloc<FSlideWindow>(xSWI->count);
		def().r				(xSWI->sw,xSWI->count*sizeof(FSlideWindow));
	}
#endif
}

void FProgressive::Render	(float LOD)
{
#if RENDER!=R_R1
	if (m_fast && RImplementation.phase==CRender::PHASE_SMAP)
	{
		int lod_id			= iFloor((1.f-clampr(LOD,0.f,1.f))*float(xSWI->count-1)+0.5f);
		VERIFY				(lod_id>=0 && lod_id<int(xSWI->count));
		FSlideWindow& SW	= xSWI->sw[lod_id];
		RCache.set_Geometry	(m_fast->rm_geom);
		RCache.Render		(D3DPT_TRIANGLELIST,m_fast->vBase,0,SW.num_verts,m_fast->iBase+SW.offset,SW.num_tris);
		RCache.stat.r.s_static.add	(SW.num_verts);
	} else {
		int lod_id		= last_lod;
		if (LOD>=0.f){
			clamp			(LOD,0.f,1.f);
			lod_id			= iFloor((1.f-LOD)*float(nSWI.count-1)+0.5f);
			last_lod		= lod_id;
		}
		VERIFY				(lod_id>=0 && lod_id<int(nSWI.count));
		FSlideWindow& SW	= nSWI.sw[lod_id];
		RCache.set_Geometry	(rm_geom);
		RCache.Render		(D3DPT_TRIANGLELIST,vBase,0,SW.num_verts,iBase+SW.offset,SW.num_tris);
		RCache.stat.r.s_static.add	(SW.num_verts);
	}
#else
	int lod_id		= last_lod;
	if (LOD>=0.f){
		clamp		(LOD,0.f,1.f);
		lod_id		= iFloor((1.f-LOD)*float(nSWI.count-1)+0.5f);
		last_lod	= lod_id;
	}
	VERIFY						(lod_id>=0 && lod_id<int(nSWI.count));
	FSlideWindow& SW			= nSWI.sw[lod_id];
	RCache.set_Geometry			(rm_geom);
	RCache.Render				(D3DPT_TRIANGLELIST,vBase,0,SW.num_verts,iBase+SW.offset,SW.num_tris);
	RCache.stat.r.s_static.add	(SW.num_verts);
#endif
}

#define PCOPY(a)	a = pFrom->a
void	FProgressive::Copy	(dxRender_Visual *pSrc)
{
	Fvisual::Copy	(pSrc);
	FProgressive	*pFrom = (FProgressive *)pSrc;
	PCOPY			(nSWI);
	PCOPY			(xSWI);
}
