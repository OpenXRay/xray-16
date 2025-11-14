#include "common.h"
#include "skin.h"
/*
struct vf
{
	float2 tc0	: TEXCOORD0;		// base
	float3 c0	: COLOR0;		// color
	float  fog	: FOG;
	float4 hpos	: POSITION;
};
*/

struct v2p
{
	float2 tc0	: TEXCOORD0;		// base
	float3 c0	: COLOR0;		// color
	float  fog	: FOG;
	float4 hpos	: SV_Position;
};

v2p _main(v_model v)
{
	v2p		o;

	float4 	pos 	= v.P;
	float3  pos_w 	= mul( m_W, pos );
	float3 	norm_w 	= normalize( mul( m_W, v.N ) );

	o.hpos 		= mul( m_WVP, pos );		// xform, input in world coords
	o.tc0		= v.tc.xy;					// copy tc
	o.c0 		= calc_model_lq_lighting( norm_w );
	o.fog 		= saturate(calc_fogging( float4( pos_w, 1 ) ));	// fog, input in world coords

	return o;
}

/////////////////////////////////////////////////////////////////////////
#ifdef 	SKIN_NONE
v2p	main(v_model v) 		{ return _main(v); 		}
#endif

#ifdef 	SKIN_0
v2p	main(v_model_skinned_0 v) 	{ return _main(skinning_0(v)); }
#endif

#ifdef	SKIN_1
v2p	main(v_model_skinned_1 v) 	{ return _main(skinning_1(v)); }
#endif

#ifdef	SKIN_2
v2p	main(v_model_skinned_2 v) 	{ return _main(skinning_2(v)); }
#endif

#ifdef	SKIN_3
v2p	main(v_model_skinned_3 v) 	{ return _main(skinning_3(v)); }
#endif

#ifdef	SKIN_4
v2p	main(v_model_skinned_4 v) 	{ return _main(skinning_4(v)); }
#endif