#include "common.h"

struct 	a2v
{
// 	float4 tc0:		TEXCOORD0;	// Texture coordinates
	float4 P:	 	POSITION;	// Object-space position
};

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_shadow_direct main ( a2v I )
{
	v2p_shadow_direct	O;

	O.hpos 	= mul		(m_WVP,	I.P	);
#ifndef USE_HWSMAP
	O.depth = O.hpos.z;
#endif
 	return	O;
}
FXVS;
