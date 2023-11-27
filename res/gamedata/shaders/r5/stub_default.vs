#include "common.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_TL main ( v_TL I)
{
	v2p_TL O;

	O.HPos = mul( m_WVP, I.P);
	O.Tex0 = I.Tex0;
	O.Color = I.Color.bgra;	//	swizzle vertex colour

 	return O;
}