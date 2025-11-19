#ifndef	DOF_H_INCLUDED
#define	DOF_H_INCLUDED

#ifndef	USE_DOF

float3	dof(float2 center)
{
	float3 	img 	= s_image.Sample( smp_rtlinear, center);
	return	img;
}

#else

float4	dof_params;
float3	dof_kernel;

float DOFFactor( float depth)
{
	float	dist_to_focus	= depth-dof_params.y;
	float 	blur_far	= saturate( dist_to_focus
					/ (dof_params.z-dof_params.y) );
	float 	blur_near	= saturate( dist_to_focus
					/ (dof_params.x-dof_params.y) );
	float 	blur 		= blur_near+blur_far;
	blur*=blur;
	return blur;
}


#define MAXCOF		7.f
#define EPSDEPTH	0.0001f
float3	dof(float2 center)
{
#ifndef USE_MSAA
   float 	depth		= s_position.Sample( smp_nofilter, center).z;
#else
   float 	depth		= s_position.Load( int3( center * pos_decompression_params2.xy ,0),0 ).z;
#endif
	if (depth <= EPSDEPTH)	depth = dof_params.w;
	float	blur 		= DOFFactor(depth);

	float2 	scale 	= float2	(.5f / 1024.f, .5f / 768.f) * (dof_kernel.z * blur);

	float2 	o  [12];
		o[0]	= float2(-0.326212f , -0.405810f)*scale;
		o[1] 	= float2(-0.840144f , -0.073580f)*scale;
		o[2] 	= float2(-0.695914f ,  0.457137f)*scale;
		o[3] 	= float2(-0.203345f ,  0.620716f)*scale;
		o[4] 	= float2( 0.962340f , -0.194983f)*scale;
		o[5] 	= float2( 0.473434f , -0.480026f)*scale;
		o[6] 	= float2( 0.519456f ,  0.767022f)*scale;
		o[7] 	= float2( 0.185461f , -0.893124f)*scale;
		o[8] 	= float2( 0.507431f ,  0.064425f)*scale;
		o[9] 	= float2( 0.896420f ,  0.412458f)*scale;
		o[10] 	= float2(-0.321940f , -0.932615f)*scale;
		o[11] 	= float2(-0.791559f , -0.597710f)*scale;

	float3	sum 	= s_image.Sample( smp_nofilter, center).xyz;
	float 	contrib	= 1.f;

   	[unroll] for (int i=0; i<12; i++)
	{
		float2 	tap 		= center + o[i];
		float4	tap_color	= s_image.Sample( smp_nofilter, tap );
#ifndef USE_MSAA
      float 	tap_depth 	= s_position.Sample( smp_nofilter, tap).z;
#else
      float 	tap_depth 	= s_position.Load( int3( tap* pos_decompression_params2.xy,0),0).z;
#endif
		[flatten] if (tap_depth <= EPSDEPTH)	tap_depth = dof_params.w;
		float 	tap_contrib	= DOFFactor(tap_depth);
		sum 		+= tap_color.xyz	* tap_contrib;
		contrib		+= tap_contrib;
	}

	return 	float3	(sum/contrib);
}

#endif

#endif