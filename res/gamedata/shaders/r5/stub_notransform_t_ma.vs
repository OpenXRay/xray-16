#include "common_iostructs.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_TL main ( v_TL_positiont I )
{
	v2p_TL O;

//	O.HPos = P;

	{
		I.P.xy += 0.5f;
		O.HPos.x = I.P.x/1024 * 2 - 1;
		O.HPos.y = (I.P.y/768 * 2 - 1)*-1;
		O.HPos.zw = I.P.zw;
	}

	O.Tex0 = I.Tex0;
	O.Color = I.Color.aaaa;	//	swizzle vertex colour

 	return O;
}