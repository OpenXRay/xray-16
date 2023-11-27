#include "common.h"
#include "lmodel.h"

#ifndef ISAMPLE
#define ISAMPLE 0
#endif

float3x4 m_sunmask;	// ortho-projection
#ifndef USE_MSAA
Texture2D	s_patched_normal;
#else
Texture2DMS< float4, MSAA_SAMPLES >	s_patched_normal;
#endif

#ifdef MSAA_OPTIMIZATION
float4 main ( float2 tc : TEXCOORD0, float2 tcJ : TEXCOORD1, uint iSample : SV_SAMPLEINDEX ) : SV_Target
#else
float4 main ( float2 tc : TEXCOORD0, float2 tcJ : TEXCOORD1 ) : SV_Target
#endif
{
#ifndef USE_MSAA
	float3 _N = s_patched_normal.Sample( smp_nofilter, tc );
#else
#ifndef MSAA_OPTIMIZATION
	float3 _N = s_patched_normal.Load(int3( tc * pos_decompression_params2.xy, 0 ), ISAMPLE );
#else
	float3 _N = s_patched_normal.Load(int3( tc * pos_decompression_params2.xy, 0 ), iSample);
#endif	
#endif

#ifndef GBUFFER_OPTIMIZATION	
	return float4(_N,1);
#else
	return float4( gbuf_pack_normal( _N ), 0, 0 );
#endif
}
