/**
 * @ Version: SCREEN SPACE SHADERS - UPDATE 20
 * @ Description: LUT shader
 * @ Modified time: 2024-01-22 04:12
 * @ Author: https://www.moddb.com/members/ascii1457
 * @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders
 */

// Settings
#include "settings_screenspace_LUT.h"

// Internal --
#define LUT_GROUPS		max(1, G_CELLS_GROUPS)
#define TEXEL_SIZE		float2(1.0f / G_LUT_SIZE_W, 1.0f / (G_CELLS_SIZE * LUT_GROUPS))
#define TEXEL_HALF		float2(TEXEL_SIZE.xy / 2.0f)
#define TEXEL_FIX		TEXEL_SIZE.y * LUT_GROUPS

uniform float4 ssfx_lut;

float3 ssfx_lut_pp(float3 base_col)
{
	// Prepare LUT UVs
	float3 cells = base_col * G_CELLS_SIZE - base_col;
	float lut_frac = frac(cells.b);
	cells.rg = TEXEL_HALF + cells.rg * TEXEL_SIZE;
	cells.r += (cells.b - lut_frac) * TEXEL_FIX;

	// Final LUT UVs
	float4 uvs = float4(cells.rg, cells.r + TEXEL_FIX, cells.g);

	// Group offset
	float2 grp_offset = float2(0.0, ssfx_lut.y / LUT_GROUPS);

	// Sample LUTs
	float3 lut_col = lerp(	s_lut_atlas.Sample(smp_linear, uvs.xy + grp_offset).rgb, 
							s_lut_atlas.Sample(smp_linear, uvs.zw + grp_offset).rgb,
							lut_frac);

#ifdef G_ADVANCE_TRANSITION
	
	// Group2 offset
	grp_offset = float2(0.0, ssfx_lut.z / LUT_GROUPS);

	// Sample transition LUTs
	float3 SecondLUT = lerp(s_lut_atlas.Sample(smp_linear, uvs.xy + grp_offset).rgb,
							s_lut_atlas.Sample(smp_linear, uvs.zw + grp_offset).rgb,
							lut_frac);

	lut_col = lerp(lut_col, SecondLUT, ssfx_lut.w);
#endif

	return lerp(base_col.rgb, lut_col.rgb, ssfx_lut.x);

}