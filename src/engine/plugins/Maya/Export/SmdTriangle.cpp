/***
*
*	Copyright (c) 2001 Escape Factory, Ltd. All rights reserved.
*
****/
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h" 

#include "SmdTriangle.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


SmdVertex::SmdVertex()
{
	id = -1;
}

SmdVertex::~SmdVertex()
{

}

SmdVertex::SmdVertex(MPoint pt, float u, float v, const WBVec& wb)
{
	pos.set	((float)pt.x,(float)pt.y,(float)pt.z);
	uv.set	(u,v);
	influence = wb;
	id = -1;
}

SmdTriangle::SmdTriangle()
{
	id = -1;
	sm_group = -1;
}

SmdTriangle::~SmdTriangle()
{
}
