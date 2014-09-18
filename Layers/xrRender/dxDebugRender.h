#ifndef	dxDebugRender_included
#define	dxDebugRender_included
#pragma once

#ifdef DEBUG

#include "../../Include/xrRender/DebugRender.h"

class dxDebugRender : public IDebugRender
{
public:
					dxDebugRender		();

	virtual void	Render				();
	virtual void	add_lines			(Fvector const *vertices, u32 const &vertex_count, u16 const *pairs, u32 const &pair_count, u32 const &color);

	// routed to RCache
	virtual void	NextSceneMode		();
	virtual void	ZEnable				(bool bEnable);
	virtual void	OnFrameEnd			();
	virtual void	SetShader			(const debug_shader &shader);
	virtual void	CacheSetXformWorld	(const Fmatrix& M);
	virtual void	CacheSetCullMode	(CullMode);
	virtual void	SetAmbient			(u32 colour);

	// Shaders
	virtual void	SetDebugShader		(dbgShaderHandle shdHandle);
	virtual void	DestroyDebugShader	(dbgShaderHandle shdHandle);

#ifdef DEBUG
	virtual void	dbg_DrawTRI			(Fmatrix& T, Fvector& p1, Fvector& p2, Fvector& p3, u32 C);
#endif	//	DEBUG

private:
			void	try_render			(u32 const &vertex_count, u32 const &index_count);

private:
	enum {
		line_vertex_limit				= 32767,
		line_index_limit				= 32767
	};

private:
	typedef xr_vector<u16>				Indices;
	typedef xr_vector<FVF::L>			Vertices;

protected:
	Vertices		m_line_vertices;
	Indices			m_line_indices;

private:
	ref_shader		m_dbgShaders[dbgShaderCount];
};

extern dxDebugRender DebugRenderImpl;
extern dxDebugRender* rdebug_render;
#endif // DEBUG

#endif	//	dxDebugRender_included