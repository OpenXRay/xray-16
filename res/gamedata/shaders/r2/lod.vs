#include "common.h"

struct vv
{
	float3 pos0	: POSITION0	;
	float3 pos1	: POSITION1	;
	float3 n0	: NORMAL0	;
	float3 n1	: NORMAL1	;
	float2 tc0	: TEXCOORD0	;
	float2 tc1	: TEXCOORD1	;
	float4 rgbh0	: TEXCOORD2;	// rgb.h
	float4 rgbh1	: TEXCOORD3;	// rgb.h
	float4 sun_af	: COLOR0;	// x=sun_0, y=sun_1, z=alpha, w=factor
};
struct vf
{
	float4 	hpos	: POSITION	;
	half3	Pe	: TEXCOORD0	;
 	float2 	tc0	: TEXCOORD1	;	// base0
 	float2 	tc1	: TEXCOORD2	;	// base1
	half4 	af	: COLOR1	;	// alpha&factor
};

#define L_SCALE (2.0h*1.55h)
vf 	main	( vv I )
{
	vf 		o;

	// lerp pos
	float 	factor 	= I.sun_af.w	;
	float4 	pos 	= float4	(lerp(I.pos0,I.pos1,factor),1);

	float 	h 	= lerp		(I.rgbh0.w,I.rgbh1.w,factor)		*L_SCALE;

	o.hpos 		= mul		(m_VP, 	pos);				// xform, input in world coords
	o.Pe		= mul		(m_V,	pos);

	// replicate TCs
	o.tc0		= I.tc0;						
	o.tc1		= I.tc1;						

	// calc normal & lighting
	o.af		= float4	(h,h,I.sun_af.z,factor);
	return o	;
}
FXVS;
