#include "common.h"
#include "skin.h"
#include "iostructs\v_model_shadow_aref.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_shadow_direct_aref _main( v_model	I )
{
	v2p_shadow_direct_aref	O;
	float4 	hpos 	= mul	(m_WVP,	I.P	);
	O.hpos 	= hpos;
	O.tc0 	= I.tc;
#ifndef USE_HWSMAP
	O.depth = O.hpos.z;
#endif
 	return	O;
}
