#ifndef  WMARK_H
#define  WMARK_H

#define	NORMAL_SHIFT	0.007
#define	MIN_SHIFT	0.003
#define	MAX_SHIFT	0.011
#define RANGE		100.0

uniform float3		eye_direction;

float4 	wmark_shift 	(float3 pos, float3 norm)
{
	float3	P 	= 	pos;
	float3 	N 	= 	norm;
	float3	sd 	= 	eye_position-P;
	float 	d 	= 	length(sd);
	float 	w 	= 	min(d/RANGE,1.0);
	float 	s 	= 	lerp(MIN_SHIFT,MAX_SHIFT,d);
		P	+=	N.xyz*NORMAL_SHIFT;
		P	-=	normalize(eye_direction + normalize(P-eye_position))*s;
	return	float4	(P, 1.0);
}
#endif