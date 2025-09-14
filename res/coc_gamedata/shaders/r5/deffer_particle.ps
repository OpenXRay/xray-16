#if ( defined(MSAA_ALPHATEST_DX10_1_ATOC) || defined(MSAA_ALPHATEST_DX10_1) ) 
#define EXTEND_F_DEFFER
#endif

#include "common.h"
#include "sload.h"

struct 	p_particle
{
	float4 	color	: COLOR0;
	p_flat	base;	
};

#ifdef	MSAA_ALPHATEST_DX10_1_ATOC
f_deffer 	main	( p_particle II, float4 pos2d : SV_Position )
#else	//	MSAA_ALPHATEST_DX10_1_ATOC
f_deffer 	main	( p_particle II )
#endif	//	MSAA_ALPHATEST_DX10_1_ATOC
{
  f_deffer	O;
  p_flat	I;	I=II.base;

  // 1. Base texture + kill pixels with low alpha
//  float4 	D 	= 	tex2D		(s_base, I.tcdh);
#if defined(MSAA_ALPHATEST_DX10_1)
  uint mask = 0x0;
  float2 ddx_base = ddx( I.tcdh );
  float2 ddy_base = ddy( I.tcdh );

  float2 pixeloffset = MSAAOffsets[0]*(1.0/16.0);
  
  float2 texeloffset =  pixeloffset.x * ddx_base + pixeloffset.y * ddy_base;

  float4 	D 	= 	s_base.Sample( smp_base, I.tcdh + texeloffset );
			D	*=	II.color;

  if( D.w-def_aref >= 0 ) mask |= 0x1;

  [unroll] for( int i = 1; i < MSAA_SAMPLES; ++i )
  {
	pixeloffset = MSAAOffsets[i]*(1.0/16.0);
	texeloffset =  pixeloffset.x * ddx_base + pixeloffset.y * ddy_base;
	float4 DI 	= s_base.Sample( smp_base, I.tcdh+ texeloffset );
	DI *=	II.color;
	if( DI.w-def_aref >= 0 ) mask |= ( uint(0x1) << i );
  }
	
  if( mask == 0x0 )
	discard;
#else
  float4 	D 	= 	s_base.Sample( smp_base, I.tcdh);
			D	*=	II.color;

#ifdef 	MSAA_ALPHATEST_DX10_1_ATOC
	float alpha = (D.w-def_aref*0.5f)/(1-def_aref*0.5f);
	uint mask = alpha_to_coverage ( alpha, pos2d );
#else	//	MSAA_ALPHATEST_DX10_1_ATOC
	clip	(D.w-def_aref);
#endif	//	MSAA_ALPHATEST_DX10_1_ATOC
			
#endif

  // 2. Standart output
  float4		Ne  = float4	(normalize((float3)I.N.xyz)					, I.position.w	);
#ifndef EXTEND_F_DEFFER
  O				= pack_gbuffer( 
								Ne,
								float4 	(I.position.xyz + Ne.xyz*def_virtualh/2.h	, xmaterial		),
								float4	(D.xyz,			def_gloss) );		// OUT: rgb.gloss
#else
  O				= pack_gbuffer( 
								Ne,
								float4 	(I.position.xyz + Ne.xyz*def_virtualh/2.h	, xmaterial		),
								float4	(D.xyz,			def_gloss),
								mask );		// OUT: rgb.gloss
#endif								
								
  return O;
}
