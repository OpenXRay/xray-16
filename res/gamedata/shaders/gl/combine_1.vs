#include "common.h"
#include "iostructs\v_combine.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p _main ( _in I )
{
	v2p 		O;
	O.hpos 		= float4	(I.P.x, -I.P.y, 0.0, 1.0);
	float2 uv 	= abs(I.P.zw - float2(0, 1));
#ifdef USE_VTF
	float  	scale 	= texelFetch	(s_tonemap,int2(0,0),0).x;
	O.tc0		= float4	(uv, scale, scale);
#else // USE_VTF
	O.tc0		= uv;
#endif // USE_VTF
	O.tcJ		= I.tcJ;
 	return	O;
}
