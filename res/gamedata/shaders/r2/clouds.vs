#include "common.h"
#include "shared\cloudconfig.h"

struct 	vi
{
	float4	p		: POSITION	;
	float4	dir		: COLOR0	;	// dir0,dir1(w<->z)
	float4	color	: COLOR1	;	// rgb. intensity
};

struct 	vf
{
	float4 	hpos	: POSITION	;
	float4	color	: COLOR0	;	// rgb. intensity, for SM3 - tonemap-prescaled, HI-res
  	float2	tc0		: TEXCOORD0	;
  	float2	tc1		: TEXCOORD1	;
};

vf main (vi v)
{
	vf 		o;

	o.hpos 		= mul		(m_WVP, v.p);	// xform, input in world coords
	
//	if (length(float3(v.p.x,0,v.p.z))>CLOUD_FADE)	o.color.w = 0	;

	// generate tcs
	float2  d0	= v.dir.xy*2-1;
	float2  d1	= v.dir.wz*2-1;
	float2 	_0	= v.p.xz * CLOUD_TILE0 + d0*timers.z*CLOUD_SPEED0;
	float2 	_1	= v.p.xz * CLOUD_TILE1 + d1*timers.z*CLOUD_SPEED1;
	o.tc0		= _0;					// copy tc
	o.tc1		= _1;					// copy tc

	o.color		=	v.color	;			// copy color, low precision, cannot prescale even by 2
	o.color.w	*= 	pow		(v.p.y,25);
#ifdef USE_VTF
	float  	scale 	= 	tex2Dlod (s_tonemap,float4(.5,.5,.5,.5)).x ;
	o.color.rgb 	*= 	scale	;		// high precision
#endif

	return o;
}
