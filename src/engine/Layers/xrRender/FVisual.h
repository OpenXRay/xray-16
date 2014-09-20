// FVisual.h: interface for the FVisual class.
//
//////////////////////////////////////////////////////////////////////
#ifndef FVisualH
#define FVisualH
#pragma once

#ifdef _EDITOR
#	include "fbasicvisual.h"
#else
#	include "fbasicvisual.h"
#endif

class	Fvisual					: public		dxRender_Visual, public IRender_Mesh
{
public:
	IRender_Mesh*				m_fast			;	
public:
	virtual void				Render			(float LOD		);		// LOD - Level Of Detail  [0.0f - min, 1.0f - max], Ignored ?
	virtual void				Load			(LPCSTR N, IReader *data, u32 dwFlags);
	virtual void				Copy			(dxRender_Visual *pFrom	);
	virtual void				Release			();

	Fvisual();
	virtual ~Fvisual();
};

#endif 
