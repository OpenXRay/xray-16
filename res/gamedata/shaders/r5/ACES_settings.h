//=================================================================================================
//Defines for and variables for ACES and color grading
//=================================================================================================
#define USE_LOG_GRADING //Use log space for color grading
//=================================================================================================

//
//manual settings
//
/*
	Slope = float3(1.000, 1.000, 1.000);
	Offset = float3(0.000, 0.000, 0.000);
	Power = float3(1.000, 1.000, 1.000);
	Saturation = 1.000;
*/
//
//settings for supporting in-game console commands
//
	Slope = pp_img_corrections.xxx; 	//brightness
	Power = 2*(1-pp_img_cg.rgb); 		//color grading
	Saturation = pp_img_corrections.z; 	//saturation	
