#include "common.h"

struct 	a2v
{
	float4 P:	 	POSITION;	// Object-space position
 	float4 tc0:		TEXCOORD0;	// Texture coordinates
};

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v_shadow_direct_aref 	main	( v_static  	I )
{
	v_shadow_direct_aref 		O;
	O.hpos 	= mul				(m_WVP,	I.P		);
	O.tc0 	= unpack_tc_base	(I.tc,I.T.w,I.B.w	);	// copy tc
#ifndef USE_HWSMAP
	O.depth = O.hpos.z;
#endif
 	return	O;
}
FXVS;
