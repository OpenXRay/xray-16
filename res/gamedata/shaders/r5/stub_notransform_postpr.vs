#include "common_iostructs.h"

uniform float4		screen_res;

v2p_postpr main ( v_postpr I )
{
	v2p_postpr	O;

	{
		I.P.xy += 0.5f;
		O.HPos.x = I.P.x * screen_res.z * 2 - 1;
		O.HPos.y = (I.P.y * screen_res.w * 2 - 1)*-1;
		O.HPos.zw = I.P.zw;
	}


	O.Tex0	= I.Tex0;
	O.Tex1	= I.Tex1;
	O.Tex2	= I.Tex2;
	
	O.Color = I.Color.bgra;
	O.Gray	= I.Gray.bgra;

 	return O;
}