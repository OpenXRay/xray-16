#ifndef	common_defines_h_included
#define	common_defines_h_included

//////////////////////////////////////////////////////////////////////////////////////////
// Defines                                		//
#define def_gloss       float(2.0/255.0)
#define def_aref        float(200.0/255.0)
#define def_dbumph      float(0.333)
#define def_virtualh    float(0.05)              // 5cm
#define def_distort     float(0.05)             // we get -0.5 .. 0.5 range, this is -512 .. 512 for 1024, so scale it
#define def_hdr         float(9.0)         		// hight luminance range float(3.0)
#define def_hdr_clip	float(0.75)        		//

#define	LUMINANCE_VECTOR	float3(0.3, 0.38, 0.22)

//////////////////////////////////////////////////////////////////////////////////////////
#ifndef SMAP_size
#define SMAP_size	1024.0
#endif
//////////////////////////////////////////////////////////////////////////////////////////

#endif	//	common_defines_h_included