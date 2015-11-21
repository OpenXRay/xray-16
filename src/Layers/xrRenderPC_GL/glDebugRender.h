#pragma once

#ifdef DEBUG

#include "../../Include/xrRender/DebugRender.h"

class glDebugRender :
	public IDebugRender
{
public:
	glDebugRender();
	~glDebugRender();

	virtual void	Render() { };
	virtual void	add_lines(Fvector const *vertices, u32 const &vertex_count, u16 const *pairs, u32 const &pair_count, u32 const &color) { };

	// routed to RCache
	virtual void	NextSceneMode() { };
	virtual void	ZEnable(bool bEnable) { };
	virtual void	OnFrameEnd() { };
	virtual void	SetShader(const debug_shader &shader) { };
	virtual void	CacheSetXformWorld(const Fmatrix& M) { };
	virtual void	CacheSetCullMode(CullMode) { };
	virtual void	SetAmbient(u32 colour) { };

	// Shaders
	virtual void	SetDebugShader(dbgShaderHandle shdHandle) { };
	virtual void	DestroyDebugShader(dbgShaderHandle shdHandle) { };

#ifdef DEBUG
	virtual void	dbg_DrawTRI(Fmatrix& T, Fvector& p1, Fvector& p2, Fvector& p3, u32 C) { };
#endif	//	DEBUG
};

extern glDebugRender DebugRenderImpl;
#endif // DEBUG
