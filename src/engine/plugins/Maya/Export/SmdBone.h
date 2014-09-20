/***
*
*	Copyright (c) 2001 Escape Factory, Ltd. All rights reserved.
*
****/
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SMDBONE_H__27691F7E_CEE4_422A_9FDE_7972C7C13819__INCLUDED_)
#define AFX_SMDBONE_H__27691F7E_CEE4_422A_9FDE_7972C7C13819__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <maya/MDagPath.h>
#include <stdio.h>

class SmdBone  
{
public:
	Fvector		trans;
	Fvector		orient;

	int			id;
	int			parentId;
	LPSTR		name;
	MDagPath	path;
public:
				SmdBone();
	virtual		~SmdBone();
};

#endif // !defined(AFX_SMDBONE_H__27691F7E_CEE4_422A_9FDE_7972C7C13819__INCLUDED_)
