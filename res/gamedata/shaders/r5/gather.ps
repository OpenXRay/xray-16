//#define SM_4_1

float4 sm_gather( float2 tc, int2 offset )
{
#ifdef SM_4_1
	return s_smap.Gather( smp_nofilter, tc, offset ); 
#else
    static const float scale = float( SMAP_size );
	float2 fc = frac( tc * scale );
	
	tc -= fc / scale;
	
	float s0 = s_smap.SampleLevel( smp_nofilter, tc, 0, offset + int2( 0, 1 ) );
	float s1 = s_smap.SampleLevel( smp_nofilter, tc, 0, offset + int2( 1, 1 ) );
	float s2 = s_smap.SampleLevel( smp_nofilter, tc, 0, offset + int2( 1, 0 ) );
	float s3 = s_smap.SampleLevel( smp_nofilter, tc, 0, offset + int2( 0, 0 ) );
	
	return float4( s0, s1, s2, s3 );
#endif
}

float4 sm_minmax_gather( float2 tc, int2 offset )
{
#ifdef SM_4_1
	return s_smap_minmax.Gather( smp_nofilter, tc, offset ); 
#else
    static const float scale = float( SMAP_size / 4 );
	float2 fc = frac( tc * scale );
	
	tc -= fc / scale;
	
	float s0 = s_smap_minmax.SampleLevel( smp_nofilter, tc, 0, offset + int2( 0, 1 ) ).x;
	float s1 = s_smap_minmax.SampleLevel( smp_nofilter, tc, 0, offset + int2( 1, 1 ) ).x;
	float s2 = s_smap_minmax.SampleLevel( smp_nofilter, tc, 0, offset + int2( 1, 0 ) ).x;
	float s3 = s_smap_minmax.SampleLevel( smp_nofilter, tc, 0, offset + int2( 0, 0 ) ).x;
	
	return float4( s0, s1, s2, s3 );
#endif
}