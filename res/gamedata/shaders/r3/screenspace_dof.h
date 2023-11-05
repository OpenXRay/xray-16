/**
 * @ Version: SCREEN SPACE SHADERS - UPDATE 12
 * @ Description: DOF Implementation
 * @ Modified time: 2022-11-09 01:54
 * @ Author: https://www.moddb.com/members/ascii1457
 * @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders
 */

#include "check_screenspace.h"

uniform float4 ssfx_wpn_dof_1;
uniform float4 ssfx_wpn_dof_2;

float3 SSFX_DOF(float2 tc, float3 depth, float3 img)
{
	// Full Blur Scene + CA
	float CA = float2(4.0f / screen_res.x, 4.0f / screen_res.y);
	float3 blur_far = 0;
	blur_far.r = s_blur_2.SampleLevel(smp_rtlinear, tc + CA, 0).r;
	blur_far.g = s_blur_2.SampleLevel(smp_rtlinear, tc, 0).g;
	blur_far.b = s_blur_2.SampleLevel(smp_rtlinear, tc - CA, 0).b;

	// Use Depth to adjust blur intensity
	float blur_w = lerp(ssfx_wpn_dof_1.w, 0, smoothstep(ssfx_wpn_dof_1.x, ssfx_wpn_dof_1.y, depth.z ) );	
	blur_w *= depth.z > SKY_EPS; // Don't apply to the sky ( Sky depth = float(0.001) )

	float edgeBlur = 0;

	// Peripheral vision blur
	if (ssfx_wpn_dof_2.x > 0)
	{
		// Vignette to calc blur
		float2 mid_uv = tc - float2(0.5f, 0.5f);
 		edgeBlur = pow(smoothstep(0.0f, saturate(1.0f - ssfx_wpn_dof_2.x), length(mid_uv)), 1.5f) * 1.33f;

		blur_w = saturate(blur_w + edgeBlur) * ssfx_wpn_dof_1.w;
	}

	// Close blur ( Weapon blur ) 
	if (blur_w > 0)
	{
		float offset_f = 0.001f;
		float ratio = screen_res.y / screen_res.x;
		float2 blur_res = float2(blur_w, blur_w) * float2(offset_f * ratio, offset_f);

		// Offset pattern
		float2 blur_Offsets[8] =
		{
			float2( -blur_res.x,-blur_res.y),	// XOX
			float2( blur_res.x,blur_res.y),		// OOO
			float2( blur_res.x,-blur_res.y),	// XOX
			float2(-blur_res.x,blur_res.y),

			float2( 0,-blur_res.y),	// OXO
			float2( blur_res.x,0),	// XOX
			float2( -blur_res.x,0),	// OXO
			float2(0,blur_res.y)
		};

		float3 Wpn_Blur = 0;

		// Create blur
		for (int b = 0; b < 8; b++)
		{
			Wpn_Blur += s_image.SampleLevel(smp_rtlinear, tc + blur_Offsets[b], 0).rgb;
			Wpn_Blur += s_image.SampleLevel(smp_rtlinear, tc + blur_Offsets[b] * 2, 0).rgb;
		}

		// Normalize blur
		img = Wpn_Blur / 16;

		// Peripheral vision blur with extra help from s_blur_2
		img = lerp(img, blur_far, saturate(0.4f - (1.0f - edgeBlur)));
	}
	
	// Far blur ( Reload, Inventory and PDA )
	if (ssfx_wpn_dof_1.z > 0)
	{
		// Let's use s_blur_2 for far blur
		img = lerp(img, blur_far, saturate(smoothstep(1.0f, 2.4f, length(depth) ) + int(depth.z <= SKY_EPS)) * ssfx_wpn_dof_1.z);
	}

	return img;
}