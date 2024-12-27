#ifndef	common_cbuffers_h_included
#define	common_cbuffers_h_included

#ifndef MSAA_OPTIMIZATION
//	Used by dynamic lights and volumetric effects
cbuffer	dynamic_light
{
	float4	Ldynamic_color;	// dynamic light color (rgb1)	- spot/point/sun
	float4	Ldynamic_pos;	// dynamic light pos+1/range(w)	- spot/point
	float4	Ldynamic_dir;	// dynamic light direction		- sun
}
#else
cbuffer	dynamic_light
{
	float4	Ldynamic_color;	// dynamic light color (rgb1)	- spot/point/sun
	float4	Ldynamic_pos;	// dynamic light pos+1/range(w)	- spot/point
	float4	Ldynamic_dir;	// dynamic light direction		- sun
}
#endif

#endif	//	common_cbuffers_h_included