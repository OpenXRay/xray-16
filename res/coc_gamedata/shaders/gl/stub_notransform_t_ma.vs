#include "common_iostructs.h"
#include "iostructs\v_TL.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_TL _main ( v_TL I )
{
	v2p_TL O;

	I.P.xy += 0.5;
	O.HPos.x = I.P.x/1024.0 * 2.0 - 1.0;	// SkyLoader: is it correct? Maybe need to use screen_res?
	O.HPos.y = I.P.y/768.0 * 2.0 - 1.0;
	O.HPos.zw = I.P.zw;

	O.Tex0 = I.Tex0;
	O.Color = I.Color.aaaa;	//	swizzle vertex colour

 	return O;
}