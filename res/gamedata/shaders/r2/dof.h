#ifndef	DOF_H_INCLUDED
#define	DOF_H_INCLUDED

//#define USE_DOF

#ifndef	USE_DOF

half3	dof(float2 center)
{
	half3 	img 	= tex2D		(s_image, center);
	return	img;
}

#else	//	USE_DOF

// x - near y - focus z - far w - sky distance
half4	dof_params;
half3	dof_kernel;	// x,y - resolution pre-scaled z - just kernel size

half DOFFactor( half depth)
{
	half	dist_to_focus	= depth-dof_params.y;
	half 	blur_far	= saturate( dist_to_focus
					/ (dof_params.z-dof_params.y) );
	half 	blur_near	= saturate( dist_to_focus
					/ (dof_params.x-dof_params.y) );
	half 	blur 		= blur_near+blur_far;
	blur*=blur;
	return blur;
}


//#define MAXCOF		5.h
#define MAXCOF		7.h
#define EPSDEPTH	0.0001h
half3	dof(float2 center)
{
	// Scale tap offsets based on render target size
	half 	depth		= tex2D(s_position,center).z;
	if (depth <= EPSDEPTH)	depth = dof_params.w;
	half	blur 		= DOFFactor(depth);

	//half blur = 1;
	//	const amount of blur: define controlled
	//half2 	scale 	= half2	(.5f / 1024.h, .5f / 768.h) * MAXCOF * blur;
	//	const amount of blur: engine controlled
	half2 	scale 	= half2	(.5f / 1024.h, .5f / 768.h) * (dof_kernel.z * blur);
	//	amount of blur varies according to resolution
	//	but kernel size in pixels is fixed.
	//	half2 	scale 	= dof_kernel.xy * blur;

	// poisson
	half2 	o  [12];
		o[0]	= half2(-0.326212f , -0.405810f)*scale;
		o[1] 	= half2(-0.840144f , -0.073580f)*scale;
		o[2] 	= half2(-0.695914f ,  0.457137f)*scale;
		o[3] 	= half2(-0.203345f ,  0.620716f)*scale;
		o[4] 	= half2( 0.962340f , -0.194983f)*scale;
		o[5] 	= half2( 0.473434f , -0.480026f)*scale;
		o[6] 	= half2( 0.519456f ,  0.767022f)*scale;
		o[7] 	= half2( 0.185461f , -0.893124f)*scale;
		o[8] 	= half2( 0.507431f ,  0.064425f)*scale;
		o[9] 	= half2( 0.896420f ,  0.412458f)*scale;
		o[10] 	= half2(-0.321940f , -0.932615f)*scale;
		o[11] 	= half2(-0.791559f , -0.597710f)*scale;

	// sample
	half3	sum 	= tex2D(s_image,center);
	half 	contrib	= 1.h;

   	for (int i=0; i<12; i++)
	{
		float2 	tap 		= center + o[i];
		half4	tap_color	= tex2D	(s_image,tap);
		half 	tap_depth 	= tex2D	(s_position,tap).z;
		if (tap_depth <= EPSDEPTH)	tap_depth = dof_params.w;
		half 	tap_contrib	= DOFFactor(tap_depth);
		sum 		+= tap_color	* tap_contrib;
		contrib		+= tap_contrib;
	}

	return 	half3	(sum/contrib);
}

/*
//	edge along sky line. More light-weight
half3	dof(float2 center)
{
	// Scale tap offsets based on render target size
	half 	depth		= tex2D(s_position,center).z;
//	if (depth <= EPSDEPTH)	depth = dof_params.w;
	if (depth <= EPSDEPTH)	depth = (dof_params.z-dof_params.y)*0.3;
	half	dist_to_focus	= depth-dof_params.y;
	half 	blur_far	= saturate( dist_to_focus
					/ (dof_params.z-dof_params.y) );
	half 	blur_near	= saturate( dist_to_focus
					/ (dof_params.x-dof_params.y) );
	half 	blur 		= (blur_near+blur_far);	
	blur*=blur;

	//half blur = 1;
	//	const amount of blur: define controlled
	//half2 	scale 	= half2	(.5f / 1024.h, .5f / 768.h) * MAXCOF * blur;
	//	const amount of blur: engine controlled
	half2 	scale 	= half2	(.5f / 1024.h, .5f / 768.h) * (dof_kernel.z * blur);
	//	amount of blur varies according to resolution
	//	but kernel size in pixels is fixed.
	//	half2 	scale 	= dof_kernel.xy * blur;

	// poisson
	half2 	o  [12];
		o[0]	= half2(-0.326212f , -0.405810f)*scale;
		o[1] 	= half2(-0.840144f , -0.073580f)*scale;
		o[2] 	= half2(-0.695914f ,  0.457137f)*scale;
		o[3] 	= half2(-0.203345f ,  0.620716f)*scale;
		o[4] 	= half2( 0.962340f , -0.194983f)*scale;
		o[5] 	= half2( 0.473434f , -0.480026f)*scale;
		o[6] 	= half2( 0.519456f ,  0.767022f)*scale;
		o[7] 	= half2( 0.185461f , -0.893124f)*scale;
		o[8] 	= half2( 0.507431f ,  0.064425f)*scale;
		o[9] 	= half2( 0.896420f ,  0.412458f)*scale;
		o[10] 	= half2(-0.321940f , -0.932615f)*scale;
		o[11] 	= half2(-0.791559f , -0.597710f)*scale;

	// sample 
	half3	sum 	= tex2D(s_image,center);
	half 	contrib	= 1.h;
	for (int i=0; i<12; i++)
	{
		float2 	tap 		= center + o[i];
		half4	tap_color	= tex2D	(s_image,tap);
		half 	tap_depth 	= tex2D	(s_position,tap).z;
//		if (tap_depth <= EPSDEPTH)	tap_depth = dof_params.w;
		if (tap_depth <= EPSDEPTH)	tap_depth = (dof_params.z-dof_params.y)*0.3;
//		half 	tap_contrib	= 1.h;	//(tap_depth>depth)?1.h:0.h;
		half 	tap_contrib	= 1-saturate(abs(tap_depth-depth)/dist_to_focus);
			sum 		+= tap_color	* tap_contrib;
			contrib		+= tap_contrib;
	}
	return 	half3	(sum/contrib);
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
half3	dof(float2 center)
{
	// Scale tap offsets based on render target size
	half 	depth		= tex2D(s_position,center).z;
	if (depth<=EPSDEPTH)	depth = MAXDIST;
	half 	blur		= saturate( (depth-MINDIST)/(MAXDIST-MINDIST) );	
	blur*=blur;
	//half 	blur_near	= pow(saturate( 1-(depth-NEAR)/MINDIST ), 2) * MAXCOF_NEAR;
	//half 	blur 		= (blur_near+blur_far);	

	//half blur = 1;
	half2 	scale 	= half2	(.5f / 1024.h, .5f / 768.h) * MAXCOF * blur;

	// poisson
	half2 	o  [12];
		o[0]	= half2(-0.326212f , -0.405810f)*scale;
		o[1] 	= half2(-0.840144f , -0.073580f)*scale;
		o[2] 	= half2(-0.695914f ,  0.457137f)*scale;
		o[3] 	= half2(-0.203345f ,  0.620716f)*scale;
		o[4] 	= half2( 0.962340f , -0.194983f)*scale;
		o[5] 	= half2( 0.473434f , -0.480026f)*scale;
		o[6] 	= half2( 0.519456f ,  0.767022f)*scale;
		o[7] 	= half2( 0.185461f , -0.893124f)*scale;
		o[8] 	= half2( 0.507431f ,  0.064425f)*scale;
		o[9] 	= half2( 0.896420f ,  0.412458f)*scale;
		o[10] 	= half2(-0.321940f , -0.932615f)*scale;
		o[11] 	= half2(-0.791559f , -0.597710f)*scale;

	// sample 
	half3	sum 	= tex2D(s_image,center);
	half 	contrib	= 1.h;
	for (int i=0; i<12; i++)
	{
		float2 	tap 		= center + o[i];
		half4	tap_color	= tex2D	(s_image,tap);
		half 	tap_depth 	= tex2D	(s_position,tap).z;
		half 	tap_contrib	= 1.h;	//(tap_depth>depth)?1.h:0.h;
			sum 		+= tap_color	* tap_contrib;
			contrib		+= tap_contrib;
	}
	return 	half3	(sum/contrib);
}
/**/
#endif	//	USE_DOF

#endif	//	DOF_H_INCLUDED