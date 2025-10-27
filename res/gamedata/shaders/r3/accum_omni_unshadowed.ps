#include "common.h"
#include "lmodel.h"

//	TODO: DX10: Move to Load
#ifdef MSAA_OPTIMIZATION
#ifdef GBUFFER_OPTIMIZATION
float4 main ( float4 tc:TEXCOORD0, float4 pos2d : SV_Position, uint iSample : SV_SAMPLEINDEX ) : SV_Target
#else
float4 main ( float4 tc:TEXCOORD0, uint iSample : SV_SAMPLEINDEX ) : SV_Target
#endif
#else
#ifdef GBUFFER_OPTIMIZATION
float4 main ( float4 tc:TEXCOORD0, float4 pos2d : SV_Position ) : SV_Target
#else
float4 main ( float4 tc:TEXCOORD0 ) : SV_Target
#endif
#endif
{
	const float bias_mul 	= 0.999f;

 	// Sample the fat framebuffer:
//	float4 _P		= tex2Dproj 	(s_position, tc); 
//	float4 _N		= tex2Dproj 	(s_normal,   tc); 
	float2	tcProj	= tc.xy / tc.w;

	gbuffer_data gbd = gbuffer_load_data( GLD_P(tcProj, pos2d, ISAMPLE) );

	float4 _P		= float4( gbd.P,gbd.mtl );
	float4 _N		= float4( gbd.N,gbd.hemi );

	float 	m	= xmaterial	;
# ifndef USE_R2_STATIC_SUN
			m 	= _P.w		;
# endif

	float	rsqr;
	float4	light 		= plight_local( m, _P, _N, Ldynamic_pos, Ldynamic_pos.w, rsqr );
	return 	blendp( Ldynamic_color * light, tc);
}
