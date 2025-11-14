#include "common.h"

struct 	_in        	
{
	float4	P	: POSITIONT;
	float2	tcJ	: TEXCOORD0;
};

struct 	v2p
{
	float4	tc0	: TEXCOORD0;
	float2	tcJ	: TEXCOORD1;
	float4	hpos: SV_Position;
};
v2p main ( _in I )
{
	v2p 		O;
	O.hpos 		= float4	(I.P.x, -I.P.y, 0, 1);
	float  	scale 	= s_tonemap.Load(int3(0,0,0)).x;
	O.tc0		= float4	(I.P.zw, scale, scale);
	O.tcJ		= I.tcJ;
 	return	O;
}

FXVS;
