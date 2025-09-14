#include "common.h"
#define SM_MINMAX
#include "shadow.h"

#define PLANE_EPS 0.0001
#define S (1.0/3.0)

float main ( float2 tc0 : TEXCOORD0, float2 tcJ : TEXCOORD1, float4 col: COLOR, float4 pos2d : SV_POSITION ) : SV_Target
{
	float2 tc = pos2d.xy / ( SMAP_size / 4 );
	
	float4 s0 = sm_gather( tc, int2( -2, -2 ) );
	float minz = min( min( s0.x, s0.y ), min( s0.z, s0.w ) );
	float maxz = max( max( s0.x, s0.y ), max( s0.z, s0.w ) );
		
	float2 dh;
	float  topleft  = s0.w;

	float4 s1 = sm_gather( tc, int2( 0, -2 ) );
	minz = min( minz, min( min( s1.x, s1.y ), min( s1.z, s1.w ) ) );
	maxz = max( maxz, max( max( s1.x, s1.y ), max( s1.z, s1.w ) ) );

	float topright = s1.z;
	dh.x = topright - topleft;

	float4 s2 = sm_gather( tc, int2( -2, 0 ) );
	minz = min( minz, min( min( s2.x, s2.y ), min( s2.z, s2.w ) ) );
	maxz = max( maxz, max( max( s2.x, s2.y ), max( s2.z, s2.w ) ) );
	
	float bottomleft = s2.x;
	dh.y = bottomleft - topleft;
	
	// check rest of s0
	float4 h0 = (topleft).xxxx + float4( dot( float2( 0.0, S ), dh), dot(float2( S, S ), dh), dot(float2( S, 0.0 ), dh), 0 );
	float4 s0_on_plane = ( abs( s0 - h0 ) <= (PLANE_EPS).xxxx ); 

	// check rest of s1
	float4 h1 = (topleft).xxxx + float4( dot(float2( 2*S, S ), dh), dot(float2( 1, S ),dh), dh.x, dot(float2( 2*S,0 ), dh) );
	float4 s1_on_plane = ( abs( s1 - h1 ) <= (PLANE_EPS).xxxx ); 

	// check rest of s2
	float4 h2 = (topleft).xxxx + float4( dh.y, dot(float2( S, 1 ), dh), dot(float2( S, 2*S ), dh), dot(float2( 0,2*S ), dh) );
	float4 s2_on_plane = ( abs( s2 - h2 ) <= (PLANE_EPS).xxxx ); 

	float4 s3 = sm_gather( tc, int2( 0, 0 ) );
	minz = min( minz, min( min( s3.x, s3.y ), min( s3.z, s3.w ) ) );
	maxz = max( maxz, max( max( s3.x, s3.y ), max( s3.z, s3.w ) ) );
	
	// check s3
	float4 h3 = (topleft).xxxx + float4( dot(float2( 2*S, 1 ), dh), dot(float2( 1, 1 ), dh), dot(float2( 1, 2*S ), dh), dot(float2( 2*S,2*S ), dh) );
	float4 s3_on_plane = ( abs( s3 - h3 ) <= (PLANE_EPS).xxxx ); 

	float4 s4 = sm_gather( tc, int2( -2, 2 ) );
	minz = min( minz, min( min( s4.x, s4.y ), min( s4.z, s4.w ) ) );
	maxz = max( maxz, max( max( s4.x, s4.y ), max( s4.z, s4.w ) ) );

	// check s4
	float4 h4 = (topleft).xxxx + float4( dot(float2( 0,1+2*S ), dh),dot( float2( S, 1+2*S ), dh), dot(float2( S, 1+S ), dh), dot(float2( 0,1+S ), dh) );
	float4 s4_on_plane = ( abs( s4 - h4 ) <= (PLANE_EPS).xxxx ); 

	float4 s5 = sm_gather( tc, int2( 0, 2 ) );
	minz = min( minz, min( min( s5.x, s5.y ), min( s5.z, s5.w ) ) );
	maxz = max( maxz, max( max( s5.x, s5.y ), max( s5.z, s5.w ) ) );

	// check s5
	float4 h5 = (topleft).xxxx + float4( dot(float2( 2*S,1+2*S ), dh),dot( float2( 1, 1+2*S ), dh), dot(float2( 1, 1+S ), dh), dot(float2( 2*S,1+S ), dh) );
	float4 s5_on_plane = ( abs( s5 - h5 ) <= (PLANE_EPS).xxxx ); 

	float4 s6 = sm_gather( tc, int2( 2, 2 ) );
	minz = min( minz, min( min( s6.x, s6.y ), min( s6.z, s6.w ) ) );
	maxz = max( maxz, max( max( s6.x, s6.y ), max( s6.z, s6.w ) ) );

	// check s6
	float4 h6 = (topleft).xxxx + float4( dot(float2( 1+S,1+2*S ), dh), dot(float2( 1+2*S, 1+2*S ), dh), dot(float2( 1+2*S, 1+S ), dh), dot(float2( 1+S,1+S ), dh) );
	float4 s6_on_plane = ( abs( s6 - h6 ) <= (PLANE_EPS).xxxx ); 

	float4 s7 = sm_gather( tc, int2( 2, 0 ) );
	minz = min( minz, min( min( s7.x, s7.y ), min( s7.z, s7.w ) ) );
	maxz = max( maxz, max( max( s7.x, s7.y ), max( s7.z, s7.w ) ) );

	// check s7
	float4 h7 = (topleft).xxxx + float4( dot(float2( 1+S,1 ), dh),dot( float2( 1+2*S, 1 ), dh),dot( float2( 1+2*S, 2*S ), dh), dot(float2( 1+S,2*S ), dh ) );
	float4 s7_on_plane = ( abs( s7 - h7 ) <= (PLANE_EPS).xxxx ); 

	float4 s8 = sm_gather( tc, int2( 2, -2 ) );
	minz = min( minz, min( min( s8.x, s8.y ), min( s8.z, s8.w ) ) );
	maxz = max( maxz, max( max( s8.x, s8.y ), max( s8.z, s8.w ) ) );
	
	// check s8
	float4 h8 = (topleft).xxxx + float4( dot(float2( 1+S,S ), dh), dot(float2( 1+2*S, S ), dh), dot(float2( 1+2*S, 0 ), dh), dot(float2( 1+S,0 ), dh ) );
	float4 s8_on_plane = ( abs( s8 - h8 ) <= (PLANE_EPS).xxxx ); 

	float4 on_plane = s0_on_plane * s1_on_plane;
	on_plane *= s2_on_plane * s3_on_plane;
	on_plane *= s4_on_plane * s5_on_plane;
	on_plane *= s6_on_plane * s7_on_plane;
	on_plane *= s8_on_plane;
	on_plane.x *= on_plane.y * on_plane.z * on_plane.w; 
	
	//return ( on_plane.x != 0 ? float2( topleft, maxz ) : float2( -maxz, minz ) );
	//return ( on_plane.x != 0 ? topleft : ( -maxz ) );
	return ( on_plane.x != 0 ? s3.x : ( -maxz ) );
}
