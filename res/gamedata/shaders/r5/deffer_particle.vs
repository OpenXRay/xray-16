#include 	"common.h"

struct 		vv
{
	float4	P		: POSITION;
	float2	tc		: TEXCOORD0;
	float4	c		: COLOR0;
};

struct 		v2p_particle
{
	float4 		color	: COLOR0;
	v2p_flat	base;
};

v2p_particle main( vv I )
{
	float4 	w_pos 	= I.P;

	// Eye-space pos/normal
	v2p_flat 		O;
	O.hpos 		= mul		(m_WVP,		w_pos	);
	O.N 		= normalize (eye_position-w_pos	);
	float3	Pe	= mul		(m_WV, 		I.P		);
	O.tcdh 		= float4	(I.tc.xyyy			);
	O.position	= float4	(Pe, 		.2h		);

#ifdef 	USE_TDETAIL
	O.tcdbump	= O.tcdh * dt_params;			// dt tc
#endif

	v2p_particle	pp;
	pp.color = I.c;
	pp.base = O;

	return		pp;
}
FXVS;
