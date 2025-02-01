/**
 * @ Version: SCREEN SPACE SHADERS - UPDATE 21
 * @ Description: HUD Motion Vectors ( RM_SKINNING_1B )
 * @ Modified time: 2024-07-06 04:10
 * @ Author: https://www.moddb.com/members/ascii1457
 * @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders
 */

#include "ssfx_skin_hud.h"

float4x4 m_WVP_prev;

v2p_hud _main( v_model_hud I )
{
	v2p_hud O;

	O.HPos = mul( m_WVP, I.P );

	O.PC = O.HPos; // Current
	O.PP = mul(m_WVP_prev, I.P_prev); // Previous

 	return O;
}

v2p_hud	main(v_model_skinned_1 v) { return _main(skinning_1(v)); }

FXVS;