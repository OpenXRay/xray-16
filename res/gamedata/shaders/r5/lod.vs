#include "common.h"

struct vv
{
	float3 pos0	: POSITION0	;
	float3 pos1	: POSITION1	;
	float3 n0	: NORMAL0	;
	float3 n1	: NORMAL1	;
	float2 tc0	: TEXCOORD0	;
	float2 tc1	: TEXCOORD1	;
	float4 rgbh0	: COLOR1;
	float4 rgbh1	: COLOR2;
	float4 sun_af	: COLOR0;
};
struct vf
{
	float3	Pe	: POSITION	;
 	float2 	tc0	: TEXCOORD1	;
 	float2 	tc1	: TEXCOORD2	;
	float4 	af	: COLOR1	;
	float4 	hpos: SV_Position;
};

#define L_SCALE (2.0f*1.55f)
vf 	main	( vv I )
{
	vf 		o;

	I.sun_af.xyz	= I.sun_af.zyx;
	I.rgbh0.xyz		= I.rgbh0.zyx;
	I.rgbh1.xyz		= I.rgbh1.zyx;

	float 	factor 	= I.sun_af.w	;
	float4 	pos 	= float4	(lerp(I.pos0,I.pos1,factor),1);

	float 	h 	= lerp		(I.rgbh0.w,I.rgbh1.w,factor)		*L_SCALE;

	o.hpos 		= mul		(m_VP, 	pos);
	o.Pe		= mul		(m_V,	pos);

	o.tc0		= I.tc0;						
	o.tc1		= I.tc1;						

	o.af		= float4	(h,h,I.sun_af.z,factor);
	return o	;
}
FXVS;
