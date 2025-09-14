#include "common.h"
#include "iostructs\v_postpr.h"

uniform float4		screen_res;		// Screen resolution (x-Width,y-Height, zw - 1/resolution)

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_postpr _main ( v_postpr I )
{
	v2p_postpr	O;

	I.P.xy += 0.5;
	O.HPos.x = I.P.x * screen_res.z * 2.0 - 1.0;
	O.HPos.y = I.P.y * screen_res.w * 2.0 - 1.0;
	O.HPos.zw = I.P.zw;

	O.Tex0 = I.Tex0;
	O.Tex1 = I.Tex1;
	O.Tex2 = I.Tex2;
	
	O.Color = unpack_D3DCOLOR(I.Color);
	O.Gray = unpack_D3DCOLOR(I.Gray);

 	return O;
}