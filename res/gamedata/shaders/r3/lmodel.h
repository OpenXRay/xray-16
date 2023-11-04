/**
 * @ Description: Enhanced Shaders and Color Grading 1.10
 * @ Author: https://www.moddb.com/members/kennshade
 * @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/enhanced-shaders-and-color-grading-for-151
 */
 
#ifndef	LMODEL_H
#define LMODEL_H

#include "common.h"
#include "common_brdf.h"
#include "pbr_brdf.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Lighting formulas

float4 compute_lighting(float3 N, float3 V, float3 L, float4 alb_gloss, float mat_id)
{
	// [ SSS Test ]. Overwrite terrain material
	bool m_terrain = abs(mat_id - 0.95) <= 0.04f;
	if (m_terrain)
		mat_id = 0;
	
	float3 albedo = calc_albedo(alb_gloss, mat_id);
	float3 specular = calc_specular(alb_gloss, mat_id);
	float rough = calc_rough(alb_gloss, mat_id);
	//calc_rain(albedo, specular, rough, alb_gloss, mat_id, 1);
	calc_foliage(albedo, specular, rough, alb_gloss, mat_id);
	
	float3 light = Lit_BRDF(rough, albedo, specular, V, N, L );

	//if(mat_id == MAT_FLORA) //Be aware of precision loss/errors
	if(abs(mat_id-MAT_FLORA) <= MAT_FLORA_ELIPSON) //Be aware of precision loss/errors
	{
		//Simple subsurface scattering
		float3 subsurface = SSS(N,V,L);
		light.rgb += subsurface*albedo;
	}

	return float4(light, 0);
}

float4 plight_infinity(float m, float3 pnt, float3 normal, float4 c_tex, float3 light_direction )
{
	//gsc vanilla stuff
	float3 N = normalize(normal);							// normal 
	float3 V = normalize(-pnt);					// vector2eye
	float3 L = normalize(-light_direction);						// vector2light

	float4 light = compute_lighting(N,V,L,c_tex,m);
	
	return light; // output (albedo.gloss)
}

float4 plight_local(float m, float3 pnt, float3 normal, float4 c_tex, float3 light_position, float light_range_rsq, out float rsqr )
{
	float atteps = 0.1;
	
	float3 L2P = pnt - light_position;                       		// light2point 
	rsqr = dot(L2P,L2P); // distance 2 light (squared)
	rsqr = max(rsqr, atteps);
	//rsqr = rsqr + 1.0;
	
	//vanilla atten - linear
	float att = saturate(1.0 - rsqr*light_range_rsq); // q-linear attenuate
	att = SRGBToLinear(att);
	/*
	//unity atten - quadtratic
	//catlikecoding.com/unity/tutorials/custom-srp/point-and-spot-lights/
	att = rsqr * light_range_rsq;
	att *= att;
	att = saturate(1.0 - att);
	att *= att;
	att = att / rsqr;
	*/
	float3 N = normalize(normal);							// normal 
	float3 V = normalize(-pnt);					// vector2eye
	float3 L = normalize(-L2P);					// vector2light

	float4 light = compute_lighting(N,V,L,c_tex,m);
	
	return att*light;		// output (albedo.gloss)
}

float3 specular_phong(float3 pnt, float3 normal, float3 light_direction)
{
	float3 H = normalize(pnt + light_direction );
	float nDotL = saturate(dot(normal, light_direction));
	float nDotH = saturate(dot(normal, H));
	float nDotV = saturate(dot(normal, pnt));
	float lDotH = saturate(dot(light_direction, H));
	//float vDotH = saturate(dot(pnt, H));
	return L_sun_color.rgb * Lit_Specular(nDotL, nDotH, nDotV, lDotH, 0.02, 0.1);
}

//	TODO: DX10: Remove path without blending
half4 blendp(half4 value, float4 tcp)
{
	return 	value;
}

half4 blend(half4 value, float2 tc)
{
	return 	value;
}

#endif