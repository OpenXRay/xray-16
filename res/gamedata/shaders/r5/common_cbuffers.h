#ifndef	common_cbuffers_h_included
#define	common_cbuffers_h_included

#ifndef MSAA_OPTIMIZATION
cbuffer	dynamic_light
{
	float4	Ldynamic_color;
	float4	Ldynamic_pos;
	float4	Ldynamic_dir;
}
#else
cbuffer	dynamic_light
{
	float4	Ldynamic_color;
	float4	Ldynamic_pos;
	float4	Ldynamic_dir;
}
#endif

#endif