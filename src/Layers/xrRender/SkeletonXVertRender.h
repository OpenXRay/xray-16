#ifndef	SkeletonXVertRender_h_included
#define	SkeletonXVertRender_h_included
#pragma once

#pragma pack(push,2)
struct vertRender// T&B are not skinned, because in R2 skinning occurs always in hardware
{
	Fvector	P;
	Fvector	N;
	float	u,v;
};
#pragma pack(pop)

#endif	//	SkeletonXVertRender_h_included