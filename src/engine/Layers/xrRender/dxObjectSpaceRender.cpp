#include "stdafx.h"

#ifdef DEBUG

#include "dxObjectSpaceRender.h"

dxObjectSpaceRender::dxObjectSpaceRender()
{
	m_shDebug.create("debug\\wireframe","$null");
}

dxObjectSpaceRender::~dxObjectSpaceRender()
{
	m_shDebug.destroy();
}

void dxObjectSpaceRender::Copy (IObjectSpaceRender &_in)
{
	*this = *(dxObjectSpaceRender*)&_in;
}

void dxObjectSpaceRender::dbgAddSphere(const Fsphere &sphere, u32 colour)
{
	dbg_S.push_back	(mk_pair(sphere,colour));
}

void dxObjectSpaceRender::dbgRender()
{
	R_ASSERT(bDebug);

	RCache.set_Shader(m_shDebug);
	for (u32 i=0; i<q_debug.boxes.size(); i++)
	{
		Fobb&		obb		= q_debug.boxes[i];
		Fmatrix		X,S,R;
		obb.xform_get(X);
		RCache.dbg_DrawOBB(X,obb.m_halfsize,D3DCOLOR_XRGB(255,0,0));
		S.scale		(obb.m_halfsize);
		R.mul		(X,S);
		RCache.dbg_DrawEllipse(R,D3DCOLOR_XRGB(0,0,255));
	}
	q_debug.boxes.clear();

	for (i=0; i<dbg_S.size(); i++)
	{
		std::pair<Fsphere,u32>& P = dbg_S[i];
		Fsphere&	S = P.first;
		Fmatrix		M;
		M.scale		(S.R,S.R,S.R);
		M.translate_over(S.P);
		RCache.dbg_DrawEllipse(M,P.second);
	}
	dbg_S.clear();
}

void dxObjectSpaceRender::SetShader()
{
	RCache.set_Shader(m_shDebug);
}

#endif // DEBUG