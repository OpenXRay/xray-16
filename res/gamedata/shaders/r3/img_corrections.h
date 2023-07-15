#ifndef IMG_CORRECTIONS_H
#define IMG_CORRECTIONS_H
#include "common.h"
#include "anomaly_shaders.h"

float3 img_corrections(float3 img)
{
	//exposure
	img.xyz *= pp_img_corrections.x;
	
	//color grading (thanks KD and Crytek and Cjayho)
	float fLum = dot(img.xyz, LUMINANCE_VECTOR);
	float3 cColor = lerp(0.0, pp_img_cg.xyz, saturate( fLum * 2.0 ) );
	cColor = lerp( cColor, 1.0, saturate( fLum - 0.5) * 2.0 );
	
    if (pp_img_cg.x > 0.0 || pp_img_cg.y > 0.0 || pp_img_cg.z > 0.0) 
	{
		img.xyz = saturate(lerp( img.xyz, cColor.xyz , saturate( fLum * 0.15 ) ));	
	}
	
	//saturation
    img.xyz = lerp(img.xyz, dot(img.xyz, LUMINANCE_VECTOR), (1.0 - pp_img_corrections.z)); 
	
	//gamma correction
	img.xyz = pow(img,(1.0/pp_img_corrections.y));	
	
	//that's all :)
	return img.xyz;
}
#endif