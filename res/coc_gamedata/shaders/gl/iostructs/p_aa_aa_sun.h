
out vec4 SV_Target;
#ifdef GBUFFER_OPTIMIZATION
in vec4 gl_FragCoord;
#endif
#ifdef MSAA_OPTIMIZATION
in int gl_SampleID;
#endif

layout(location = TEXCOORD0)	in float2 	p_aa_AA_sun_tc		; // TEXCOORD0;
layout(location = TEXCOORD1)	in float2	p_aa_AA_sun_unused	; // TEXCOORD1;
layout(location = TEXCOORD2)	in float2 	p_aa_AA_sun_LT		; // TEXCOORD2;
layout(location = TEXCOORD3)	in float2	p_aa_AA_sun_RT		; // TEXCOORD3;
layout(location = TEXCOORD4)	in float2 	p_aa_AA_sun_LB		; // TEXCOORD4;
layout(location = TEXCOORD5)	in float2	p_aa_AA_sun_RB		; // TEXCOORD5;

#ifdef MSAA_OPTIMIZATION
#ifdef GBUFFER_OPTIMIZATION
float4 	_main	( p_aa_AA_sun I, float4 pos2d, uint iSample );
#else
float4 	_main	( p_aa_AA_sun I, uint iSample );
#endif
#else
#ifdef GBUFFER_OPTIMIZATION
float4 	_main	( p_aa_AA_sun I, float4 pos2d );
#else
float4 	_main	( p_aa_AA_sun I );
#endif
#endif

void main()
{
	p_aa_AA_sun	I;
	I.tc		= p_aa_AA_sun_tc;
	I.unused	= p_aa_AA_sun_unused;
	I.LT		= p_aa_AA_sun_LT;
	I.RT		= p_aa_AA_sun_RT;
	I.LB		= p_aa_AA_sun_LB;
	I.RB		= p_aa_AA_sun_RB;

#ifdef MSAA_OPTIMIZATION
#ifdef GBUFFER_OPTIMIZATION
	SV_Target	= _main	( I, gl_FragCoord, gl_SampleID );
#else
	SV_Target	= _main	( I, gl_SampleID );
#endif
#else
#ifdef GBUFFER_OPTIMIZATION
	SV_Target	= _main	( I, gl_FragCoord );
#else
	SV_Target	= _main	( I );
#endif
#endif
}
