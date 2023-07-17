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
float SSS(float3 N, float3 V, float3 L)
{
	float S = saturate(dot(V, -(L + N))) * G_SSS_INTENSITY;
	return S;
}