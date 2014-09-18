#include "stdafx.h"

void CRenderTarget::mark_msaa_edges()
{
   u32	Offset;
	float	d_Z	= EPS_S, d_W = 1.f;
	u32	C		= color_rgba	(255,255,255,255);

   // Fill vertex buffer
	FVF::TL2uv* pv				= (FVF::TL2uv*) RCache.Vertex.Lock	(4,g_combine_2UV->vb_stride,Offset);
	pv->set						(-1,	-1,	0,	d_W, C, 0, 1, 0,		0);	pv++;
	pv->set						(-1,	1,	d_Z,	d_W, C, 0, 0, 0,		0);	pv++;
	pv->set						(1,		-1,	d_Z,	d_W, C, 1, 1, 0,	0);	pv++;
	pv->set						(1,		1,	d_Z,	d_W, C, 1, 0, 0,	0);	pv++;
	RCache.Vertex.Unlock		(4,g_combine_2UV->vb_stride);
	u_setrt	(NULL,NULL,NULL,rt_MSAADepth->pZRT);
	RCache.set_Element		(s_mark_msaa_edges->E[0]);
	RCache.set_Geometry		(g_combine_2UV);	
   StateManager.SetStencil( TRUE, D3DCMP_ALWAYS, 0x80, 0xFF, 0x80, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP  );
   StateManager.SetColorWriteEnable( 0 );
   StateManager.SetDepthFunc( D3DCMP_ALWAYS );
   StateManager.SetDepthEnable( FALSE );
   StateManager.SetCullMode( D3DCULL_NONE );
	RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
   StateManager.SetColorWriteEnable( D3D10_COLOR_WRITE_ENABLE_ALL );
}
