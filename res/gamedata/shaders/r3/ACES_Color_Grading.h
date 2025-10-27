//=================================================================================================
//Color grading - Color Decision List
//ACEScc compatible CDL https://github.com/ampas/aces-dev/blob/518c27f577e99cdecfddf2ebcfaa53444b1f9343/documents/LaTeX/S-2014-003/appendixB.tex#L1
//=================================================================================================

float ASC_CDL(float x, float Slope, float Offset, float Power)
{
	x *= Slope;
	x += Offset;
	
	//ACEScc has negative values that must not be clamped
	//Use this to apply power to the negative range 
	float IsNeg = x>=0 ? 1 : -1;
	x = abs(x);
	x = pow(x,Power);
	x *= IsNeg;
	
	return x;
}

void Color_Grading(inout float3 aces)
{
	float3 x = aces;
	
	//ASC-CDL (SOP-S) style color grading
	float3 Slope = {1.000, 1.000, 1.000};
	float3 Offset = {0.000, 0.000, 0.000};
	float3 Power = {1.000, 1.000, 1.000};
	float Saturation = 1;

//load custom settings from another file
#include "ACES_settings.h"
	
#ifdef USE_LOG_GRADING
	//to ACEScc log space
	x = lin_to_ACEScc(x);
#endif
	
	//apply CDL color grading
	x = float3(
	ASC_CDL(x.r, Slope.r, Offset.r, Power.r),
	ASC_CDL(x.g, Slope.g, Offset.g, Power.g),
	ASC_CDL(x.b, Slope.b, Offset.b, Power.b));
	
	//apply saturation
	float luma = dot(x, LUMINANCE_VECTOR);
	x = luma + Saturation * (x - luma);

#ifdef USE_LOG_GRADING
	//from ACEScc log space
	x = ACEScc_to_lin(x);
#endif
	
	aces = x;
}