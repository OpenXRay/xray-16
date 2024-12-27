/**
 * @ Version: SCREEN SPACE SHADERS - UPDATE 17
 * @ Description: Rain Shader - VS
 * @ Modified time: 2023-06-22 20:16
 * @ Author: https://www.moddb.com/members/ascii1457
 * @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders
 */

#include "common.h"

// Vertex Struct
struct v2p_Rain
{
	float2 Tex0	 : TEXCOORD0;
	float4 tc	 : TEXCOORD1;
	float4 HPos	 : SV_Position;
};

uniform float4x4 mVPTexgen;

v2p_Rain main ( v_TL I)
{
	v2p_Rain O;

	// Basic Stuff
	O.HPos = mul( m_WVP, I.P);
	O.Tex0 = I.Tex0;

	// Screen Space Data
	O.tc = mul( mVPTexgen, I.P);
	O.tc.z = O.HPos.z;

 	return O;
}