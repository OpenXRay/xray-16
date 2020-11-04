#ifndef	common_functions_h_included
#define	common_functions_h_included

//	contrast function
float Contrast(float Input, float ContrastPower)
{
     //piecewise contrast function
     bool IsAboveHalf = Input > 0.5 ;
     float ToRaise = saturate(2*(IsAboveHalf ? (1.0 - Input) : Input));
     float Output = 0.5*pow(ToRaise, ContrastPower); 
     Output = IsAboveHalf ? (1.0 - Output) : Output;
     return Output;
}

#ifdef USE_SHOC_RESOURCES
void tonemap( out float4 low, out float4 high, float3 rgb, float scale)
{
        rgb	= rgb*scale;

        low	= rgb.xyzz;
        high	= low/def_hdr;        // 8x dynamic range
}
#else // USE_SHOC_RESOURCES
void tonemap( out float4 low, out float4 high, float3 rgb, float scale)
{
	rgb		= rgb*scale;

	const float fWhiteIntensity = 1.7;
	const float fWhiteIntensitySQR = fWhiteIntensity*fWhiteIntensity;

	low	= ( (rgb*(1.0+rgb/fWhiteIntensitySQR)) / (rgb+1.0) ).xyzz;
	high	= rgb.xyzz/def_hdr;	// 8x dynamic range
}
#endif // USE_SHOC_RESOURCES

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
	return (tc.xy + float2	(du,dv))*(32.0/32768.0); //!Increase from 32bit to 64bit floating point
}

float3 calc_sun_r1( float3 norm_w )    
{
	return L_sun_color*saturate(dot((norm_w),-L_sun_dir_w));                 
}

float3 calc_model_hemi_r1( float3 norm_w )    
{
	return max(0.0,norm_w.y)*L_hemi_color.rgb;
}

float3 calc_model_lq_lighting( float3 norm_w )    
{
	return L_material.x*calc_model_hemi_r1(norm_w) + L_ambient.rgb + L_material.y*calc_sun_r1(norm_w);
}

float3 	unpack_normal( float3 v )	{ return 2*v-1; }
float3 	unpack_normal( float4 v )	{ return 2*v.xyz-1; }
float3 	unpack_bx2( float3 v )	{ return 2*v-1; }
float3 	unpack_bx2( float4 v )	{ return 2*v.xyz-1; }
float3 	unpack_bx4( float3 v )	{ return 4*v-2; } //!reduce the amount of stretching from 4*v-2 and increase precision
float3 	unpack_bx4( float4 v )	{ return 4*v.xyz-2; }
float2 	unpack_tc_lmap( float2 tc )	{ return tc*(1.0/32768.0);	} // [-1  .. +1 ] 
float4	unpack_color( float4 c ) { return c.bgra; }
float4	unpack_D3DCOLOR( float4 c ) { return c.bgra; }
float3	unpack_D3DCOLOR( float3 c ) { return c.bgr; }

float3   p_hemi(float2 tc)
{
	float4 t_lmh = tex2D(s_hemi, tc);

#ifdef USE_SHOC_RESOURCES
	float r_lmh = (1.0/3.0);
	return float3(dot(t_lmh.rgb, float3(r_lmh, r_lmh, r_lmh)));
#else // USE_SHOC_RESOURCES
	return float3(t_lmh.a);
#endif // USE_SHOC_RESOURCES
}

float	get_hemi(float4 lmh)
{
#ifdef USE_SHOC_RESOURCES
	float r_lmh = (1.0/3.0);
	return dot(lmh.rgb, float3(r_lmh, r_lmh, r_lmh));
#else // USE_SHOC_RESOURCES
	return lmh.a;
#endif // USE_SHOC_RESOURCES
}

float	get_sun(float4 lmh)
{
#ifdef USE_SHOC_RESOURCES
	return lmh.a;
#else // USE_SHOC_RESOURCES
	return lmh.g;
#endif // USE_SHOC_RESOURCES
}

float3	v_hemi(float3 n)
{
	return L_hemi_color.rgb*(0.5 + 0.5*n.y);                   
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
#define USABLE_BIT_11               uint(0x00800000)   // At least two of those four bit flags must be mutually exclusive (i.e. all 4 bits must not be set together)
#define USABLE_BIT_12               uint(0x01000000)   // This is because setting 0x47800000 sets all 5 FP16 exponent bits to 1 which means infinity
#define USABLE_BIT_13               uint(0x02000000)   // This will be translated to a +/-MAX_FLOAT in the FP16 render target (0xFBFF/0x7BFF), overwriting the 
#define USABLE_BIT_14               uint(0x04000000)   // mantissa bits where other bit flags are stored.
#define USABLE_BIT_15               uint(   1 << 31)   // = uint(0x80000000) // fix for integrated Intel cards
#define MUST_BE_SET                 uint(0x40000000)   // This flag *must* be stored in the floating-point representation of the bit flag to store

/*
float2 gbuf_pack_normal( float3 norm )
{
   float2 res;

   res = 0.5 * ( norm.xy + float2( 1, 1 ) ) ;
   res.x *= ( norm.z < 0 ? -1.0 : 1.0 );

   return res;
}

float3 gbuf_unpack_normal( float2 norm )
{
   float3 res;

   res.xy = ( 2.0 * abs( norm ) ) - float2(1,1);

   res.z = ( norm.x < 0 ? -1.0 : 1.0 ) * sqrt( abs( 1 - res.x * res.x - res.y * res.y ) );

   return res;
}
*/

// Holger Gruen AMD - I change normal packing and unpacking to make sure N.z is accessible without ALU cost
// this help the HDAO compute shader to run more efficiently
float2 gbuf_pack_normal( float3 norm )
{
   float2 res;

   res.x  = norm.z;
   res.y  = 0.5 * ( norm.x + 1.0 ) ;
   res.y *= ( norm.y < 0.0 ? -1.0 : 1.0 );

   return res;
}

float3 gbuf_unpack_normal( float2 norm )
{
   float3 res;

   res.z  = norm.x;
   res.x  = ( 2.0 * abs( norm.y ) ) - 1.0;
   res.y = ( norm.y < 0 ? -1.0 : 1.0 ) * sqrt( abs( 1 - res.x * res.x - res.z * res.z ) );

   return res;
}

float gbuf_pack_hemi_mtl( float hemi, float mtl )
{
   uint packed_mtl = uint( ( mtl / 1.333333333 ) * 31.0 );
//   uint packed_hemi = ( MUST_BE_SET + ( uint( hemi * 255.0 ) << 13 ) + ( ( packed_mtl & uint( 31 ) ) << 21 ) );
	//	Clamp hemi max value
	uint packed_hemi = ( MUST_BE_SET + ( uint( saturate(hemi) * 255.9 ) << 13 ) + ( ( packed_mtl & uint( 31 ) ) << 21 ) );

   if( ( packed_hemi & USABLE_BIT_13 ) == 0 )
      packed_hemi |= USABLE_BIT_14;

   if( ( packed_mtl & uint( 16 ) ) != 0 )
      packed_hemi |= USABLE_BIT_15;

   return asfloat( packed_hemi );
}

float gbuf_unpack_hemi( float mtl_hemi )
{
//   return float( ( asuint( mtl_hemi ) >> 13 ) & uint(255) ) * (1.0/255.0);
	return float( ( asuint( mtl_hemi ) >> 13 ) & uint(255) ) * (1.0/254.8);
}

float gbuf_unpack_mtl( float mtl_hemi )
{
   uint packed_mtl       = asuint( mtl_hemi );
   uint packed_hemi  = ( ( packed_mtl >> 21 ) & uint(15) ) + ( ( packed_mtl & USABLE_BIT_15 ) == 0 ? 0 : 16 );
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
gbuffer_data gbuffer_load_data( float2 tc, float4 pos2d, uint iSample )
{
	gbuffer_data gbd;

	gbd.P = float3(0,0,0);
	gbd.hemi = 0;
	gbd.mtl = 0;
	gbd.C = float3(0,0,0);
	gbd.N = float3(0,0,0);

#ifndef USE_MSAA
	float4 P	= tex2D( s_position, tc );
#else
	float4 P	= texelFetch( s_position, int2( pos2d ), int(iSample) );
#endif

	// 3d view space pos reconstruction math
	// center of the plane (0,0) or (0.5,0.5) at distance 1 is eyepoint(0,0,0) + lookat (assuming |lookat| ==1
	// left/right = (0,0,1) -/+ tan(fHorzFOV/2) * (1,0,0 ) 
	// top/bottom = (0,0,1) +/- tan(fVertFOV/2) * (0,1,0 )
	// lefttop		= ( -tan(fHorzFOV/2),  tan(fVertFOV/2), 1 )
	// righttop		= (  tan(fHorzFOV/2),  tan(fVertFOV/2), 1 )
	// leftbottom   = ( -tan(fHorzFOV/2), -tan(fVertFOV/2), 1 )
	// rightbottom	= (  tan(fHorzFOV/2), -tan(fVertFOV/2), 1 )
	gbd.P  = float3( P.z * ( pos2d.xy * pos_decompression_params.zw - pos_decompression_params.xy ), P.z );

	// reconstruct N
	gbd.N = gbuf_unpack_normal( P.xy );

	// reconstruct material
	gbd.mtl	= gbuf_unpack_mtl( P.w );

   // reconstruct hemi
   gbd.hemi = gbuf_unpack_hemi( P.w );

#ifndef USE_MSAA
   float4	C	= tex2D( s_diffuse, tc );
#else
   float4	C	= texelFetch( s_diffuse, int2( pos2d ), int(iSample) );
#endif

	gbd.C		= C.xyz;
	gbd.gloss	= C.w;

	return gbd;
}

gbuffer_data gbuffer_load_data( float2 tc, float4 pos2d )
{
   return gbuffer_load_data( tc, pos2d, 0 );
}

gbuffer_data gbuffer_load_data_offset( float2 tc, float2 OffsetTC, float4 pos2d )
{
	float4  delta	  = float4( ( OffsetTC - tc ) * pos_decompression_params2.xy, 0, 0 );

	return gbuffer_load_data( OffsetTC, pos2d + delta, 0 );
}

gbuffer_data gbuffer_load_data_offset( float2 tc, float2 OffsetTC, float4 pos2d, uint iSample )
{
   float4  delta	  = float4( ( OffsetTC - tc ) * pos_decompression_params2.xy, 0, 0 );

   return gbuffer_load_data( OffsetTC, pos2d + delta, iSample );
}

#else // GBUFFER_OPTIMIZATION
gbuffer_data gbuffer_load_data( float2 tc, uint iSample )
{
	gbuffer_data gbd;

#ifndef USE_MSAA
	float4 P	= tex2D( s_position, tc );
#else
   float4 P		= texelFetch( s_position, int2( tc * pos_decompression_params2.xy ), int(iSample) );
#endif

	gbd.P		= P.xyz;
	gbd.mtl		= P.w;

#ifndef USE_MSAA
	float4 N	= tex2D( s_normal, tc );
#else
	float4 N	= texelFetch( s_normal, int2( tc * pos_decompression_params2.xy ), int(iSample) );
#endif

	gbd.N		= N.xyz;
	gbd.hemi	= N.w;

#ifndef USE_MSAA
	float4	C	= tex2D( s_diffuse, tc );
#else
	float4	C	= texelFetch( s_diffuse, int2( tc * pos_decompression_params2.xy ), int(iSample) );
#endif


	gbd.C		= C.xyz;
	gbd.gloss	= C.w;

	return gbd;
}

gbuffer_data gbuffer_load_data( float2 tc  )
{
   return gbuffer_load_data( tc, 0 );
}

gbuffer_data gbuffer_load_data_offset( float2 tc, float2 OffsetTC, uint iSample )
{
   return gbuffer_load_data( OffsetTC, iSample );
}

#endif // GBUFFER_OPTIMIZATION

//////////////////////////////////////////////////////////////////////////
//	Aplha to coverage code
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
		else if( alpha < 0.40 ) // only one bit set
			mask = 1;
	}
  else
  {
	if( alpha < 0.60 ) // 2 bits set => 1100 0110 0011 1001 1010 0101
	{
		mask = 3;
	}
	else if( alpha < 0.8 ) // 3 bits set => 1110 0111 1011 1101 
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
		else // only one bit set 0.2222
			mask = 1;
	}
	else 
	{
		if( alpha < 0.3333 ) // 2 bits set0=> 10000001 + 11000000 .. 00000011 : 8 // 0.2222
		  				   //        set1=> 10100000 .. 00000101 + 10000010 + 01000001 : 8
						   //		set2=> 10010000 .. 00001001 + 10000100 + 01000010 + 00100001 : 8
						   //		set3=> 10001000 .. 00010001 + 10001000 + 01000100 + 00100010 + 00010001 : 8
		{  
			mask = 3;
		}
	    else // 3 bits set0 => 11100000 .. 00000111 + 10000011 + 11000001 : 8 ? 0.4444 // 0.3333
			 //        set1 => 10110000 .. 00001011 + 10000101 + 11000010 + 01100001: 8
			 //        set2 => 11010000 .. 00001101 + 10000110 + 01000011 + 10100001: 8
			 //        set3 => 10011000 .. 00010011 + 10001001 + 11000100 + 01100010 + 00110001 : 8
			 //        set4 => 11001000 .. 00011001 + 10001100 + 01000110 + 00100011 + 10010001 : 8
		{
			mask = 0x7;
		}
	}
  }
  else
  {
	  if( alpha < 0.6666 )
	  {
		if( alpha < 0.5555 ) // 4 bits set0 => 11110000 .. 00001111 + 10000111 + 11000011 + 11100001 : 8 // 0.5555
		 				   //        set1 => 11011000 .. 00011011 + 10001101 + 11000110 + 01100011 + 10110001 : 8
						   //        set2 => 11001100 .. 00110011 + 10011001 : 4 make 8
						   //        set3 => 11000110 + 01100011 + 10110001 + 11011000 + 01101100 + 00110110 + 00011011 + 10001101 : 8
						   //        set4 => 10111000 .. 00010111 + 10001011 + 11000101 + 11100010 + 01110001 : 8
						   //        set5 => 10011100 .. 00100111 + 10010011 + 11001001 + 11100100 + 01110010 + 00111001 : 8
						   //        set6 => 10101010 .. 01010101 : 2 make 8
						   //        set7 => 10110100 +  01011010 + 00101101 + 10010110 + 01001011 + 10100101 + 11010010 + 01101001 : 8
						   //        set8 => 10011010 +  01001101 + 10100110 + 01010011 + 10101001 + 11010100 + 01101010 + 00110101 : 8
		{
			mask = 0xf;
		}
		else // 5 bits set0 => 11111000 01111100 00111110 00011111 10001111 11000111 11100011 11110001 : 8  // 0.6666
		     //        set1 => 10111100 : 8
		     //        set2 => 10011110 : 8
		     //        set3 => 11011100 : 8
		     //        set4 => 11001110 : 8
		     //        set5 => 11011010 : 8
		     //        set6 => 10110110 : 8
		{
			mask = 0x1F;
		}
	  }
	  else
	  {
		if( alpha < 0.7777 ) // 6 bits set0 => 11111100 01111110 00111111 10011111 11001111 11100111 11110011 11111001 : 8
						  //        set1 => 10111110 : 8
						  //        set2 => 11011110 : 8
		{
			mask = 0x3F;
		}
		else if( alpha < 0.8888 ) // 7 bits set0 => 11111110 :8
		{
			mask = 0x7F;
		}
		else // all 8 bits set
			mask = 0xFF;
	 }
  }

	return mask;
}
#endif
#endif



#endif	//	common_functions_h_included
