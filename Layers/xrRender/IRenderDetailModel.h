#ifndef IRenderDetailModelH
#define IRenderDetailModelH
#pragma once

#include "../../Include/xrRender/RenderDetailModel.h"

//////////////////////////////////////////////////////////////////////////
// definition (Detail Model)
class		IRender_DetailModel
{
public:
	struct fvfVertexIn	{
		Fvector P;
		float	u,v;
	};
	struct fvfVertexOut	{
		Fvector P;
		u32		C;
		float	u,v;
	};
public:
	Fsphere		bv_sphere;
	Fbox		bv_bb;
	flags32		m_Flags;
	float		m_fMinScale;
	float		m_fMaxScale;

	ref_shader	shader;
	fvfVertexIn	*vertices;
	u32			number_vertices;     
	u16			*indices;
	u32			number_indices;
public:
	virtual void					transfer	(Fmatrix& mXform, fvfVertexOut* vDest, u32 C, u16* iDest, u32 iOffset)	= 0;
	virtual void					transfer	(Fmatrix& mXform, fvfVertexOut* vDest, u32 C, u16* iDest, u32 iOffset, float du, float dv)	= 0;
	virtual ~IRender_DetailModel()	{};
};

#endif