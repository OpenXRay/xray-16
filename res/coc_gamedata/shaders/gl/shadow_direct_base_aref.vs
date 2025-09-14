#include "common.h"
#include "iostructs\v_shadow_aref.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_shadow_direct_aref _main ( v_static I )
{
	v2p_shadow_direct_aref 		O;
	O.hpos 	= mul				(m_WVP,	I.P		);
	O.tc0 	= unpack_tc_base	(I.tc,I.T.w,I.B.w	);	// copy tc
#ifndef USE_HWSMAP
	O.depth = O.hpos.z;
#endif
 	return	O;
}
