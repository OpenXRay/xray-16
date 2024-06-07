/*
	Common functions used by lighting 
	Material table
	
	/////////////////
	Anomaly Team 2020
	/////////////////
*/
#include "common.h"

// SSS Settings
#include "settings_screenspace_FLORA.h"

////////////////////////
//Material table
#define MAT_FLORA 0.15f
#define MAT_FLORA_ELIPSON 0.04f

// Simple subsurface scattering
float3 SSS(float3 N, float3 V, float3 L)
{
	float S = saturate(dot(V, -(L + N))) * ssfx_florafixes_2.x;
	return S * lerp(float3(1.0f, 1.0f, 1.0f), L_sun_color.rgb, ssfx_florafixes_2.y);
}