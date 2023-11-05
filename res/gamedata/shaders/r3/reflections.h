#ifndef REFLECTIONS_H
#define REFLECTIONS_H

TextureCube	s_env0;
TextureCube	s_env1;

float3 calc_envmap(float3 vreflect)
{
	vreflect.y = vreflect.y*2-1;
	float3	env0	= s_env0.SampleLevel( smp_base, vreflect.xyz, 0).xyz;
	float3	env1	= s_env1.SampleLevel( smp_base, vreflect.xyz, 0).xyz;
	return lerp (env0,env1,L_ambient.w);
}
#endif