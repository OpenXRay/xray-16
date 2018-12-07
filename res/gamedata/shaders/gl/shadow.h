#ifndef	SHADOW_H
#define SHADOW_H

#include "common.h"

//uniform	sampler	s_smap	: register(ps,s0);	// 2D/cube shadowmap
//Texture2D<float>	s_smap;		// 2D/cube shadowmap
//	Used for RGBA texture too ?!
Texture2D	s_dmap;			// 2D/cube depthmap
Texture2DShadow	s_smap;		// 2D/cube shadowmap

Texture2D	s_smap_minmax;	// 2D/cube shadowmap
#include "gather.ps"

//SamplerComparisonState		smp_smap;	//	Special comare sampler
//sampler		smp_jitter;

Texture2D	jitter0;
Texture2D	jitter1;
//uniform sampler2D	jitter2;
//uniform sampler2D	jitter3;
//uniform float4 		jitterS;

Texture2D	jitterMipped;

#ifndef USE_ULTRA_SHADOWS
#define	KERNEL	0.6
#else
#define	KERNEL	1.0
#endif

float modify_light( float light )
{
   return ( light > 0.7 ? 1.0 : lerp( 0.0, 1.0, saturate( light / 0.7 ) ) ); 
}

//////////////////////////////////////////////////////////////////////////////////////////
// hardware + PCF
//////////////////////////////////////////////////////////////////////////////////////////
float sample_hw_pcf (float4 tc,float4 shift)
{
	const float 	ts = KERNEL / float(SMAP_size);
	return tex2Dproj( s_smap, tc + tc.w * shift * ts );
}

#define GS2 3

float shadow_hw( float4 tc )
{
  	float	s0		= sample_hw_pcf( tc, float4( -1, -1, 0, 0) );
  	float	s1		= sample_hw_pcf( tc, float4( +1, -1, 0, 0) );
  	float	s2		= sample_hw_pcf( tc, float4( -1, +1, 0, 0) );
  	float	s3		= sample_hw_pcf( tc, float4( +1, +1, 0, 0) );

	return	(s0+s1+s2+s3)/4.0;
}

#if SUN_QUALITY>=4
#define FILTER_SIZE	11
#define FS  FILTER_SIZE
#define FS2 ( FILTER_SIZE / 2 )

const float W2[11][11] = 
                 { { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0 }, 
			       { 0.0,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.0 },
			       { 0.0,0.2,1.0,1.0,1.0,1.0,1.0,1.0,1.0,0.2,0.0 },
			       { 0.0,0.2,1.0,1.0,1.0,1.0,1.0,1.0,1.0,0.2,0.0 },
			       { 0.0,0.2,1.0,1.0,1.0,1.0,1.0,1.0,1.0,0.2,0.0 },
			       { 0.0,0.2,1.0,1.0,1.0,1.0,1.0,1.0,1.0,0.2,0.0 },
			       { 0.0,0.2,1.0,1.0,1.0,1.0,1.0,1.0,1.0,0.2,0.0 },
			       { 0.0,0.2,1.0,1.0,1.0,1.0,1.0,1.0,1.0,0.2,0.0 },
			       { 0.0,0.2,1.0,1.0,1.0,1.0,1.0,1.0,1.0,0.2,0.0 },
			       { 0.0,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.0 },
			       { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0 },
			       };

const float W1[11][11] = 
                 { { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0 }, 
			       { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0 },
			       { 0.0,0.0,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.0,0.0 },
			       { 0.0,0.0,0.2,1.0,1.0,1.0,1.0,1.0,0.2,0.0,0.0 },
			       { 0.0,0.0,0.2,1.0,1.0,1.0,1.0,1.0,0.2,0.0,0.0 },
			       { 0.0,0.0,0.2,1.0,1.0,1.0,1.0,1.0,0.2,0.0,0.0 },
			       { 0.0,0.0,0.2,1.0,1.0,1.0,1.0,1.0,0.2,0.0,0.0 },
			       { 0.0,0.0,0.2,1.0,1.0,1.0,1.0,1.0,0.2,0.0,0.0 },
			       { 0.0,0.0,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.0,0.0 },
			       { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0 },
			       { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0 },
			       };

const float W0[11][11] = 
                 { { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0 }, 
			       { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0 },
			       { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0 },
			       { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0 },
			       { 0.0,0.0,0.0,0.0,0.1,0.1,0.1,0.0,0.0,0.0,0.0 },
			       { 0.0,0.0,0.0,0.0,0.1,1.0,0.1,0.0,0.0,0.0,0.0 },
			       { 0.0,0.0,0.0,0.0,0.1,0.1,0.1,0.0,0.0,0.0,0.0 },
			       { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0 },
			       { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0 },
			       { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0 },
			       { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0 },
			       };

float Fw( int r, int c, float fL )
{
	return        (1.0-fL) * (1.0-fL) * (1.0-fL) * W0[r][c] +
	       3.0 * (1.0-fL) * (1.0-fL) *   fL     * W1[r][c] +
	       3.0 *    fL    *    fL    * (1.0-fL) * W2[r][c] +
                     fL    *    fL    *   fL     * 1.0;
} 

#define BLOCKER_FILTER_SIZE	11
#define BFS  BLOCKER_FILTER_SIZE
#define BFS2 ( BLOCKER_FILTER_SIZE / 2 )

#define SUN_WIDTH 300.0

// uses gather for DX11/10.1 and visibilty encoding for DX10.0
float shadow_extreme_quality( float3 tc )
{
   float  s   = 0.0;
   float2 stc = ( float( SMAP_size ) * tc.xy ) + float2( 0.5, 0.5 );
   float2 tcs = floor( stc );
   float2 fc;
   int    row;
   int    col;
   float  w = 0.0;
   float  avgBlockerDepth = 0;
   float  blockerCount = 0;
   float  fRatio;
   float4 v1[ FS2 + 1 ];
   float2 v0[ FS2 + 1 ];
   float2 off;

   fc     = stc - tcs;
   tc.xy  = tc.xy - ( 1.0 / float( SMAP_size ) * fc );
   tc.z  -= 0.0001;

#if defined(SM_4_1) || defined( SM_5) 
    // find number of blockers and sum up blocker depth
    for( row = -BFS2; row <= BFS2; row += 2 )
    {
        for( col = -BFS2; col <= BFS2; col += 2 )
        {
            float4 d4 = textureGatherOffset( s_smap, tc.xy, int2( col, row ) );
            float4 b4 = ( tc.zzzz <= d4 ) ? (0.0).xxxx : (1.0).xxxx;   
			
            blockerCount += dot( b4, (1.0).xxxx );
            avgBlockerDepth += dot( d4, b4 );
        }
    }
#else // SM_4_0
	uint vmask[ FS + 1 ];

    for( col = 0; col <= FS; ++col )
		vmask[ col ] = uint(0);
	
	for( row = -FS2; row <= FS2; row +=2 )
    {
        for( int col = -FS2; col <= FS2; col +=2 )
        {
            float4 d4;
            float  b;

			d4.w = textureLodOffset (s_dmap, tc.xy, 0, int2( col, row ) ).x;
			b = ( tc.z <= d4.w ) ? (0.0) : (1.0); 
			vmask[ col + FS2 + 0 ] += ( ( tc.z <= d4.w ) ? ( uint(1) << uint( row + FS2 + 0 ) ) : uint(0) );
			blockerCount     += b;
            avgBlockerDepth  += d4.w * b;
			
			d4.z = textureLodOffset (s_dmap, tc.xy, 0, int2( col+1, row ) ).x;
			b = ( tc.z <= d4.z ) ? (0.0) : (1.0); 
			vmask[ col + FS2 + 1 ] += ( ( tc.z <= d4.z ) ? ( uint(1) << uint( row + FS2 + 0 ) ) : uint(0) );
			blockerCount     += b;
            avgBlockerDepth  += d4.z * b;
			
			d4.x = textureLodOffset (s_dmap, tc.xy, 0, int2( col, row+1 ) ).x;
			vmask[ col + FS2 + 0 ] += ( ( tc.z <= d4.x ) ? ( uint(1) << uint( row + FS2 + 1 ) ) : uint(0) );
			b = ( tc.z <= d4.x ) ? (0.0) : (1.0); 
			blockerCount     += b;
            avgBlockerDepth  += d4.x * b;

			d4.y = textureLodOffset (s_dmap, tc.xy, 0, int2( col+1, row+1 ) ).x;
			vmask[ col + FS2 + 1 ] += ( ( tc.z <= d4.y ) ? ( uint(1) << uint( row + FS2 + 1 ) ) : uint(0) );
			b = ( tc.z <= d4.y ) ? (0.0) : (1.0); 
			blockerCount     += b;
            avgBlockerDepth  += d4.y * b;
        }
    }
#endif
   
   // compute ratio average blocker depth vs. pixel depth
   if( blockerCount > 0.0 )
   {
	   avgBlockerDepth /= blockerCount;
       fRatio = saturate( ( ( tc.z - avgBlockerDepth ) * SUN_WIDTH ) / avgBlockerDepth );
       fRatio *= fRatio;
   }
   else
   {
	   fRatio = 0.0; 
   }

   for( row = 0; row < FS; ++row )
   {
      for( col = 0; col < FS; ++col )
         w += Fw(row,col,fRatio);
   }

    // filter shadow map samples using the dynamic weights
    for( row = -FS2; row <= FS2; row += 2 )
    {
        for( int col = -FS2; col <= FS2; col += 2 )
        {
#if ( defined(SM_5) ) || ( defined(SM_4_1) )
            v1[(col+FS2)/2] = textureGatherOffset (s_smap, tc.xy, tc.z, 
                                                   int2( col, row ) );
#else
            v1[(col+FS2)/2].w = ( ( vmask[ col + FS2 + 0 ] & ( uint(1) << uint( row + FS2 + 0 ) ) ) ? 1.0 : 0.0 );
            v1[(col+FS2)/2].z = ( ( vmask[ col + FS2 + 1 ] & ( uint(1) << uint( row + FS2 + 0 ) ) ) ? 1.0 : 0.0 );
            v1[(col+FS2)/2].x = ( ( vmask[ col + FS2 + 0 ] & ( uint(1) << uint( row + FS2 + 1 ) ) ) ? 1.0 : 0.0 );
            v1[(col+FS2)/2].y = ( ( vmask[ col + FS2 + 1 ] & ( uint(1) << uint( row + FS2 + 1 ) ) ) ? 1.0 : 0.0 );
#endif
		  if( col == -FS2 )
		  {
			 s += ( 1 - fc.y ) * ( v1[0].w * ( Fw(row+FS2,0,fRatio) - Fw(row+FS2,0,fRatio) * fc.x ) + v1[0].z * ( fc.x * ( Fw(row+FS2,0,fRatio) - Fw(row+FS2,1,fRatio) ) +  Fw(row+FS2,1,fRatio) ) );
			 s += (     fc.y ) * ( v1[0].x * ( Fw(row+FS2,0,fRatio) - Fw(row+FS2,0,fRatio) * fc.x ) + v1[0].y * ( fc.x * ( Fw(row+FS2,0,fRatio) - Fw(row+FS2,1,fRatio) ) +  Fw(row+FS2,1,fRatio) ) );
			 if( row > -FS2 )
			 {
				s += ( 1 - fc.y ) * ( v0[0].x * ( Fw(row+FS2-1,0,fRatio) - Fw(row+FS2-1,0,fRatio) * fc.x ) + v0[0].y * ( fc.x * ( Fw(row+FS2-1,0,fRatio) - Fw(row+FS2-1,1,fRatio) ) +  Fw(row+FS2-1,1,fRatio) ) );
				s += (     fc.y ) * ( v1[0].w * ( Fw(row+FS2-1,0,fRatio) - Fw(row+FS2-1,0,fRatio) * fc.x ) + v1[0].z * ( fc.x * ( Fw(row+FS2-1,0,fRatio) - Fw(row+FS2-1,1,fRatio) ) +  Fw(row+FS2-1,1,fRatio) ) );
			 }
		  }
		  else if( col == FS2 )
		  {
			 s += ( 1 - fc.y ) * ( v1[FS2].w * ( fc.x * ( Fw(row+FS2,FS-2,fRatio) - Fw(row+FS2,FS-1,fRatio) ) + Fw(row+FS2,FS-1,fRatio) ) + v1[FS2].z * fc.x * Fw(row+FS2,FS-1,fRatio) );
			 s += (     fc.y ) * ( v1[FS2].x * ( fc.x * ( Fw(row+FS2,FS-2,fRatio) - Fw(row+FS2,FS-1,fRatio) ) + Fw(row+FS2,FS-1,fRatio) ) + v1[FS2].y * fc.x * Fw(row+FS2,FS-1,fRatio) );
			 if( row > -FS2 )
			 {
				s += ( 1 - fc.y ) * ( v0[FS2].x * ( fc.x * ( Fw(row+FS2-1,FS-2,fRatio) - Fw(row+FS2-1,FS-1,fRatio) ) + Fw(row+FS2-1,FS-1,fRatio) ) + v0[FS2].y * fc.x * Fw(row+FS2-1,FS-1,fRatio) );
				s += (     fc.y ) * ( v1[FS2].w * ( fc.x * ( Fw(row+FS2-1,FS-2,fRatio) - Fw(row+FS2-1,FS-1,fRatio) ) + Fw(row+FS2-1,FS-1,fRatio) ) + v1[FS2].z * fc.x * Fw(row+FS2-1,FS-1,fRatio) );
			 }
		  }
		  else
		  {
			 s += ( 1 - fc.y ) * ( v1[(col+FS2)/2].w * ( fc.x * ( Fw(row+FS2,col+FS2-1,fRatio) - Fw(row+FS2,col+FS2+0,fRatio) ) + Fw(row+FS2,col+FS2+0,fRatio) ) +
						           v1[(col+FS2)/2].z * ( fc.x * ( Fw(row+FS2,col+FS2-0,fRatio) - Fw(row+FS2,col+FS2+1,fRatio) ) + Fw(row+FS2,col+FS2+1,fRatio) ) );
			 s += (     fc.y ) * ( v1[(col+FS2)/2].x * ( fc.x * ( Fw(row+FS2,col+FS2-1,fRatio) - Fw(row+FS2,col+FS2+0,fRatio) ) + Fw(row+FS2,col+FS2+0,fRatio) ) +
						           v1[(col+FS2)/2].y * ( fc.x * ( Fw(row+FS2,col+FS2-0,fRatio) - Fw(row+FS2,col+FS2+1,fRatio) ) + Fw(row+FS2,col+FS2+1,fRatio) ) );
			 if( row > -FS2 )
			 {
				s += ( 1 - fc.y ) * ( v0[(col+FS2)/2].x * ( fc.x * ( Fw(row+FS2-1,col+FS2-1,fRatio) - Fw(row+FS2-1,col+FS2+0,fRatio) ) + Fw(row+FS2-1,col+FS2+0,fRatio) ) +
							          v0[(col+FS2)/2].y * ( fc.x * ( Fw(row+FS2-1,col+FS2-0,fRatio) - Fw(row+FS2-1,col+FS2+1,fRatio) ) + Fw(row+FS2-1,col+FS2+1,fRatio) ) );
				s += (     fc.y ) * ( v1[(col+FS2)/2].w * ( fc.x * ( Fw(row+FS2-1,col+FS2-1,fRatio) - Fw(row+FS2-1,col+FS2+0,fRatio) ) + Fw(row+FS2-1,col+FS2+0,fRatio) ) +
							          v1[(col+FS2)/2].z * ( fc.x * ( Fw(row+FS2-1,col+FS2-0,fRatio) - Fw(row+FS2-1,col+FS2+1,fRatio) ) + Fw(row+FS2-1,col+FS2+1,fRatio) ) );
			 }
	      }
		  if( row != FS2 )
			v0[(col+FS2)/2] = v1[(col+FS2)/2].xy;
	   }
   }

   return s/w;
}

float4 Fw( int r, int c )
{
	return float4( W0[r][c], W1[r][c], W2[r][c], 1.0 );
}

//======================================================================================
// This shader computes the contact hardening shadow filter
//======================================================================================
float shadow_extreme_quality_fused( float3 tc )
{
    float4 s   = (0.0).xxxx;
    float2 stc = ( float(SMAP_size) * tc.xy ) + float2( 0.5, 0.5 );
    float2 tcs = floor( stc );
    float2 fc;
    int    row;
    int    col;
    float  w = 0.0;
    float  avgBlockerDepth = 0;
    float  blockerCount = 0;
    float  fRatio;
    float4 v1[ FS2 + 1 ];
    float2 v0[ FS2 + 1 ];
    float2 off;

    fc     = stc - tcs;
    tc.xy  = tc.xy - ( fc * (1.0/float(SMAP_size)) );

    // filter shadow map samples using the dynamic weights
    for( row = -FS2; row <= FS2; row += 2 )
    {
        for( col = -FS2; col <= FS2; col += 2 )
        {
            float4 d4;
            
#ifndef PS_4            
            d4 = textureGather( s_dmap, tc.xy + (1.0/float(SMAP_size)) * float2( col, row ) );
#else
			d4.w = textureLod( s_dmap, tc.xy + (1.0/float(SMAP_size)) * float2( col, row ), 0 ).x;
			d4.z = textureLod( s_dmap, tc.xy + (1.0/float(SMAP_size)) * float2( col+1, row ) , 0 ).x;
			d4.y = textureLod( s_dmap, tc.xy + (1.0/float(SMAP_size)) * float2( col+1, row+1 ), 0 ).x;
			d4.x = textureLod( s_dmap, tc.xy + (1.0/float(SMAP_size)) * float2( col, row+1 ), 0 ).x;
#endif
            float4 b4  = ( tc.zzzz <= d4 ) ? (0.0).xxxx : (1.0).xxxx;   

            v1[(col+FS2)/2] = ( tc.zzzz <= d4 ) ? (1.0).xxxx : (0.0).xxxx;
            blockerCount += dot( b4, (1.0).xxxx );
            avgBlockerDepth += dot( d4, b4 );
          
            if( col == -FS2 )
            {
                s += ( 1 - fc.y ) * ( v1[0].w * ( Fw(row+FS2,0) - 
                                      Fw(row+FS2,0) * fc.x ) + v1[0].z * 
                                    ( fc.x * ( Fw(row+FS2,0) - 
                                      Fw(row+FS2,1) ) +  
                                      Fw(row+FS2,1) ) );
                s += (     fc.y ) * ( v1[0].x * ( Fw(row+FS2,0) - 
                                      Fw(row+FS2,0) * fc.x ) + 
                                      v1[0].y * ( fc.x * ( Fw(row+FS2,0) - 
                                      Fw(row+FS2,1) ) +  
                                      Fw(row+FS2,1) ) );
                if( row > -FS2 )
                {
                    s += ( 1 - fc.y ) * ( v0[0].x * ( Fw(row+FS2-1,0) - 
                                          Fw(row+FS2-1,0) * fc.x ) + v0[0].y * 
                                        ( fc.x * ( Fw(row+FS2-1,0) - 
                                          Fw(row+FS2-1,1) ) +  
                                          Fw(row+FS2-1,1) ) );
                    s += (     fc.y ) * ( v1[0].w * ( Fw(row+FS2-1,0) - 
                                          Fw(row+FS2-1,0) * fc.x ) + v1[0].z * 
                                        ( fc.x * ( Fw(row+FS2-1,0) - 
                                          Fw(row+FS2-1,1) ) +  
                                          Fw(row+FS2-1,1) ) );
                }
            }
            else if( col == FS2 )
            {
                s += ( 1 - fc.y ) * ( v1[FS2].w * ( fc.x * ( Fw(row+FS2,FS-2) - 
                                      Fw(row+FS2,FS-1) ) + 
                                      Fw(row+FS2,FS-1) ) + v1[FS2].z * fc.x * 
                                      Fw(row+FS2,FS-1) );
                s += (     fc.y ) * ( v1[FS2].x * ( fc.x * ( Fw(row+FS2,FS-2) - 
                                      Fw(row+FS2,FS-1) ) + 
                                      Fw(row+FS2,FS-1) ) + v1[FS2].y * fc.x * 
                                      Fw(row+FS2,FS-1) );
                if( row > -FS2 )
                {
                    s += ( 1 - fc.y ) * ( v0[FS2].x * ( fc.x * 
                                        ( Fw(row+FS2-1,FS-2) - 
                                          Fw(row+FS2-1,FS-1) ) + 
                                          Fw(row+FS2-1,FS-1) ) + 
                                          v0[FS2].y * fc.x * Fw(row+FS2-1,FS-1) );
                    s += (     fc.y ) * ( v1[FS2].w * ( fc.x * 
                                        ( Fw(row+FS2-1,FS-2) - 
                                          Fw(row+FS2-1,FS-1) ) + 
                                          Fw(row+FS2-1,FS-1) ) + 
                                          v1[FS2].z * fc.x * Fw(row+FS2-1,FS-1) );
                }
            }
            else
            {
                s += ( 1 - fc.y ) * ( v1[(col+FS2)/2].w * ( fc.x * 
                                    ( Fw(row+FS2,col+FS2-1) - 
                                      Fw(row+FS2,col+FS2+0) ) + 
                                      Fw(row+FS2,col+FS2+0) ) +
                                      v1[(col+FS2)/2].z * ( fc.x * 
                                    ( Fw(row+FS2,col+FS2-0) - 
                                      Fw(row+FS2,col+FS2+1) ) + 
                                      Fw(row+FS2,col+FS2+1) ) );
                s += (     fc.y ) * ( v1[(col+FS2)/2].x * ( fc.x * 
                                    ( Fw(row+FS2,col+FS2-1) - 
                                      Fw(row+FS2,col+FS2+0) ) + 
                                      Fw(row+FS2,col+FS2+0) ) +
                                      v1[(col+FS2)/2].y * ( fc.x * 
                                    ( Fw(row+FS2,col+FS2-0) - 
                                      Fw(row+FS2,col+FS2+1) ) + 
                                      Fw(row+FS2,col+FS2+1) ) );
                if( row > -FS2 )
                {
                    s += ( 1 - fc.y ) * ( v0[(col+FS2)/2].x * ( fc.x * 
                                        ( Fw(row+FS2-1,col+FS2-1) - 
                                          Fw(row+FS2-1,col+FS2+0) ) + 
                                          Fw(row+FS2-1,col+FS2+0) ) +
                                          v0[(col+FS2)/2].y * ( fc.x * 
                                        ( Fw(row+FS2-1,col+FS2-0) - 
                                          Fw(row+FS2-1,col+FS2+1) ) + 
                                          Fw(row+FS2-1,col+FS2+1) ) );
                    s += (     fc.y ) * ( v1[(col+FS2)/2].w * ( fc.x * 
                                        ( Fw(row+FS2-1,col+FS2-1) - 
                                          Fw(row+FS2-1,col+FS2+0) ) + 
                                          Fw(row+FS2-1,col+FS2+0) ) +
                                          v1[(col+FS2)/2].z * ( fc.x * 
                                        ( Fw(row+FS2-1,col+FS2-0) - 
                                          Fw(row+FS2-1,col+FS2+1) ) + 
                                          Fw(row+FS2-1,col+FS2+1) ) );
                }
            }
            
            if( row != FS2 )
            {
                v0[(col+FS2)/2] = v1[(col+FS2)/2].xy;
            }
        }
    }

    // compute ratio using formulas from PCSS
    if( blockerCount > 0.0 )
    {
        avgBlockerDepth /= blockerCount;
        fRatio = saturate( ( ( tc.z - avgBlockerDepth ) * SUN_WIDTH ) / avgBlockerDepth );
        fRatio *= fRatio;
    }
    else
    {
        fRatio = 0.0; 
    }

    // sum up weights of dynamic filter matrix
    for( row = 0; row < FS; ++row )
    {
       for( col = 0; col < FS; ++col )
       {
          w += Fw(row,col,fRatio);
       }
    }

    return dot(s, float4((1.0-fRatio)*(1.0-fRatio)*(1.0-fRatio),
						 3.0 * (1.0-fRatio)*(1.0-fRatio)*fRatio,
						 3.0 * fRatio*fRatio*(1.0-fRatio),
						 fRatio*fRatio*fRatio ) )/w;
}
#endif

#ifdef SM_4_1

float dx10_1_hw_hq_7x7( float3 tc )
{
   float  s = 0.0;
   float2 stc = ( float(SMAP_size) * tc.xy ) + float2( 0.5, 0.5 );
   float2 tcs = floor( stc );
   float2 fc;
   int    row;
   int    col;

   fc.xy = stc - tcs;
   tc.xy = tcs * ( 1.0 / float(SMAP_size) );
   
   // loop over the rows
   for( row = -GS2; row <= GS2; row += 2 )
   {
       for( col = -GS2; col <= GS2; col += 2 )
       {
            float4 v = mask( lessThanEqual( tc.zzzz, textureGatherOffset( s_dmap, tc.xy, int2( col, row ) ) ), float4(1.0), float4(0.0)); 
            
            if( row == -GS2 ) // top row
            {
                if( col == -GS2 ) // left
                    s += dot( float4( 1.0-fc.x, 1.0, 1.0-fc.y, (1.0-fc.x)*(1.0-fc.y) ), v );
                else if( col == GS2 ) // right
                    s += dot( float4( 1.0, fc.x, fc.x*(1.0-fc.y), 1.0-fc.y ), v );
                else // center
                    s += dot( float4( 1.0, 1.0, 1.0-fc.y, 1.0-fc.y ), v );
            }
            else if( row == GS2 )  // bottom row
            {
                if( col == -GS2 ) // left
                    s += dot( float4( (1.0-fc.x)*fc.y, fc.y, 1.0, (1.0-fc.x) ), v );
                else if( col == GS2 ) // right
                    s += dot( float4( fc.y, fc.x*fc.y, fc.x, 1.0 ), v );
                else // center
                    s += dot( float4(fc.yy,1.0,1.0), v );
            }
            else // center rows
            {
                if( col == -GS2 ) // left
                    s += dot( float4( (1.0-fc.x), 1.0, 1.0, (1.0-fc.x) ), v ); 
                else if( col == GS2 ) // right
                    s += dot( float4( 1.0, fc.x, fc.x, 1.0 ), v ); 
                else // center
                    s += dot( float4(1.0), v ); 
            }
        }
   }
  
   return s*(1.0/49.0);
}

#endif

float dx10_0_hw_hq_7x7( float4 tc )
{
   tc.xyz /= tc.w;

   float  s   = 0.0;
   float2 stc = ( float(SMAP_size) * tc.xy ) + float2( 0.5, 0.5 );
   float2 tcs = floor( stc );
   float2 fc;

   fc    = stc - tcs;
   tc.xy = tc.xy - ( fc * ( 1.0/float(SMAP_size) ) );

   float2 pwAB = ( float2( 2.0 ) - fc ); 	
   float2 tcAB = float2( 1.0/float(SMAP_size) ) / pwAB;
   float2 tcM  = float2(0.5/float(SMAP_size) );
   float2 pwGH = ( float2( 1.0 ) + fc );
   float2 tcGH = (1.0/float(SMAP_size)) * ( fc / pwGH );

   // top row
   s += ( pwAB.x * pwAB.y ) * textureOffset( s_smap, float3(tc.xy + tcAB, tc.z), int2( -3, -3 ) ); // left
   s += (    2.0 * pwAB.y ) * textureOffset( s_smap, float3(tc.xy + float2( tcM.x, tcAB.y), tc.z), int2( -3, -1 ) );
   s += (    2.0 * pwAB.y ) * textureOffset( s_smap, float3(tc.xy + float2( tcM.x, tcAB.y), tc.z), int2( -3, 1 ) );
   s += ( pwGH.x * pwAB.y ) * textureOffset( s_smap, float3(tc.xy + float2( tcGH.x, tcAB.y), tc.z), int2( -3, 3 ) ); // right

   // center rows
   s += ( pwAB.x * 2.0    ) * textureOffset( s_smap, float3(tc.xy + float2( tcAB.x, tcM.y ), tc.z), int2( -1, -3 ) );
   s += (    2.0 * 2.0    ) * textureOffset( s_smap, float3(tc.xy + tcM, tc.z), int2( -1, -1 ) );
   s += (    2.0 * 2.0    ) * textureOffset( s_smap, float3(tc.xy + tcM, tc.z), int2( -1, 1 ) );
   s += ( pwGH.x * 2.0    ) * textureOffset( s_smap, float3(tc.xy + float2( tcGH.x, tcM.y),  tc.z), int2( -1, 3 ) );
   s += ( pwAB.x * 2.0    ) * textureOffset( s_smap, float3(tc.xy + float2( tcAB.x, tcM.y ), tc.z), int2( 1, -3 ) );
   s += (    2.0 * 2.0    ) * textureOffset( s_smap, float3(tc.xy + tcM, tc.z), int2( 1, -1 ) );
   s += (    2.0 * 2.0    ) * textureOffset( s_smap, float3(tc.xy + tcM, tc.z), int2( 1, 1 ) );
   s += ( pwGH.x * 2.0    ) * textureOffset( s_smap, float3(tc.xy + float2( tcGH.x, tcM.y),  tc.z), int2( 1, 3 ) );

   // bottom row
   s += ( pwAB.x * pwGH.y ) * textureOffset( s_smap, float3(tc.xy + float2( tcAB.x, tcGH.y ), tc.z), int2( 3, -3 ) );
   s += (    2.0 * pwGH.y ) * textureOffset( s_smap, float3(tc.xy + float2( tcM.x, tcGH.y ), tc.z), int2( 3, -1 ) );
   s += (    2.0 * pwGH.y ) * textureOffset( s_smap, float3(tc.xy + float2( tcM.x, tcGH.y ), tc.z), int2( 3, 1 ) );
   s += ( pwGH.x * pwGH.y ) * textureOffset( s_smap, float3(tc.xy + tcGH, tc.z), int2( 3, 3 ) );

   return s/49.0;
}

#ifdef SM_MINMAX
bool cheap_reject( float3 tc, inout bool full_light ) 
{
   float4 plane0  = sm_minmax_gather( tc.xy, int2( -1,-1 ) );
   float4 plane1  = sm_minmax_gather( tc.xy, int2(  1,-1 ) );
   float4 plane2  = sm_minmax_gather( tc.xy, int2( -1, 1 ) );
   float4 plane3  = sm_minmax_gather( tc.xy, int2(  1, 1 ) );
   bool plane     = all( greaterThanEqual( plane0, float4(0.0) )) && all(greaterThanEqual( plane1, float4(0.0) )) && all(greaterThanEqual( plane2, float4(0.0) )) && all(greaterThanEqual( plane3, float4(0.0) ) );

   if( !plane ) // if there are no proper plane equations in the support region
   {
      bool no_plane  = all(lessThan( plane0, float4(0.0) )) && all(lessThan( plane1, float4(0.0) )) && all(lessThan( plane2, float4(0.0) )) && all(lessThan( plane3, float4(0.0) ) );
      float4 z       = float4( tc.z - 0.0005 );
      bool reject    = all( greaterThan( z, -plane0 )) && all(greaterThan( z, -plane1 )) && all(greaterThan( z, -plane2 )) && all(greaterThan( z, -plane3 ) );
      if( no_plane && reject )
      {
         full_light = false;
         return true;
      }
      else
      {
         return false;
      }
   }
   else // plane equation detected
   {
      // compute corrected z for texel pos
      const float scale = float( SMAP_size ) / 4.0;
      float2 fc  = frac( tc.xy * scale );
      float  z   = lerp( lerp( plane0.y, plane1.x, fc.x ), lerp( plane2.z, plane3.w, fc.x ), fc.y );

      // do minmax test with new z
      full_light = ( ( tc.z - 0.0001 ) <= z );

      return true; 
   }
}

#endif	//	SM_MINMAX

float shadow_hw_hq( float4 tc )
{
#ifdef SM_MINMAX
   bool   full_light = false; 
   bool   cheap_path = cheap_reject( tc.xyz / tc.w, full_light );

   if( cheap_path )
   {
      if( full_light == true )
         return 1.0;
      else
         return sample_hw_pcf( tc, float4(0.0) ); 
   }
   else
   {
#if SUN_QUALITY>=4 // extreme quality
      return shadow_extreme_quality( tc.xyz / tc.w );
#else // SUN_QUALITY<4
#ifdef SM_4_1
      return dx10_1_hw_hq_7x7( tc.xyz / tc.w );
#else // SM_4_1
      return dx10_0_hw_hq_7x7( tc ); 
#endif // SM_4_1
#endif //SUN_QUALITY==4
   }
#else //	SM_MINMAX
#if SUN_QUALITY>=4 // extreme quality
      return shadow_extreme_quality( tc.xyz / tc.w );
#else // SUN_QUALITY<4
#ifdef SM_4_1
      return dx10_1_hw_hq_7x7( tc.xyz / tc.w );
#else // SM_4_1
      return dx10_0_hw_hq_7x7( tc ); 
#endif // SM_4_1
#endif //SUN_QUALITY==4
#endif //	SM_MINMAX
}

//////////////////////////////////////////////////////////////////////////////////////////
//	D24X8+PCF
//////////////////////////////////////////////////////////////////////////////////////////

float4 	test 		(float4 tc, float2 offset)
{

	float4	tcx	= float4 (tc.xy + tc.w*offset, tc.zw);
	return 	float4(tex2Dproj (s_smap,tcx));
}

/*half 	shadowtest_sun 	(float4 tc, float4 tcJ)			// jittered sampling
{
	half4	r;

	const 	float 	scale 	= (0.5/float(SMAP_size));

	float  	texsize = 2.0*float(SMAP_size);
	float2 	tc_J	= tc.xy/tc.w*texsize/8.0;
	float2 	fr 		= frac(tc_J)*0.5;
	
//	half4	J0 	= tex2D	(jitter0,fr)*scale;
//	half4	J1 	= tex2D	(jitter1,fr)*scale*2;
	float4	J0 	= jitter0.Sample( smp_jitter, fr )*scale;
//	float4	J1 	= jitter1.Sample( smp_jitter, fr )*scale;

	float k = 0.99/float(SMAP_size);
	r.x 	= test 	(tc,J0.xy+float2(-k,-k)).x;
	r.y 	= test 	(tc,J0.wz+float2( k,-k)).y;
	
 	r.z		= test	(tc,J0.xy+float2(-k, k)).z;
 	r.w		= test	(tc,J0.wz+float2( k, k)).x;
	
	half4	f;
	float k1 = 1.5/float(SMAP_size);
	f.x 	= test 	(tc,-J0.xy+float2(-k1,0)).x;
	f.y 	= test 	(tc,-J0.wz+float2( 0,-k1)).y;

	f.z		= test	(tc,-J0.xy+float2( k1, 0)).z;
 	f.w		= test	(tc,-J0.wz+float2( 0, k1)).x;

	half res = ( r.x + r.y + r.z + r.w + f.x + f.y + f.z + f.w )*1.0/(4.0 + 4.0 );
	return res;
}*/
half 	shadowtest_sun 	(float4 tc, float4 tcJ)			// jittered sampling
{
	half4	r;

	//	const 	float 	scale 	= (2.0/float(SMAP_size));
	const 	float 	scale 	= (0.7/float(SMAP_size));


	float2 	tc_J	= frac(tc.xy/tc.w*float(SMAP_size)/4.0 )*0.5;
	float4	J0		= tex2D	(jitter0,tc_J)*scale;
	//half4	J1 		= tex2D	(jitter1,tc_J)*scale;

	const float k = 0.5/float(SMAP_size);
	r.x 	= test 	(tc, J0.xy+half2(-k,-k)).x;
	r.y 	= test 	(tc, J0.wz+half2( k,-k)).y;
	r.z		= test	(tc,-J0.xy+half2(-k, k)).z;
	r.w		= test	(tc,-J0.wz+half2( k, k)).x;

	return	dot(r,float4(1.0/4.0));
}

half 	shadow_high 	(float4 tc)			// jittered sampling
{

	const	float 	scale 	= (0.5/float(SMAP_size));

	float2 	tc_J	= frac(tc.xy/tc.w*float(SMAP_size)/4.0 )*0.5;
	float4	J0 		= tex2D	(jitter0,tc_J)*scale;

	const float k = 1.0/float(SMAP_size);
	half4	r;
	r.x 	= test 	(tc,J0.xy+half2(-k,-k)).x;
	r.y 	= test 	(tc,J0.wz+half2( k,-k)).y;

	r.z		= test	(tc,J0.xy+half2(-k, k)).z;
	r.w		= test	(tc,J0.wz+half2( k, k)).x;


	const float k1 = 1.3/float(SMAP_size);
	half4	r1;
	r1.x 	= test 	(tc,-J0.xy+half2(-k1,0)).x;
	r1.y 	= test 	(tc,-J0.wz+half2( 0,-k1)).y;

	r1.z	= test	(tc,-2*J0.xy+half2( k1, 0)).z;
	r1.w	= test	(tc,-2*J0.wz+half2( 0, k1)).x;

	return ( r.x + r.y + r.z + r.w + r1.x + r1.y + r1.z + r1.w )*1.0/8.0;
}

float shadow( float4 tc ) 
{
#ifdef USE_ULTRA_SHADOWS
#	ifdef SM_MINMAX
		return modify_light( shadow_hw_hq( tc ) ); 
#	else
		return shadow_hw_hq( tc ); 
#	endif
#else
#	if SUN_QUALITY>=2 // Hight quality
		//return shadowtest_sun 	( tc, float4(0,0,0,0) );			// jittered sampling;
		return shadow_hw		(tc);
#	else
		return shadow_hw		(tc);
#	endif
#endif
}

float shadow_volumetric( float4 tc ) 
{
	return sample_hw_pcf	(tc,float4(-1,-1,0,0)); 
}


#ifdef SM_MINMAX

//////////////////////////////////////////////////////////////////////////////////////////
// hardware + PCF
//////////////////////////////////////////////////////////////////////////////////////////

float shadow_dx10_1( float4 tc, float2 tcJ, float2 pos2d ) 
{
   return shadow( tc ); 
}

float shadow_dx10_1_sunshafts( float4 tc, float2 pos2d ) 
{
   float3 t         = tc.xyz / tc.w;
   float minmax     = textureLod( s_smap_minmax, t.xy, 0 ).x;
   bool   umbra     = ( ( minmax < 0 ) && ( t.z > -minmax ) );

   if( umbra )
   {
      return 0.0;
   }
   else
   {
      return shadow_hw( tc ); 
   }
}

#endif


//////////////////////////////////////////////////////////////////////////////////////////
// testbed

//uniform sampler2D	jitter0;
//uniform sampler2D	jitter1;
float 	shadowtest 	(float4 tc, float4 tcJ)				// jittered sampling
{
	float4	r;

	const 	float 	scale 	= (2.7/float(SMAP_size));

	float4	J0 	= tex2Dproj	(jitter0,tcJ)*scale;
	float4	J1 	= tex2Dproj	(jitter1,tcJ)*scale;

		r.x 	= test 	(tc,J0.xy).x;
		r.y 	= test 	(tc,J0.wz).y;
		r.z		= test	(tc,J1.xy).z;
		r.w		= test	(tc,J1.wz).x;

	return	dot(r,float4(1.0/4.0));
}

float 	shadow_rain 	(float4 tc, float2 tcJ)			// jittered sampling
{
	float4	r;

	const 	float 	scale 	= (4.0/float(SMAP_size));
//	float4	J0 	= jitter0.Sample( smp_jitter, tcJ )*scale;
//	float4	J1 	= jitter1.Sample( smp_jitter, tcJ )*scale;
	float4	J0 	= tex2D( jitter0, tcJ )*scale;
	float4	J1 	= tex2D( jitter1, tcJ )*scale;

	r.x 	= test 	(tc,J0.xy).x;
	r.y 	= test 	(tc,J0.wz).y;
	r.z		= test	(tc,J1.xy).z;
	r.w		= test	(tc,J1.wz).x;

//	float4	J0 	= jitterMipped.Sample( smp_base, tcJ )*scale;

//	r.x 	= test 	(tc,J0.xy).x;
//	r.y 	= test 	(tc,J0.wz).y;
//	r.z		= test	(tc,J0.yz).z;
//	r.w		= test	(tc,J0.xw).x;

	return	dot(r,float4(1.0/4.0));
}

//////////////////////////////////////////////////////////////////////////////////////////
#ifdef  USE_SUNMASK	
uniform float3x4 m_sunmask;	// ortho-projection
float sunmask( float4 P )
{
	float2 		tc	= mul( m_sunmask, P );		//
	return 		tex2D( s_lmap, tc ).w;			// A8 
}
#else
float sunmask( float4 P ) { return 1.0; }		// 
#endif
//////////////////////////////////////////////////////////////////////////////////////////
uniform float4x4	m_shadow;

#endif