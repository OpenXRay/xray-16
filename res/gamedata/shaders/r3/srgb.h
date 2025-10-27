//=================================================================================================
//Gamma Correction
//=================================================================================================
//#define USE_STRICT_GAMMA_CORRECTION //use gamma correction for sky blending, might distort the original colors
//=================================================================================================

float LinearTosRGB(float gammaPre)
{
	/*
	float Low = gammaPre * 12.92;
	float High = (pow(gammaPre, 1.0 / 2.4) * 1.055) - 0.055;
	return (gammaPre <= 0.0031308) ? Low : High;
	*/
	//return (gammaPre <= 0.00313080495356037151702786377709) ? gammaPre * 12.92 : (1.055 * pow(gammaPre, 0.41666666666666666666666666666667) - 0.055);
	//return max(1.055 * pow(gammaPre, 0.416666667) - 0.055, 0.0);
	
	//Cheap sRGB doesn't cause clipping
	return pow(gammaPre, 0.45454545);
}

float3 LinearTosRGB(float3 gammaPre)
{
	gammaPre= max(0.0, gammaPre);
	float3 gammaPost = float3(
	LinearTosRGB(gammaPre.r),
	LinearTosRGB(gammaPre.g),
	LinearTosRGB(gammaPre.b));
	return gammaPost;
}

float SRGBToLinear(float gammaPre)
{
	/*
	float Low = gammaPre / 12.92;
	float High = pow((gammaPre + 0.055) / 1.055, 2.4);
	return(gammaPre <= 0.04045) ? Low : High;
	*/
	//return (gammaPre <= 0.04045) ? gammaPre * 0.07739938080495356037151702786378 : pow((gammaPre + 0.055) * 0.94786729857819905213270142180095, 2.4);
	//return gammaPre * (gammaPre * (gammaPre * 0.305306011 + 0.682171111) + 0.012522878);
	
	//Cheap sRGB doesn't cause clipping
	return pow(gammaPre, 2.2);
}

float3 SRGBToLinear(float3 gammaPre)
{
	gammaPre= max(0.0, gammaPre);
	float3 gammaPost = float3(
	SRGBToLinear(gammaPre.r),
	SRGBToLinear(gammaPre.g),
	SRGBToLinear(gammaPre.b));
	return gammaPost;
}

#include "tonemap_srgb.h"