#include "common.h"
#include "ogse_config.h"
#include "ogse_functions.h"

#define EPS 0.0001f
uniform float4 screen_res;

float4 main(v2p_ssss I) : SV_Target
{
	float4 depth;
#ifndef USE_MSAA
	depth.x = s_position.Sample(smp_nofilter, I.tc0.xy + float2(0, 1.0) * screen_res.zw).z;
	depth.y = s_position.Sample(smp_nofilter, I.tc0.xy + float2(1, 0.65) * screen_res.zw).z;
	depth.z = s_position.Sample(smp_nofilter, I.tc0.xy + float2(-1, 0.65) * screen_res.zw).z;
#else
	depth.x = s_position.Load(int3((I.tc0.xy + float2(0, 1.0) * screen_res.zw) * pos_decompression_params2.xy, 0), 0).z;
	depth.y = s_position.Load(int3((I.tc0.xy + float2(1, 0.65) * screen_res.zw) * pos_decompression_params2.xy, 0), 0).z;
	depth.z = s_position.Load(int3((I.tc0.xy + float2(-1, 0.65) * screen_res.zw) * pos_decompression_params2.xy, 0), 0).z;
#endif
	
	float4 sceneDepth;
	sceneDepth.x = normalize_depth(depth.x)*is_not_sky(depth.x);
	sceneDepth.y = normalize_depth(depth.y)*is_not_sky(depth.y);
	sceneDepth.z = normalize_depth(depth.z)*is_not_sky(depth.z);

	sceneDepth.w = (sceneDepth.x + sceneDepth.y + sceneDepth.z) * 0.333;
	
	depth.w = saturate(1 - sceneDepth.w*1000);
	
	float4 Color = float4(depth.w, depth.w, depth.w, sceneDepth.w);	
	return Color;
}