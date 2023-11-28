#include "common.h"

uniform float3x4	m_xform		;
uniform float3x4	m_xform_v	;
uniform float4 		consts; 	// {1/quant,1/quant,???,???}
uniform float4 		c_scale,c_bias,wind,wave;
uniform float2 		c_sun;		// x=*, y=+

v2p_bumped 	main 	(v_tree I)
{
	I.Nh	=	unpack_D3DCOLOR(I.Nh);
	I.T		=	unpack_D3DCOLOR(I.T);
	I.B		=	unpack_D3DCOLOR(I.B);
	
	// Transform to world coords
	float3 pos		= mul			(m_xform, I.P);
	float H	= I.P;

	//
	float2 	result	= 0;
	float4 	w_pos 	= float4(pos.x+result.x, pos.y, pos.z+result.y, 1);
	float2 	tc = (I.tc * consts).xy;
	float hemi 	= I.Nh.w * c_scale.w + c_bias.w;
//	float hemi 	= I.Nh.w;

	// Eye-space pos/normal
	v2p_bumped 		O;
	float3	Pe		= mul		(m_V,  	w_pos		);
	//float3 Pe = mul(m_V, float4(pos.xyz,1));
	O.tcdh 			= float4	(tc.xyyy			);
	O.hpos 			= mul		(m_VP,	w_pos		);
	O.position		= float4	(Pe, 	hemi		);

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	float suno 	= I.Nh.w * c_sun.x + c_sun.y	;
	O.tcdh.w		= suno;					// (,,,dir-occlusion)
#endif

	// Calculate the 3x3 transform from tangent space to eye-space
	// TangentToEyeSpace = object2eye * tangent2object
	//		    = object2eye * transpose(object2tangent) (since the inverse of a rotation is its transpose)
	float3 N = unpack_bx2(I.Nh);	// just scale (assume normal in the -.5f, .5f)
	float3 T = unpack_bx2(I.T);		//
	float3 B = unpack_bx2(I.B);		//  
	N = normalize(N);
	B = normalize(B);
	T = normalize(T);
	
	float3 sphereOffset = float3(0.0, 1.0, 0.0);
	float3 sphereScale = float3(1.0, 2.0, 1.0);
	float3 sphereN = normalize(sphereScale * I.P.xyz + sphereOffset); //Spherical normals trick
	/*
	float3 camFacingN = normalize((w_pos - eye_position.xyz) * float3(-1,0,-1));
	sphereN = lerp(camFacingN, sphereN, saturate(H)); //roots face the camera, the tips face the sky
	
	sphereN.xz *= 0.5;
	sphereN.y = sqrt(1 - saturate(dot(sphereN.xz, sphereN.xz)));
	sphereN = normalize(sphereN);
	*/ 
	
	//tangent basis
	float3 flatB = float3(0,0,1);

	if (abs(dot(sphereN, flatB)) > 0.99f) 
		flatB = float3(0,1,0);
		
	float3 flatT = normalize(cross(sphereN, flatB));
	flatB = normalize(cross(sphereN, flatT));	
	
	//foliage
	float foliageMat = 0.5; //foliage
	//float foliageMask = saturate(abs(xmaterial-foliageMat)-0.02); //foliage
	float foliageMask = (abs(xmaterial-foliageMat) >= 0.2) ? 1 : 0; //foliage
	//float foliageMask = 1; //foliage
	N = normalize(lerp(N, sphereN, foliageMask)); //blend to foliage normals
	//B = normalize(lerp(B, flatB, foliageMask)); //blend to foliage normals
	//T = normalize(lerp(T, flatT, foliageMask)); //blend to foliage normals
	
	float3x3 xform	= mul	((float3x3)m_xform_v, float3x3(
						T.x,B.x,N.x,
						T.y,B.y,N.y,
						T.z,B.z,N.z
					));

	// The pixel shader operates on the bump-map in [0..1] range
	// Remap this range in the matrix, anyway we are pixel-shader limited :)
	// ...... [ 2  0  0  0]
	// ...... [ 0  2  0  0]
	// ...... [ 0  0  2  0]
	// ...... [-1 -1 -1  1]
	// issue: strange, but it's slower :(
	// issue: interpolators? dp4? VS limited? black magic?

	// Feed this transform to pixel shader
	O.M1 			= xform[0];
	O.M2 			= xform[1];
	O.M3 			= xform[2];

#ifdef 	USE_TDETAIL
	O.tcdbump		= O.tcdh * dt_params;		// dt tc
#endif

	return O;
}
FXVS;
