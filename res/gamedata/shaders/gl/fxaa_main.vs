#include "common.h"
#include "iostructs\v_TL.h"

uniform float4		screen_res;		// Screen resolution (x-Width,y-Height, zw - 1/resolution)

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_TL _main (v_TL I)
{
	v2p O;
	O.HPos 	= float4	(I.P.x * screen_res.z * 2 - 1, (I.P.y * screen_res.w * 2 - 1)* -1, 0, 1);
	O.Tex0	= I.Tex0;
  
	return O; 
}
