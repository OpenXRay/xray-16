/***
*
*	Copyright (c) 2001 Escape Factory, Ltd. All rights reserved.
*
****/
//
//////////////////////////////////////////////////////////////////////

#if !defined(TRIANGLE_H)
#define TRIANGLE_H

#include "SmdBone.h"
#include "..\..\..\editors\Ecore\editor\EditMesh.h"

class SmdVertex
{
public:
	WBVec		influence;
	Fvector		pos;
	Fvector2	uv;
	int			id;
public:
				SmdVertex	();
				SmdVertex	(MPoint pt, float u, float v, const WBVec& wb);
	virtual		~SmdVertex	();

	void		set			(double x, double y, double z, float u, float v);
	void		set			(MPoint pt, float u, float v, int ID);

	IC bool		similar		(double x, double y, double z, float u, float v, const WBVec& wb)const
	{
		if (influence.size()!=wb.size())	return false;
		if (!fsimilar(pos.x,(float)x))		return false;
		if (!fsimilar(pos.y,(float)y))		return false;
		if (!fsimilar(pos.z,(float)z))		return false;
		if (!fsimilar(uv.x,u))				return false;
		if (!fsimilar(uv.y,v))				return false;
		for (u32 k=0; k<influence.size(); k++){
			if (influence[k].bone!=wb[k].bone)					return FALSE;
			if (!fsimilar(influence[k].weight,wb[k].weight))	return FALSE;
		}
		return true;
	}
};

class SmdTriangle
{
public:
	MObject		shader;
	int			v[3];
	int			id;
	u32			sm_group;
public:
				SmdTriangle	();
	virtual		~SmdTriangle();
};

#endif