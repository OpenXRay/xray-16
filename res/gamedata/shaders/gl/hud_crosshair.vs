#include "common.h"
#include "iostructs\v_TL0uv.h"

uniform float4		screen_res;		// Screen resolution (x-Width,y-Height, zw - 1/resolution)

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_TL0uv _main ( v_TL0uv I )
{
	v2p_TL0uv O;

	I.P.xy += 0.5;
	O.HPos.x = I.P.x * screen_res.z * 2.0 - 1.0;
	O.HPos.y = I.P.y * screen_res.w * 2.0 - 1.0;
	O.HPos.zw = I.P.zw;

	O.Color = unpack_D3DCOLOR(I.Color);

 	return O;
}
