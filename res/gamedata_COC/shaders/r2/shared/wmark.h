#ifndef  WMARK_H
#define  WMARK_H
#include "common.h"

#define	NORMAL_SHIFT	0.007f
#define	MIN_SHIFT		0.003f
#define	MAX_SHIFT		0.011f
#define RANGE			100.f

float4 	wmark_shift 	(float3 pos, float3 norm)
{
	float3	P 	= 	pos;
	float3 	N 	= 	norm;
	float3	sd 	= 	eye_position-P;
	float 	d 	= 	length(sd);
	float 	w 	= 	min(d/RANGE,1.f);
	float 	s 	= 	lerp(MIN_SHIFT,MAX_SHIFT,d);
		P	+=	N.xyz*NORMAL_SHIFT;
		P	-=	normalize(eye_direction + normalize(P-eye_position)) * s;
	return	float4	(P,1.f);
}
#endif