#include 	"common.h"

struct 		vv
{
	float4	P		: POSITION;
	float2	tc		: TEXCOORD0;
	float4	c		: COLOR0;
};

struct 		p_particle
{
	p_flat	base	;
	float4 	color	: COLOR0;
};

p_particle 	main	( vv I )
{
	float4 	w_pos 	= I.P;

	// Eye-space pos/normal
	p_flat 		O;
	O.hpos 		= mul		(m_WVP,		w_pos	);
	O.N 		= normalize (eye_position-w_pos	);
	float3	Pe	= mul		(m_WV, 		I.P		);
	O.tcdh 		= float4	(I.tc.xyyy			);
	O.position	= float4	(Pe, 		.2h		);

#ifdef 	USE_TDETAIL
	O.tcdbump	= O.tcdh * dt_params;			// dt tc
#endif

	p_particle	pp;	pp.base=O; pp.color = I.c;
	return		pp;
}
FXVS;
