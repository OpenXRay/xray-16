#ifndef	common_functions_h_included
#define	common_functions_h_included

float Contrast(float Input, float ContrastPower)
{
     bool IsAboveHalf = Input > 0.5 ;
     float ToRaise = saturate(2*(IsAboveHalf ? 1-Input : Input));
     float Output = 0.5*pow(ToRaise, ContrastPower); 
     Output = IsAboveHalf ? 1-Output : Output;
     return Output;
}

void tonemap( out float4 low, out float4 high, float3 rgb, float scale)
{
	rgb		=	rgb*scale;

	const float fWhiteIntensity = 1.7;

	const float fWhiteIntensitySQR = fWhiteIntensity*fWhiteIntensity;

	low		=	( (rgb*(1+rgb/fWhiteIntensitySQR)) / (rgb+1) ).xyzz;

	high	=	rgb.xyzz/def_hdr;

}

float4 combine_bloom( float3  low, float4 high)
{
        return float4( low + high.rgb*high.a, 1.0 );
}

float calc_fogging( float4 w_pos )      
{
	return dot(w_pos,fog_plane);         
}

float2 unpack_tc_base( float2 tc, float du, float dv )
{
		return (tc.xy + float2	(du,dv))*(32.f/32768.f);
}

float3 calc_sun_r1( float3 norm_w )    
{
	return L_sun_color*saturate(dot((norm_w),-L_sun_dir_w));                 
}

float3 calc_model_hemi_r1( float3 norm_w )
{
 return max(0,norm_w.y)*L_hemi_color.rgb;
}

float3 calc_model_lq_lighting( float3 norm_w )
{
	return L_material.x*calc_model_hemi_r1(norm_w) + L_ambient.rgb + L_material.y*calc_sun_r1(norm_w);
}

float3 	unpack_normal( float3 v )	{ return 2*v-1; }
float3 	unpack_bx2( float3 v )	{ return 2*v-1; }
float3 	unpack_bx4( float3 v )	{ return 4*v-2; }
float2 	unpack_tc_lmap( float2 tc )	{ return tc*(1.f/32768.f);	}
float4	unpack_color( float4 c ) { return c.bgra; }
float4	unpack_D3DCOLOR( float4 c ) { return c.bgra; }
float3	unpack_D3DCOLOR( float3 c ) { return c.bgr; }

float3   p_hemi( float2 tc )
{
	float4	t_lmh = s_hemi.Sample( smp_rtlinear, tc);
	return	t_lmh.a;
}

float   get_hemi( float4 lmh)
{
	return lmh.a;
}

float   get_sun( float4 lmh)
{
	return lmh.g;
}

float3	v_hemi(float3 n)
{
	return L_hemi_color.rgb*(.5f + .5f*n.y);
}

float3	v_sun(float3 n)                        	
{
	return L_sun_color*dot(n,-L_sun_dir_w);                
}

float3	calc_reflection( float3 pos_w, float3 norm_w )
{
    return reflect(normalize(pos_w-eye_position), norm_w);
}

#define USABLE_BIT_1                uint(0x00002000)
#define USABLE_BIT_2                uint(0x00004000)
#define USABLE_BIT_3                uint(0x00008000)
#define USABLE_BIT_4                uint(0x00010000)
#define USABLE_BIT_5                uint(0x00020000)
#define USABLE_BIT_6                uint(0x00040000)
#define USABLE_BIT_7                uint(0x00080000)
#define USABLE_BIT_8                uint(0x00100000)
#define USABLE_BIT_9                uint(0x00200000)
#define USABLE_BIT_10               uint(0x00400000)
#define USABLE_BIT_11               uint(0x00800000)
#define USABLE_BIT_12               uint(0x01000000)
#define USABLE_BIT_13               uint(0x02000000)
#define USABLE_BIT_14               uint(0x04000000)
#define USABLE_BIT_15               uint(0x80000000)
#define MUST_BE_SET                 uint(0x40000000)

float2 gbuf_pack_normal( float3 norm )
{
   float2 res;

   res.x  = norm.z;
   res.y  = 0.5f * ( norm.x + 1.0f ) ;
   res.y *= ( norm.y < 0.0f ? -1.0f : 1.0f );

   return res;
}

float3 gbuf_unpack_normal( float2 norm )
{
   float3 res;

   res.z  = norm.x;
   res.x  = ( 2.0f * abs( norm.y ) ) - 1.0f;
   res.y = ( norm.y < 0 ? -1.0 : 1.0 ) * sqrt( abs( 1 - res.x * res.x - res.z * res.z ) );

   return res;
}

float gbuf_pack_hemi_mtl( float hemi, float mtl )
{
   uint packed_mtl = uint( ( mtl / 1.333333333 ) * 31.0 );
	uint packed = ( MUST_BE_SET + ( uint( saturate(hemi) * 255.9 ) << 13 ) + ( ( packed_mtl & uint( 31 ) ) << 21 ) );

   if( ( packed & USABLE_BIT_13 ) == 0 )
      packed |= USABLE_BIT_14;

   if( (packed_mtl & uint( 16 )) != 0 )
      packed |= USABLE_BIT_15;

   return asfloat( packed );
}

float gbuf_unpack_hemi( float mtl_hemi )
{
	return float( ( asuint( mtl_hemi ) >> 13 ) & uint(255) ) * (1.0/254.8);
}

float gbuf_unpack_mtl( float mtl_hemi )
{
   uint packed       = asuint( mtl_hemi );
   uint packed_hemi  = ( ( packed >> 21 ) & uint(15) ) + ( ( packed & USABLE_BIT_15 ) == 0 ? 0 : 16 );
   return float( packed_hemi ) * (1.0/31.0) * 1.333333333;
}

#ifndef EXTEND_F_DEFFER
f_deffer pack_gbuffer( float4 norm, float4 pos, float4 col )
#else
f_deffer pack_gbuffer( float4 norm, float4 pos, float4 col, uint imask )
#endif
{
	f_deffer res;

#ifndef GBUFFER_OPTIMIZATION
	res.position	= pos;
	res.Ne			= norm;
	res.C			   = col;
#else
	res.position	= float4( gbuf_pack_normal( norm.xyz ), pos.z, gbuf_pack_hemi_mtl( norm.w, pos.w ) );
	res.C			   = col;
#endif

#ifdef EXTEND_F_DEFFER
   res.mask = imask;
#endif

	return res;
}

#ifdef GBUFFER_OPTIMIZATION
gbuffer_data gbuffer_load_data( float2 tc : TEXCOORD, float2 pos2d, int iSample )
{
	gbuffer_data gbd;

	gbd.P = float3(0,0,0);
	gbd.hemi = 0;
	gbd.mtl = 0;
	gbd.C = 0;
	gbd.N = float3(0,0,0);

#ifndef USE_MSAA
	float4 P	= s_position.Sample( smp_nofilter, tc );
#else
	float4 P	= s_position.Load( int3( pos2d, 0 ), iSample );
#endif

	gbd.P  = float3( P.z * ( pos2d * pos_decompression_params.zw - pos_decompression_params.xy ), P.z );

	gbd.N = gbuf_unpack_normal( P.xy );

	gbd.mtl	= gbuf_unpack_mtl( P.w );

   gbd.hemi = gbuf_unpack_hemi( P.w );

#ifndef USE_MSAA
   float4	C	= s_diffuse.Sample( smp_nofilter, tc );
#else
   float4	C	= s_diffuse.Load( int3( pos2d, 0 ), iSample );
#endif

	gbd.C		= C.xyz;
	gbd.gloss	= C.w;

	return gbd;
}

gbuffer_data gbuffer_load_data( float2 tc : TEXCOORD, float2 pos2d )
{
   return gbuffer_load_data( tc, pos2d, 0 );
}

gbuffer_data gbuffer_load_data_offset( float2 tc : TEXCOORD, float2 OffsetTC : TEXCOORD, float2 pos2d )
{
	float2  delta	  = ( ( OffsetTC - tc ) * pos_decompression_params2.xy );

	return gbuffer_load_data( OffsetTC, pos2d + delta, 0 );
}

gbuffer_data gbuffer_load_data_offset( float2 tc : TEXCOORD, float2 OffsetTC : TEXCOORD, float2 pos2d, uint iSample )
{
   float2  delta	  = ( ( OffsetTC - tc ) * pos_decompression_params2.xy );

   return gbuffer_load_data( OffsetTC, pos2d + delta, iSample );
}

#else
gbuffer_data gbuffer_load_data( float2 tc : TEXCOORD, uint iSample )
{
	gbuffer_data gbd;

#ifndef USE_MSAA
	float4 P	= s_position.Sample( smp_nofilter, tc );
#else
   float4 P	= s_position.Load( int3( tc * pos_decompression_params2.xy, 0 ), iSample );
#endif

	gbd.P		= P.xyz;
	gbd.mtl		= P.w;

#ifndef USE_MSAA
	float4 N	= s_normal.Sample( smp_nofilter, tc );
#else
	float4 N	= s_normal.Load( int3( tc * pos_decompression_params2.xy, 0 ), iSample );
#endif

	gbd.N		= N.xyz;
	gbd.hemi	= N.w;

#ifndef USE_MSAA
	float4	C	= s_diffuse.Sample(  smp_nofilter, tc );
#else
	float4	C	= s_diffuse.Load( int3( tc * pos_decompression_params2.xy, 0 ), iSample );
#endif


	gbd.C		= C.xyz;
	gbd.gloss	= C.w;

	return gbd;
}

gbuffer_data gbuffer_load_data( float2 tc : TEXCOORD  )
{
   return gbuffer_load_data( tc, 0 );
}

gbuffer_data gbuffer_load_data_offset( float2 tc : TEXCOORD, float2 OffsetTC : TEXCOORD, uint iSample )
{
   return gbuffer_load_data( OffsetTC, iSample );
}

#endif

#if ( defined( MSAA_ALPHATEST_DX10_1_ATOC ) || defined( MSAA_ALPHATEST_DX10_1 ) )

#if MSAA_SAMPLES == 2
uint alpha_to_coverage ( float alpha, float2 pos2d )
{
	uint mask;
	uint pos = uint(pos2d.x) | uint( pos2d.y);
	if( alpha < 0.3333 )
		mask = 0;
	else if( alpha < 0.6666 )
		mask = 1 << ( pos & 1 );
	else 
		mask = 3;

	return mask;
}
#endif

#if MSAA_SAMPLES == 4
uint alpha_to_coverage ( float alpha, float2 pos2d )
{
	uint mask;

	float off = float( ( uint(pos2d.x) | uint( pos2d.y) ) & 3 );
	alpha = saturate( alpha - off * ( ( 0.2 / 4.0 ) / 3.0 ) );
	if( alpha < 0.40 )
	{
		if( alpha < 0.20 )
			mask = 0;	
		else if( alpha < 0.40 )
			mask = 1;
	}
  else
  {
	if( alpha < 0.60 )
	{
		mask = 3;
	}
	else if( alpha < 0.8 )
	  mask = 7;
	else
	  mask = 0xf;
 }

	return mask;
}
#endif

#if MSAA_SAMPLES == 8
uint alpha_to_coverage ( float alpha, float2 pos2d )
{
	uint mask;

	float off = float( ( uint(pos2d.x) | uint( pos2d.y) ) & 3 );
	alpha = saturate( alpha - off * ( ( 0.1111 / 8.0 ) / 3.0 ) );
  if( alpha < 0.4444 )
  {
	if( alpha < 0.2222 )
	{
		if( alpha < 0.1111 )
			mask = 0;	
		else
			mask = 1;
	}
	else 
	{
		if( alpha < 0.3333 )
		{  
			mask = 3;
		}
	    else
		{
			mask = 0x7;
		}
	}
  }
  else
  {
	  if( alpha < 0.6666 )
	  {
		if( alpha < 0.5555 )
		{
			mask = 0xf;
		}
		else
		{
			mask = 0x1F;
		}
	  }
	  else
	  {
		if( alpha < 0.7777 )
		{
			mask = 0x3F;
		}
		else if( alpha < 0.8888 )
		{
			mask = 0x7F;
		}
		else
			mask = 0xFF;
	 }
  }

	return mask;
}
#endif
#endif



#endif
