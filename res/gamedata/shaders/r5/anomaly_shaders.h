#ifndef        ANOMALY_SHADERS_H
#define        ANOMALY_SHADERS_H
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //Anomaly shaders 1.5																 				 			  //
  //Credits to KD, Anonim, Crossire, Lanforse, daemonjax, Zhora Cementow, Meltac, X-Ray Oxygen, FozeSt, Zagolski, 	 //
 // SonicEthers, David Hoskins, BigWIngs, Zavie																		//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	#define	LUMINANCE_VECTOR float3(0.2125, 0.7154, 0.0721)
	
//////////////////////////////////////////////////////////////
// Screen space sunshafts
	#define SS_INTENSITY float(0.35)		
	#define SS_BLEND_FACTOR float(0.8)		
	#define SS_LENGTH float(1.0)				

//////////////////////////////////////////////////////////////
// Motion blur
	#define MBLUR_SAMPLES 	half(12)
	#define MBLUR_CLAMP	half(0.001)	
	//#define MBLUR_WPN //disabled mblur for weapon
	
/////////////////////////////////////////////////////////////
// Bokeh DoF
	#define BOKEH_AMOUNT float(256.0)

#endif