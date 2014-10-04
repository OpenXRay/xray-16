#include "common.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_TL main ( v_TL I )
{
	v2p_TL O;

//	O.HPos = I.P;
	O.HPos = mul(m_VP, I.P);	// xform, input in world coords
	O.HPos.z = O.HPos.w;
	O.Tex0 = I.Tex0;
	O.Color = I.Color.bgra;		//	swizzle vertex colour

 	return O;
}