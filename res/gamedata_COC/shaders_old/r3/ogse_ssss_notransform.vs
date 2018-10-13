#include "ogse_config.h"

uniform float4 screen_res;		// Screen resolution (x-Width,y-Height, zw - 1/resolution)

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_ssss main(v_ssss I)
{
 	v2p_ssss O;
	
	{
		I.P.xy += 0.5f;
		O.HPos.x = I.P.x * screen_res.z * 2 - 1;
		O.HPos.y = (I.P.y * screen_res.w * 2 - 1)*-1;
		O.HPos.zw = I.P.zw;
	}
    O.tc0	= I.tc0;
  
    return O; 
}