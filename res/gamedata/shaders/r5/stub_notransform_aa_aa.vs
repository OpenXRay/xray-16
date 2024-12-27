#include "common_iostructs.h"

uniform float4		screen_res;		// Screen resolution (x-Width,y-Height, zw - 1/resolution)

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_aa_AA main ( v_aa_AA I )
{
	v2p_aa_AA	O;

//	O.HPos = I.P;

	{
		I.P.xy += 0.5f;
//		O.HPos.x = I.P.x/1024 * 2 - 1;
//		O.HPos.y = (I.P.y/768 * 2 - 1)*-1;
		O.HPos.x = I.P.x * screen_res.z * 2 - 1;
		O.HPos.y = (I.P.y * screen_res.w * 2 - 1)*-1;
		O.HPos.zw = I.P.zw;
	}


	O.Tex0 = I.Tex0;
	O.Tex1 = I.Tex1;
	O.Tex2 = I.Tex2;
	O.Tex3 = I.Tex3;
	O.Tex4 = I.Tex4;
	O.Tex5 = I.Tex5;
	O.Tex6 = I.Tex6;

 	return O;
}