//=================================================================================================
//Look Modification Transforms
//Stackable preset color modifications (linear ACEScg in/out)
//=================================================================================================
//Console commands
//These can be used in LMTs to control their effect in engine
//RGB + Offset
uniform float4 shader_param_1; //Slope
uniform float4 shader_param_2; //Offset
uniform float4 shader_param_3; //Power
//uniform float4 shader_param_4; 
//uniform float4 shader_param_5; 
//uniform float4 shader_param_6; 
//uniform float4 shader_param_7; 
//uniform float4 shader_param_8; //Used by Beef's NVGs
//=================================================================================================
#include "ACES_Color_Grading.h"
#include "ACES_LMTs\LMT_Contrast_Reduction.h"
#include "ACES_LMTs\LMT_Technicolor.h"
#include "ACES_LMTs\LMT_Bleach_Bypass.h"
#include "ACES_LMTs\LMT_Blue_Fix.h"

void ACES_LMT(inout float3 aces)
{
#ifdef USE_ACES
	//do color grading in ACEScg primaries for more predictable results
	aces = ACES_to_ACEScg(aces);
#endif

	Color_Grading(aces);
	Contrast_Reduction(aces);
	
	//Technicolor(aces);
	//Bleach_Bypass(aces);


#ifdef USE_ACES
	//Return to ACES AP0
	aces = ACEScg_to_ACES(aces);

	Blue_Fix(aces); //ACES AP0 blue color shift fix
#endif

}