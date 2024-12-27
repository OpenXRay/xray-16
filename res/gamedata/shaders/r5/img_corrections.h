//DISABLED FOR ACES


#ifndef IMG_CORRECTIONS_H
#define IMG_CORRECTIONS_H
#include "common.h"
#include "anomaly_shaders.h"
#define COLOR_GRADING_LUMINANCE float3(0.213, 0.715, 0.072)
float3 img_corrections(float3 img)
{
	/*
	//exposure
	img.xyz *= pp_img_corrections.x;
	
	//do color in tonemap
	//color grading (thanks KD and Crytek)
	float fLum = dot(img.xyz, COLOR_GRADING_LUMINANCE)*2;
	float3 cMin = 0.0;
	float3 cMed = pp_img_cg.xyz;
	float3 cMax = 1.0;
	float3 cColor = lerp(cMin, cMed , saturate(fLum * 2.0 ) );
	cColor = lerp(cColor, cMax, saturate(fLum - 0.5 ) * 2.0 );
	
	//if (pp_img_cg.x > 0.0 || pp_img_cg.y > 0.0 || pp_img_cg.z > 0.0) 
	{
		img.xyz = saturate(lerp(img.xyz, cColor.xyz , saturate(fLum * 0.15f ) ));	
	}
	
	//saturation
	img.xyz = max(0, lerp(img.xyz, dot(img.xyz, LUMINANCE_VECTOR), (1.0 - pp_img_corrections.z))); 
	*/
	//gamma correction
	img.xyz = pow(img,(1./pp_img_corrections.y));	
	
	//that's all :)
	return img.xyz;
}
#endif