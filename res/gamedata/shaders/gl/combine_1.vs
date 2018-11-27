#include "common.h"
#include "iostructs\v_combine.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p _main ( _in I )
{
	v2p 		O;
	O.hpos 		= float4	(I.P.x, -I.P.y, 0.0, 1.0);
	float  	scale 	= texelFetch(s_tonemap,int2(0,0),0).x;
	O.tc0		= float4	(I.P.zw, scale, scale);
	O.tcJ		= I.tcJ;
 	return	O;
}
