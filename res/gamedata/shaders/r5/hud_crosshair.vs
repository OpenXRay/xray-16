#include "common_iostructs.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_TL0uv main ( v_TL0uv_positiont I )
{
	v2p_TL0uv O;

	{
		I.P.xy += 0.5f;
//		O.HPos.x = I.P.x/1024 * 2 - 1;
//		O.HPos.y = (I.P.y/768 * 2 - 1)*-1;
		O.HPos.x = I.P.x * screen_res.z * 2 - 1;
		O.HPos.y = (I.P.y * screen_res.w * 2 - 1)*-1;
		O.HPos.zw = I.P.zw;
	}

	O.Color = I.Color.bgra;	//	swizzle vertex colour

 	return O;
}