#ifndef _SkeletonVert_H_
#define _SkeletonVert_H_
#pragma once

struct st_SVERT;
void RegisterSVERT(st_SVERT* v);

const DWORD BONE_NONE =	0xffffffff;

struct st_SVERT {
	Fvector		P;
	Fvector		O;
	Fvector		N;
	DWORD		sm_group;
    Fvector2Vec uv;
	DWORD		bone;

	st_SVERT(){
		P.set	(0,0,0);
		N.set	(0,1,0);
        uv.clear();
		bone	= BONE_NONE;
		RegisterSVERT(this);
	}
	void	AppendUV(float _u, float _v){
	    uv.push_back(Fvector2());
        uv.back().set(_u,_v);
	}
	void	SetBone(DWORD B){
		if (bone==BONE_NONE){
			bone = B;
		} else R_ASSERT(bone==B);
	}
	BOOL	similar(st_SVERT& V){
		if (bone!=V.bone)			return FALSE;
        if (uv.size()!=V.uv.size())	return FALSE;
        for(DWORD i=0; i<uv.size(); i++){
			if (!fsimilar	(uv[i].x,V.uv[i].x))return FALSE;
			if (!fsimilar	(uv[i].y,V.uv[i].y))return FALSE;
        }
		if (!O.similar	(V.O))		return FALSE;
		if (!N.similar	(V.N))		return FALSE;
		return TRUE;
	}
};
DEFINE_VECTOR(st_SVERT*,vSVERT,SVERTIt);
#endif //_SkeletonVert_H_
