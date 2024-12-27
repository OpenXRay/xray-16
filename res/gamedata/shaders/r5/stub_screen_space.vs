#include "common.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
p_screen main(v2p_screen I)
{
	p_screen O;
	// Transform to screen space (in d3d9 it was done automatically)
	O.hpos.x 	=  (I.HPos.x * screen_res.z * 2 - 1);
	O.hpos.y 	= -(I.HPos.y * screen_res.w * 2 - 1);
	O.hpos.zw 	=	I.HPos.zw;
	
    O.tc0 		= 	I.tc0;
  
    return O; 
}