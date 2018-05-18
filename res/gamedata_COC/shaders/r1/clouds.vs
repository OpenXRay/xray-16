#include "common.h"
#include "shared\cloudconfig.h"

struct vi
{
	float4	p		: POSITION	;
	float4	dir 	: COLOR0	;	// dir0,dir1(w<->z)
	float4	color	: COLOR1	;	// rgb. intensity
};

struct 	vf
{
	float4 	hpos	: POSITION	;
	float4	color	: COLOR0	;	// rgb. intensity
  	float2	tc0		: TEXCOORD0	;
  	float2	tc1		: TEXCOORD1	;
};

vf main (vi v)
{
	vf 		o;

	o.hpos 		= mul		(m_WVP, v.p);	// xform, input in world coords
	o.color		= v.color;			// copy color
	
	o.color.w	*= pow		(v.p.y,25);
	
	//if (length(float3(v.p.x,0,v.p.z))>CLOUD_FADE)	o.color.w = 0	;

	// generate tcs
	float2 d0	= v.dir.xy*2-1;
	float2 d1	= v.dir.wz*2-1;
	o.tc0		= v.p.xz * CLOUD_TILE0 + d0*timers.z*CLOUD_SPEED0;
	o.tc1		= v.p.xz * CLOUD_TILE1 + d1*timers.z*CLOUD_SPEED1;

	return o;
}
