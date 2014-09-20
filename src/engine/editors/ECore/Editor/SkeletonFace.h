#ifndef _SkeletonFace_H_
#define _SkeletonFace_H_
#pragma once

#include "SkeletonVert.h"

struct st_SFACE;
void RegisterSFACE(st_SFACE* f);

struct st_SFACE {
	st_SVERT*	v[3];
	DWORD		m;

	st_SFACE(){
		RegisterSFACE(this);
	}
	void	VSet(int id, st_SVERT* V){
		v[id]=V;
	}
	void	SetVerts(st_SVERT* v1, st_SVERT* v2, st_SVERT* v3)
	{
		VSet(0,v1);
		VSet(1,v2);
		VSet(2,v3);
	}
	void	ReplaceVert(st_SVERT* from, st_SVERT* to)
	{
		if (v[0]==from) VSet(0,to);
		if (v[1]==from) VSet(1,to);
		if (v[2]==from) VSet(2,to);
	}
};
DEFINE_VECTOR(st_SFACE*,vSFACE,SFACEIt);
#endif // _SkeletonFace_H_
