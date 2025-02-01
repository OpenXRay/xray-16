/**
 * @ Version: SCREEN SPACE SHADERS - UPDATE 19
 * @ Description: Trees - Shadows
 * @ Modified time: 2023-12-16 13:42
 * @ Author: https://www.moddb.com/members/ascii1457
 * @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders
 */

#include "common.h"

uniform float3x4		m_xform;
uniform float4 			consts; 	// {1/quant,1/quant,???,???}

#include "screenspace_wind.h"

#ifdef	USE_AREF
v2p_shadow_direct_aref main ( v_shadow_direct_aref I )
#else
v2p_shadow_direct main ( v_shadow_direct I )
#endif
{
#ifdef	USE_AREF
	v2p_shadow_direct_aref 	O;
#else
	v2p_shadow_direct 		O;
#endif
	
	// Transform to world coords
	float3 	pos	= mul(m_xform , I.P);
	float 	H	= pos.y - m_xform._24;	// height of vertex (scaled, rotated, etc.)
	float2	tc = 0;
	float3	wind_result = 0;

#ifdef	USE_TREEWAVE
	wind_result	= 0;
#else

	#ifdef	USE_AREF
		tc = (I.tc * consts).xy;
		wind_result = ssfx_wind_tree_branches(pos, H, tc.y, ssfx_wind_setup());
	#else
		wind_result.xz = ssfx_wind_tree_trunk(pos, H, ssfx_wind_setup()).xy;
	#endif

#endif

	float4 f_pos = float4(pos.xyz + wind_result.xyz, 1);

	O.hpos 	= mul(m_VP,	f_pos	);

#ifdef	USE_AREF
	O.tc0 = tc;
#endif

#ifndef USE_HWSMAP
	O.depth = O.hpos.z;
#endif
	return O;
}
FXVS;