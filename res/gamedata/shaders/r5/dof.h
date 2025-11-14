#ifndef	DOF_H_INCLUDED
#define	DOF_H_INCLUDED

//#define USE_DOF

#ifndef	USE_DOF

float3	dof(float2 center)
{
//	float3 	img 	= tex2D		(s_image, center);
	float3 	img 	= s_image.Sample( smp_rtlinear, center);
	return	img;
}

#else	//	USE_DOF

// x - near y - focus z - far w - sky distance
float4	dof_params;
float3	dof_kernel;	// x,y - resolution pre-scaled z - just kernel size

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


//#define MAXCOF		5.h
#define MAXCOF		7.h
#define EPSDEPTH	0.0001h
float3	dof(float2 center)
{
	// Scale tap offsets based on render target size
//	float 	depth		= tex2D(s_position,center).z;
#ifndef USE_MSAA
   float 	depth		= s_position.Sample( smp_nofilter, center).z;
#else
   float 	depth		= s_position.Load( int3( center * pos_decompression_params2.xy ,0),0 ).z;
#endif
	if (depth <= EPSDEPTH)	depth = dof_params.w;
	float	blur 		= DOFFactor(depth);

	//float blur = 1;
	//	const amount of blur: define controlled
	//float2 	scale 	= float2	(.5f / 1024.h, .5f / 768.h) * MAXCOF * blur;
	//	const amount of blur: engine controlled
	float2 	scale 	= float2	(.5f / 1024.h, .5f / 768.h) * (dof_kernel.z * blur);
	//	amount of blur varies according to resolution
	//	but kernel size in pixels is fixed.
	//	float2 	scale 	= dof_kernel.xy * blur;

	// poisson
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

	// sample
//	float3	sum 	= tex2D(s_image,center);
	float3	sum 	= s_image.Sample( smp_nofilter, center);
	float 	contrib	= 1.h;

   	[unroll] for (int i=0; i<12; i++)
	{
		float2 	tap 		= center + o[i];
//		float4	tap_color	= tex2D	(s_image,tap);
//		float 	tap_depth 	= tex2D	(s_position,tap).z;
		//	TODO: DX10: test linear sampler
//		float4	tap_color	= s_image.Sample( smp_rtlinear, tap );
		float4	tap_color	= s_image.Sample( smp_nofilter, tap );
#ifndef USE_MSAA
      float 	tap_depth 	= s_position.Sample( smp_nofilter, tap).z;
#else
      float 	tap_depth 	= s_position.Load( int3( tap* pos_decompression_params2.xy,0),0).z;
#endif
		[flatten] if (tap_depth <= EPSDEPTH)	tap_depth = dof_params.w;
		float 	tap_contrib	= DOFFactor(tap_depth);
		sum 		+= tap_color	* tap_contrib;
		contrib		+= tap_contrib;
	}

	return 	float3	(sum/contrib);
}

/*
//	edge along sky line. More light-weight
float3	dof(float2 center)
{
	// Scale tap offsets based on render target size
	float 	depth		= tex2D(s_position,center).z;
//	if (depth <= EPSDEPTH)	depth = dof_params.w;
	if (depth <= EPSDEPTH)	depth = (dof_params.z-dof_params.y)*0.3;
	float	dist_to_focus	= depth-dof_params.y;
	float 	blur_far	= saturate( dist_to_focus
					/ (dof_params.z-dof_params.y) );
	float 	blur_near	= saturate( dist_to_focus
					/ (dof_params.x-dof_params.y) );
	float 	blur 		= (blur_near+blur_far);	
	blur*=blur;

	//float blur = 1;
	//	const amount of blur: define controlled
	//float2 	scale 	= float2	(.5f / 1024.h, .5f / 768.h) * MAXCOF * blur;
	//	const amount of blur: engine controlled
	float2 	scale 	= float2	(.5f / 1024.h, .5f / 768.h) * (dof_kernel.z * blur);
	//	amount of blur varies according to resolution
	//	but kernel size in pixels is fixed.
	//	float2 	scale 	= dof_kernel.xy * blur;

	// poisson
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

	// sample 
	float3	sum 	= tex2D(s_image,center);
	float 	contrib	= 1.h;
	for (int i=0; i<12; i++)
	{
		float2 	tap 		= center + o[i];
		float4	tap_color	= tex2D	(s_image,tap);
		float 	tap_depth 	= tex2D	(s_position,tap).z;
//		if (tap_depth <= EPSDEPTH)	tap_depth = dof_params.w;
		if (tap_depth <= EPSDEPTH)	tap_depth = (dof_params.z-dof_params.y)*0.3;
//		float 	tap_contrib	= 1.h;	//(tap_depth>depth)?1.h:0.h;
		float 	tap_contrib	= 1-saturate(abs(tap_depth-depth)/dist_to_focus);
			sum 		+= tap_color	* tap_contrib;
			contrib		+= tap_contrib;
	}
	return 	float3	(sum/contrib);
}
*/

/*
#define NEAR 		0.2h
//#define MINDIST 	0.4h
#define MINDIST 	1.4h
//#define MAXDIST 	100.h
//#define MAXDIST 	300.h
#define MAXDIST 	2.0h
#define MAXCOF		5.h
#define MAXCOF_NEAR	100.h
#define EPSDEPTH	0.0001h
float3	dof(float2 center)
{
	// Scale tap offsets based on render target size
	float 	depth		= tex2D(s_position,center).z;
	if (depth<=EPSDEPTH)	depth = MAXDIST;
	float 	blur		= saturate( (depth-MINDIST)/(MAXDIST-MINDIST) );	
	blur*=blur;
	//float 	blur_near	= pow(saturate( 1-(depth-NEAR)/MINDIST ), 2) * MAXCOF_NEAR;
	//float 	blur 		= (blur_near+blur_far);	

	//float blur = 1;
	float2 	scale 	= float2	(.5f / 1024.h, .5f / 768.h) * MAXCOF * blur;

	// poisson
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

	// sample 
	float3	sum 	= tex2D(s_image,center);
	float 	contrib	= 1.h;
	for (int i=0; i<12; i++)
	{
		float2 	tap 		= center + o[i];
		float4	tap_color	= tex2D	(s_image,tap);
		float 	tap_depth 	= tex2D	(s_position,tap).z;
		float 	tap_contrib	= 1.h;	//(tap_depth>depth)?1.h:0.h;
			sum 		+= tap_color	* tap_contrib;
			contrib		+= tap_contrib;
	}
	return 	float3	(sum/contrib);
}
/**/
#endif	//	USE_DOF

#endif	//	DOF_H_INCLUDED