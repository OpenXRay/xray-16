/**
 * @ Version: SCREEN SPACE SHADERS - UPDATE 21
 * @ Description: HUD Motion Vectors ( RM_SINGLE )
 * @ Modified time: 2024-07-18 07:26
 * @ Author: https://www.moddb.com/members/ascii1457
 * @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders
 */

#include "ssfx_skin_hud.h"

float4x4 m_bone; // Bone matrix
float4x4 m_WVP_prev;

v2p_hud _main( v_model_hud I, float3 psp )
{
	v2p_hud O;

	O.HPos = mul( m_WVP, I.P );
	
	// Apply bone matrix
	float3 Pbone = mul( m_bone, I.P );

	O.PC = O.HPos; // Current
	O.PP = mul(m_WVP_prev, float4(Pbone, 1.0f)); // Previous

	return O;
}

v2p_hud	main(v_model_hud v) { return _main(v, 0); }

FXVS;