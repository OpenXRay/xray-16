#include "common.h"

struct 	a2v
{
	float4	P	: POSITION;
 	int2	tc0	: TEXCOORD0;
};

v2p_shadow_direct_aref main ( v_static I )
{
	v2p_shadow_direct_aref 		O;
	O.hpos 	= mul				(m_WVP,	I.P		);
	O.tc0 	= unpack_tc_base	(I.tc,I.T.w,I.B.w	);
#ifndef USE_HWSMAP
	O.depth = O.hpos.z;
#endif
 	return	O;
}
FXVS;
