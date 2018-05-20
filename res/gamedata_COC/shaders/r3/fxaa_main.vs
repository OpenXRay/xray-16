#include "common.h"

uniform float4		screen_res;		// Screen resolution (x-Width,y-Height, zw - 1/resolution)

struct	v
{
	float3	P		: POSITION;
	float2	Tex0	: TEXCOORD0;
};

struct	v2p 
{
	float2 	Tex0	: TEXCOORD0;
	float4 	HPos	: SV_Position;	 
};

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p  main ( v I )
{
 	v2p O;
    O.HPos 	= float4	(I.P.x * screen_res.z * 2 - 1, (I.P.y * screen_res.w * 2 - 1)* -1, 0, 1);
    O.Tex0	= I.Tex0;
  
    return O; 
}