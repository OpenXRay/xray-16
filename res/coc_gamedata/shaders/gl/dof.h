#ifndef	DOF_H_INCLUDED
#define	DOF_H_INCLUDED

//#define USE_DOF

#ifndef	USE_DOF

float3	dof(float3 img, float2 center)
{
	return	img;
}

#else	//	USE_DOF

// x - near y - focus z - far w - sky distance
uniform float4	dof_params;
uniform float3	dof_kernel;	// x,y - resolution pre-scaled z - just kernel size

float DOFFactor(float depth)
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


#define MAXCOF		7.0
#define EPSDEPTHDOF	0.0001
float3	dof(float3 img, float2 center)
{
	// Scale tap offsets based on render target size
#ifndef USE_MSAA
	float 	depth		= tex2D(s_position,center).z;
#else
	float 	depth		= texelFetch(s_position, int2( center * pos_decompression_params2.xy ), 0).z;
#endif
	if (depth <= EPSDEPTHDOF)	depth = dof_params.w;
	float	blur 		= DOFFactor(depth);

	//float blur = 1;
	//	const amount of blur: define controlled
	//float2 	scale 	= float2	(0.5 / 1024.0, 0.5 / 768.0) * MAXCOF * blur;
	//	const amount of blur: engine controlled
	float2 	scale 	= float2	(0.5 / 1024.0, 0.5 / 768.0) * (dof_kernel.z * blur);
	//	amount of blur varies according to resolution
	//	but kernel size in pixels is fixed.
	//	float2 	scale 	= dof_kernel.xy * blur;

	// poisson
	float2 	o  [12];
		o[0]	= float2(-0.326212, -0.405810)*scale;
		o[1] 	= float2(-0.840144, -0.073580)*scale;
		o[2] 	= float2(-0.695914,  0.457137)*scale;
		o[3] 	= float2(-0.203345,  0.620716)*scale;
		o[4] 	= float2( 0.962340, -0.194983)*scale;
		o[5] 	= float2( 0.473434, -0.480026)*scale;
		o[6] 	= float2( 0.519456,  0.767022)*scale;
		o[7] 	= float2( 0.185461, -0.893124)*scale;
		o[8] 	= float2( 0.507431,  0.064425)*scale;
		o[9] 	= float2( 0.896420,  0.412458)*scale;
		o[10] 	= float2(-0.321940, -0.932615)*scale;
		o[11] 	= float2(-0.791559, -0.597710)*scale;

	// sample
	float3	sum 	= img;
	float 	contrib	= 1.0;

   	for (int i=0; i<12; i++)
	{
		float2 	tap 		= center + o[i];
		float4	tap_color	= tex2D	(s_image,tap);
#ifndef USE_MSAA
		float 	tap_depth 	= tex2D	(s_position,tap).z;
#else
		float 	tap_depth 	= texelFetch(s_position, int2( tap* pos_decompression_params2.xy ), 0).z;
#endif
		if (tap_depth <= EPSDEPTHDOF)	tap_depth = dof_params.w;
		float 	tap_contrib	= DOFFactor(tap_depth);
		sum 		+= tap_color.rgb	* tap_contrib;
		contrib		+= tap_contrib;
	}

	return 	float3	(sum/contrib);
}

#endif	//	USE_DOF

#endif	//	DOF_H_INCLUDED