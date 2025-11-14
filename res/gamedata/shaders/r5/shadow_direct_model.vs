#include "common.h"
#include "skin.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_shadow_direct _main( v_model	I )
{
	v2p_shadow_direct	O ;
	float4 	hpos 	= mul( m_WVP, I.P );

	O.hpos 			= hpos;
#ifndef USE_HWSMAP
	O.depth 		= O.hpos.z;
#endif
 	return			O ;
}

/////////////////////////////////////////////////////////////////////////
#ifdef 	SKIN_NONE
v2p_shadow_direct 	main(v_model v) 			{ return _main(v); }
#endif

#ifdef 	SKIN_0
v2p_shadow_direct 	main(v_model_skinned_0 v) 	{ return _main(skinning_0(v)); }
#endif

#ifdef	SKIN_1
v2p_shadow_direct 	main(v_model_skinned_1 v) 	{ return _main(skinning_1(v)); }
#endif

#ifdef	SKIN_2
v2p_shadow_direct 	main(v_model_skinned_2 v) 	{ return _main(skinning_2(v)); }
#endif

#ifdef	SKIN_3
v2p_shadow_direct 	main(v_model_skinned_3 v) 	{ return _main(skinning_3(v)); }
#endif

#ifdef	SKIN_4
v2p_shadow_direct 	main(v_model_skinned_4 v) 	{ return _main(skinning_4(v)); }
#endif

FXVS;
