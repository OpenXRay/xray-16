#include "common.h"
#include "iostructs\v_TL.h"

uniform float4		screen_res;		// Screen resolution (x-Width,y-Height, zw - 1/resolution)

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_TL _main ( v_TL I )
{
	v2p_TL O;

	I.P.xy += 0.5;
	O.HPos.x = I.P.x * screen_res.z * 2.0 - 1.0;
	O.HPos.y = I.P.y * screen_res.w * 2.0 - 1.0;
	O.HPos.zw = I.P.zw;

	O.Tex0 = I.Tex0;
	O.Color = float4(I.Color.bgr*4.0, 1.0);	//	swizzle vertex colour

 	return O;
}