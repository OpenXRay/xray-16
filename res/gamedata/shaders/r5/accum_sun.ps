#include "common.h"
#include "lmodel.h"
#include "shadow.h"

/*
struct 	_input
{
	float2 	tc 	: TEXCOORD0;
	float4 	tcJ : TEXCOORD1;
	float2	LT	: TEXCOORD2;
	float2	RT	: TEXCOORD3;
	float2	LB	: TEXCOORD4;
	float2 	RB	: TEXCOORD5;
};
*/

#ifdef MSAA_OPTIMIZATION
#ifdef GBUFFER_OPTIMIZATION
float4 	main	( p_aa_AA_sun I, float4 pos2d : SV_Position, uint iSample : SV_SAMPLEINDEX ) : SV_Target
#else
float4 	main	( p_aa_AA_sun I, uint iSample : SV_SAMPLEINDEX ) : SV_Target
#endif
#else
#ifdef GBUFFER_OPTIMIZATION
float4 	main	( p_aa_AA_sun I, float4 pos2d : SV_Position ) : SV_Target
#else
float4 	main	( p_aa_AA_sun I ) : SV_Target
#endif
#endif
{
	gbuffer_data gbd = gbuffer_load_data( GLD_P(I.tc, pos2d, ISAMPLE) );

//  float4 _P		= tex2D 	(s_position, 	I.tc); 
//  float4  _N		= tex2D 	(s_normal,   	I.tc); 
	float4 	_P	= float4( gbd.P, gbd.mtl );
	float4	_N	= float4( gbd.N, gbd.hemi );

	// ----- light-model
	float 	m	= xmaterial	;
# ifndef USE_R2_STATIC_SUN
			m 	= _P.w		;
# endif
	float4	light	= plight_infinity( m, _P, _N, Ldynamic_dir );

	// ----- shadow
	float4 	s_sum;
//		s_sum.x	= tex2D	( s_smap, I.LT).x;
//		s_sum.y = tex2D	( s_smap, I.RT).y;
//		s_sum.z	= tex2D	( s_smap, I.LB).z;
//		s_sum.w = tex2D	( s_smap, I.RB).w;
		s_sum.x	= s_smap.Sample( smp_nofilter, I.LT).x;
		s_sum.y = s_smap.Sample( smp_nofilter, I.RT).y;
		s_sum.z	= s_smap.Sample( smp_nofilter, I.LB).z;
		s_sum.w = s_smap.Sample( smp_nofilter, I.RB).w;
//	float 	s 	= dot	( s_sum, 1.h/4.h);
	float 	s 	= ((s_sum.x+s_sum.y)+(s_sum.z+s_sum.w))*(1.h/4.h);

	return 		Ldynamic_color * light * s;
}
